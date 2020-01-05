//
// Created by Ben Banerjee-Richards on 27/12/2019.
//

#include "SymbolLoader.h"
#include "IOException.h"

std::optional<FileSymbols> loadSymbols(std::string path, Cache &cache) {
    // Try to load each FileSymbols from cache
    auto begin = std::chrono::steady_clock::now();
    auto maybeCacheItem = cache.getItem(path);
    FileSymbols fileSymbols;
    if (maybeCacheItem.has_value()) {
        fileSymbols = *maybeCacheItem.value();
    } else {
        auto analysisDetail = isSystemPath(path) ? analysis::AnalysisDetail::PACKAGE_ONLY
                                                 : analysis::AnalysisDetail::FULL;

        try {
            fileSymbols = analysis::getFileSymbols(path, analysisDetail);
        } catch (IOException e) {
            std::cerr << "Failed to load symbols - file not found:" << path << std::endl;
            return std::optional<FileSymbols>();
        }
        cache.addItem(path, std::make_shared<FileSymbols>(fileSymbols));
    }
    auto end = std::chrono::steady_clock::now();
    std::cout << "[" << std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count() << "ms]"
              << "Loaded symbols for " << path << std::endl;

    return fileSymbols;
}


void doLoadAllSymbols(std::string path, std::vector<std::string> includes, FileSymbolMap &fileSymbolMap, Cache &cache) {
    // Already been processed so done. Probably a cycle somewhere
    if (fileSymbolMap.count(path) > 0) return;

    auto maybeFileSymbols = loadSymbols(path, cache);
    if (!maybeFileSymbols.has_value()) return;
    FileSymbols fileSymbols = maybeFileSymbols.value();

    fileSymbolMap[path] = fileSymbols;

    for (auto &import : fileSymbols.imports) {
        auto maybePath = (import.type == ImportType::Path) ? resolvePath(includes, import.data) : resolveModulePath(
                includes, splitPackage(import.data));

        if (!maybePath.has_value()) {
            std::cerr << "Failed to resolve path against @INC - " << import.data << std::endl;
            continue;
        }
        doLoadAllSymbols(maybePath.value(), includes, fileSymbolMap, cache);
    }
}

FileSymbolMap loadAllFileSymbols(std::string path, std::string contextPath, Cache &cache) {
    auto context = directoryOf(contextPath);
    auto includes = getIncludePaths(context);
    includes.emplace_back(context);     // Force search in current directory
    FileSymbolMap fileSymbolMap;
    doLoadAllSymbols(path, includes, fileSymbolMap, cache);

    // Replace temp file with context path
    if (path != contextPath && fileSymbolMap.count(path) > 0) {
        auto pathValue = fileSymbolMap[path];
        fileSymbolMap.erase(path);
        fileSymbolMap[contextPath] = pathValue;
    }

    return fileSymbolMap;
}

GlobalVariablesMap buildGlobalVariablesMap(const FileSymbolMap &fileSymbolsMap) {
    GlobalVariablesMap globalVariablesMap;
    for (const auto &fileSymbolWithPath : fileSymbolsMap) {
        auto path = fileSymbolWithPath.first;
        auto fileSymbols = fileSymbolWithPath.second;

        for (const auto &globalWithUsages : fileSymbols.globals) {
            auto global = globalWithUsages.first;
            auto usages = globalWithUsages.second;

            globalVariablesMap.addGlobal(global, path, usages);
        }
    }

    return globalVariablesMap;
}

std::optional<Symbols> buildSymbols(std::string rootPath, std::string contextPath, Cache &cache) {
    auto fileSymbolsMap = loadAllFileSymbols(rootPath, contextPath, cache);
    if (fileSymbolsMap.count(contextPath) == 0) {
        std::cerr << "FileAnalysis failed - could not find symbols for root file with path " << rootPath << std::endl;
        return std::optional<Symbols>();
    }

    Symbols symbols;
    symbols.rootFilePath = contextPath;
    symbols.rootFileSymbols = fileSymbolsMap[contextPath];
    symbols.globalVariablesMap = buildGlobalVariablesMap(fileSymbolsMap);
    return symbols;
}

std::optional<Symbols> buildSymbols(std::string rootPath, std::string contextPath) {
    Cache cache;
    return buildSymbols(rootPath, contextPath, cache);
}

