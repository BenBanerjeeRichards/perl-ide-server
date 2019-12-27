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

FileSymbolMap loadSymbols(std::string path) {
    auto context = directoryOf(path);
    auto includes = getIncludePaths(path);
    includes.emplace_back(context);     // Force search in current directory
    FileSymbolMap fileSymbolMap;
    doLoadSymbols(path, includes, fileSymbolMap);
    return fileSymbolMap;
}
