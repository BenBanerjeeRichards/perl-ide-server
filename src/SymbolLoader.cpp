//
// Created by Ben Banerjee-Richards on 27/12/2019.
//

#include "SymbolLoader.h"

void doLoadSymbols(std::string path, std::vector<std::string> includes, FileSymbolMap &fileSymbolMap, Cache &cache) {
    // Already been processed so done. Probably a cycle somewhere
    if (fileSymbolMap.count(path) > 0) return;
    std::cout << "Loading symbols in " << path << std::endl;

    // Try to load each FileSymbols from cache
    auto maybeCacheItem = cache.getItem(path);
    FileSymbols fileSymbols;
    if (maybeCacheItem.has_value()) {
        fileSymbols = *maybeCacheItem.value();
    } else {
        fileSymbols = analysis::getFileSymbols(path);
        cache.addItem(path, std::make_shared<FileSymbols>(fileSymbols));
    }

    fileSymbolMap[path] = fileSymbols;

    for (auto &import : fileSymbols.imports) {
        std::string path = (import.type == ImportType::Path) ? resolvePath(includes, import.data) : resolveModulePath(
                includes, splitPackage(import.data));

        if (path.empty()) {
            std::cerr << "Failed to resolve path against @INC - " << import.data << std::endl;
            continue;
        }
        doLoadSymbols(path, includes, fileSymbolMap, cache);
    }
}

FileSymbolMap loadAllFileSymbols(std::string path, std::string contextPath, Cache &cache) {
    auto context = directoryOf(contextPath);
    auto includes = getIncludePaths(context);
    includes.emplace_back(context);     // Force search in current directory
    FileSymbolMap fileSymbolMap;
    doLoadSymbols(path, includes, fileSymbolMap, cache);

    // Replace temp file with context path
    if (path != contextPath && fileSymbolMap.count(path) > 0) {
        auto pathValue = fileSymbolMap[path];
        fileSymbolMap.erase(path);
        fileSymbolMap[contextPath] = pathValue;
    }

    std::cout << buildGlobalVariablesMap(fileSymbolMap).toStr() << std::endl;

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

