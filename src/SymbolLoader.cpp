//
// Created by Ben Banerjee-Richards on 27/12/2019.
//

#include "SymbolLoader.h"

void doLoadSymbols(std::string path, std::vector<std::string> includes, FileSymbolMap &fileSymbolMap) {
    // Already been processed so done. Probably a cycle somewhere
    if (fileSymbolMap.count(path) > 0) return;
    std::cout << "Loading symbols in " << path << std::endl;

    FileSymbols fileSymbols = analysis::getFileSymbols(path);
    fileSymbolMap[path] = fileSymbols;

    for (auto &import : fileSymbols.imports) {
        std::string path = (import.type == ImportType::Path) ? resolvePath(includes, import.data) : resolveModulePath(
                includes, splitPackage(import.data));

        if (path.empty()) {
            std::cerr << "Failed to resolve path against @INC - " << import.data << std::endl;
            continue;
        }
        doLoadSymbols(path, includes, fileSymbolMap);
    }

}

FileSymbolMap loadAllFileSymbols(std::string path, std::string contextPath) {
    auto context = directoryOf(contextPath);
    auto includes = getIncludePaths(contextPath);
    includes.emplace_back(context);     // Force search in current directory
    FileSymbolMap fileSymbolMap;
    doLoadSymbols(path, includes, fileSymbolMap);

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

std::optional<Symbols> buildSymbols(std::string rootPath, std::string contextPath) {
    auto fileSymbolsMap = loadAllFileSymbols(rootPath, contextPath);
    if (fileSymbolsMap.count(rootPath) == 0) {
        std::cerr << "FileAnalysis failed - could not find symbols for root file with path " << rootPath << std::endl;
        return std::optional<Symbols>();
    }

    Symbols symbols;
    symbols.rootFilePath = rootPath;
    symbols.rootFileSymbols = fileSymbolsMap[rootPath];
    symbols.globalVariablesMap = buildGlobalVariablesMap(fileSymbolsMap);
    return symbols;
}