std::set<std::string> doBuildGraphWithFile(const std::string &path, std::vector<std::string> includes,
                                           std::unordered_map<std::string, PathNode> &importGraph, Cache &cache) {
    auto maybeFileSymbols = loadSymbols(path, cache);

    // Now start to build the graph
    if (importGraph.count(path) == 0) {
        importGraph[path] = PathNode();
    }

    // For the children, we need the full path
    if (maybeFileSymbols.has_value()) {
        std::set<std::string> childPaths;

        for (const auto &import : maybeFileSymbols.value().imports) {
            std::optional<std::string> maybeChildPath;
            if (import.type == ImportType::Path) {
                maybeChildPath = resolvePath(includes, import.data);
            } else {
                maybeChildPath = resolveModulePath(includes, splitPackage(import.data));
            }

            if (!maybeChildPath.has_value()) continue;
            auto childPath = maybeChildPath.value();

            childPaths.insert(childPath);
            importGraph[path].children.insert(childPath);

            // Now update parent for this path's parents
            if (importGraph.count(childPath) == 0) {
                importGraph[childPath] = PathNode();
            }

            importGraph[childPath].parents.insert(path);
        }

        return childPaths;
    }

    return std::set<std::string>{};
}

std::unordered_map<std::string, PathNode>
loadProjectGraph(const std::vector<std::string> &projectFiles, std::vector<std::string> includes, Cache &cache) {
    // First load every file, along with every file that the file includes
    // Caching ensures that we don't double load anything
    FileSymbolMap fileSymbolsMap;

    // Import graph of all the files. Directed, disconnected, cyclic graph
    std::unordered_map<std::string, PathNode> importGraph;

    std::vector<std::string> processedFiles;
    std::queue<std::string> workList;

    for (const auto &path : projectFiles) workList.push(path);

    while (!workList.empty()) {
        std::string path = workList.front();
        workList.pop();
        // Already processed this file
        if (std::find(processedFiles.begin(), processedFiles.end(), path) != processedFiles.end()) continue;

        auto children = doBuildGraphWithFile(path, includes, importGraph, cache);
        processedFiles.emplace_back(path);
        for (auto child : children) {
            std::cout << "Adding to worklist " << child << std::endl;
            workList.push(child);
        }
    }

    return importGraph;
}


void doFindPathsConnectedTo(std::string currentPath, std::unordered_map<std::string, PathNode> &importGraph,
                            std::set<std::string> &connectedList) {
    if (importGraph.count(currentPath) == 0) return;
    if (std::find(connectedList.begin(), connectedList.end(), currentPath) != connectedList.end()) return;

    for (auto child : importGraph[currentPath].children) {
        if (std::find(connectedList.begin(), connectedList.end(), child) != connectedList.end()) {
            // child is in list already, so stop at this branch
            continue;
        }
        connectedList.insert(child);
        doFindPathsConnectedTo(child, importGraph, connectedList);
    }

    for (auto parent : importGraph[currentPath].parents) {
        if (std::find(connectedList.begin(), connectedList.end(), parent) != connectedList.end()) {
            // child is in list already, so stop at this branch
            continue;
        }
        connectedList.insert(parent);
        doFindPathsConnectedTo(parent, importGraph, connectedList);
    }
}

std::set<std::string> pathsConnectedTo(std::string path, std::unordered_map<std::string, PathNode> &importGraph) {
    std::set<std::string> connectedList = std::set<std::string>{path};
    doFindPathsConnectedTo(path, importGraph, connectedList);
    return connectedList;
}

void doFindDescendentsOf(std::string currentPath, std::unordered_map<std::string, PathNode> &importGraph,
                         std::set<std::string> &seen) {
    if (importGraph.count(currentPath) == 0) return;
    if (std::find(seen.begin(), seen.end(), currentPath) != seen.end()) return;

    for (const auto &child : importGraph[currentPath].children) {
        seen.insert(child);
        doFindDescendentsOf(child, importGraph, seen);
    }
}


std::set<std::string> descendentsOf(std::string path, std::unordered_map<std::string, PathNode> graph) {
    auto seen = std::set<std::string>();
    doFindDescendentsOf(path, graph, seen);
    return seen;
}

std::set<std::string> relatedFiles(std::string path, std::unordered_map<std::string, PathNode> graph) {
    auto paths = std::set<std::string>{path};

    // Add pure descendents
    auto descs = descendentsOf(path, graph);
    paths.insert(descs.begin(), descs.end());

    if (graph.count(path) != 0) {
        for (auto parent : graph[path].parents) {
            auto connected = pathsConnectedTo(parent, graph);
            paths.insert(connected.begin(), connected.end());
        }
    }

    return paths;
}


std::string projGraphToDot(const std::unordered_map<std::string, PathNode> &graph, bool showParents) {
    std::string dot = "digraph g {\n";
    for (const auto &node : graph) {
        auto nodeName = fileName(node.first);
        dot += "\t" + nodeName + "\n";

        for (const auto &child : node.second.children) {
            dot += "\t" + nodeName + "->" + fileName(child) + "\n";
        }

        if (showParents) {
            for (const auto &parent : node.second.parents) {
                dot += "\t" + nodeName + "->" + fileName(parent) + +"[style=dashed]\n";
            }
        }
    }

    dot += "}";
    return dot;
}