//
// Created by Ben Banerjee-Richards on 2019-11-28.
//

#include "FileAnalysis.h"

FileSymbols analysis::getFileSymbols(const std::string &path) {
    auto program = readFile(path);
    Tokeniser tokeniser(program);
    std::vector<Token> tokens = tokeniser.tokenise();
    FileSymbols fileSymbols;

    int partial = -1;
    auto parseTree = buildParseTree(tokens, partial);
    fileSymbols.packages = parsePackages(parseTree);
    parseFirstPass(parseTree, fileSymbols);
    buildVariableSymbolTree(parseTree, fileSymbols);
    return fileSymbols;
}

std::vector<AutocompleteItem>
analysis::autocompleteVariables(const std::string &filePath, const std::string &contextPath, FilePos location,
                                std::vector<std::string> projectFiles, char sigilContext, Cache &cache) {
    std::vector<AutocompleteItem> completions;
    auto symbolsMaybe = buildProjectSymbols(filePath, contextPath, projectFiles, cache, true, false, false);
    if (!symbolsMaybe.has_value()) {
        return completions;
    }

    Symbols symbols = symbolsMaybe.value();

    // Symbol map contains all lexically scoped variables
    SymbolMap symbolMap = getSymbolMap(symbols.rootFileSymbols, location);

    // Get package - so if we are in a global's package, we can suggest it's short name providing there are no
    // conflicts with lexically scoped variables
    auto currentPackage = findPackageAtPos(symbols.rootFileSymbols.packages, location);
    for (auto globalItem : symbols.globalVariablesMap.globalsMap) {
        GlobalVariable global = globalItem.first;
        if (global.getPackage() == currentPackage) {
            if (symbolMap.count(global.getSigil() + global.getName()) == 0) {
                auto variableName = variableForCompletion(global.getSigil() + global.getName(),
                                                          sigilContext);
                if (!variableName.empty()) {
                    completions.emplace_back(AutocompleteItem(variableName, global.getFullName()));
                }
            } else {
                auto variableName = variableForCompletion(global.getFullName(), sigilContext);
                if (!variableName.empty()) {
                    completions.emplace_back(AutocompleteItem(variableName, ""));
                }
            }
        } else {
            auto variableName = variableForCompletion(global.getFullName(), sigilContext);
            if (!variableName.empty()) {
                completions.emplace_back(AutocompleteItem(variableName, ""));
            }
        }
    }

    // Now add lexical variables
    for (const auto &symbolVal : symbolMap) {
        auto variableName = variableForCompletion(symbolVal.first, sigilContext);
        if (!variableName.empty()) {
            completions.emplace_back(AutocompleteItem(variableName, ""));
        }
    }

    return completions;
}

std::vector<AutocompleteItem>
analysis::autocompleteSubs(const std::string &filePath, const std::string &contextPath, FilePos location,
                           std::vector<std::string> projectFiles, Cache &cache) {
    std::vector<AutocompleteItem> completions;
    auto symbolsMaybe = buildProjectSymbols(filePath, contextPath, projectFiles, cache, false, true, false);
    if (!symbolsMaybe.has_value()) {
        return completions;
    }

    Symbols symbols = symbolsMaybe.value();
    std::string currentPackage = findPackageAtPos(symbols.rootFileSymbols.packages, location);

    for (auto sub : symbols.subroutines) {
        if (sub.package == currentPackage) {
            completions.emplace_back(AutocompleteItem(sub.name, sub.name + "()"));
        } else {
            completions.emplace_back(
                    AutocompleteItem(sub.package + "::" + sub.name, sub.name + "()"));
        }
    }

    return completions;
}

std::optional<std::unordered_map<std::string, std::vector<Range>>>
analysis::findVariableUsages(const std::string &filePath, const std::string &contextPath, FilePos location,
                             Symbols &symbols) {
    std::unordered_map<std::string, std::vector<Range>> usages;

    // Test if local variable first
    auto maybeVariable = findVariableAtLocation(symbols.rootFileSymbols, location);
    if (maybeVariable.has_value()) {
        // Was a local variable
        usages[symbols.rootFilePath] = maybeVariable.value().usages;
        return usages;
    } else {
        // Not a local, now check for global variable and get usages
        // Will include multiple files
        auto globals = symbols.globalVariablesMap.globalsMap;

        for (auto globalWithFiles : globals) {
            GlobalVariable globalVariable = globalWithFiles.first;
            // This global doesn't appear in the current file, so skip
            if (globalWithFiles.second.count(contextPath) == 0) {
                continue;
            }

            // Now check every usage to see if location is within usage
            for (Range &usage : globalWithFiles.second[contextPath]) {
                if (insideRange(usage, location)) {
                    // We've found a global
                    return globals[globalVariable];
                }
            }
        }
    }

    return {};
}


std::optional<analysis::Declaration>
doFindSubroutineDeclaration(std::string contextPath, FilePos location, Symbols &symbols) {
    auto subMap = symbols.subroutineMap.subsMap;

    for (const auto &subMapItem : subMap) {
        auto fileToRangeMap = subMapItem.second;
        if (fileToRangeMap.count(contextPath) == 0) continue;

        for (auto range : fileToRangeMap[contextPath]) {
            if (insideRange(range, location)) {
                // We've found the subroutine symbol, now get declaration
                analysis::Declaration decl;
                decl.path = subMapItem.first.path;
                decl.pos = subMapItem.first.subroutine.pos;
                return decl;
            }
        }
    }

    return {};
}

std::optional<std::unordered_map<std::string, std::vector<Range>>>
analysis::findSubroutineUsages(const std::string &filePath, const std::string &contextPath, FilePos location,
                               Symbols &symbols) {
    auto subMap = symbols.subroutineMap.subsMap;

    for (const auto &subMapItem : subMap) {
        auto fileToRangeMap = subMapItem.second;
        if (fileToRangeMap.count(contextPath) == 0) continue;

        for (auto range : fileToRangeMap[contextPath]) {
            if (insideRange(range, location)) {
                // We've found the subroutine symbol, now get declaration
                return subMap[subMapItem.first];
            }
        }
    }

    return {};
}

std::unordered_map<std::string, std::vector<Range>>
analysis::findUsages(const std::string &filePath, const std::string &contextPath, FilePos location,
                     std::vector<std::string> projectFiles, Cache &cache) {
    auto symbolsMaybe = buildProjectSymbols(filePath, contextPath, std::move(projectFiles), cache, true, false, true);
    if (!symbolsMaybe.has_value()) {
        return std::unordered_map<std::string, std::vector<Range>>();
    }

    auto symbols = symbolsMaybe.value();

    if (auto variableUsages = analysis::findVariableUsages(filePath, contextPath, location, symbols)) {
        return variableUsages.value();
    }


    if (auto subUsages = analysis::findSubroutineUsages(filePath, contextPath, location, symbols)) {
        return subUsages.value();
    }

    return std::unordered_map<std::string, std::vector<Range>>();
}

std::optional<FilePos> analysis::findVariableDeclaration(const std::string &filePath, FilePos location) {
    FileSymbols fileSymbols = getFileSymbols(filePath);
    return findVariableDeclaration(fileSymbols, location);
}


std::optional<analysis::Declaration>
analysis::findSubroutineDeclaration(const std::string &filePath, const std::string &contextPath, FilePos location,
                                    std::vector<std::string> projectFiles, Cache &cache) {
    auto symbolsMaybe = buildProjectSymbols(filePath, contextPath, projectFiles, cache, false, false, true);
    if (!symbolsMaybe.has_value()) {
        return {};
    }

    auto symbols = symbolsMaybe.value();
    return doFindSubroutineDeclaration(contextPath, location, symbols);
}


/**
 * Load project files (and their imports) into cache, for fast future loading
 * @param projectFiles
 * @param cache
 */
void analysis::indexProject(std::vector<std::string> projectFiles, Cache &cache) {
    for (const auto &file : projectFiles) {
        buildSymbols(file, file, cache);
    }
}

