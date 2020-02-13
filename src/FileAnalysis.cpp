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

std::optional<GlobalVariable>
analysis::findGlobalVariable(const std::string &filePath, const std::string &contextPath, FilePos location,
                             Symbols &symbols) {
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
        for (GlobalVariable &usage : globalWithFiles.second[contextPath]) {
            if (insideRange(usage.getLocation(), location)) {
                // We've found a global
                return globalVariable;
            }
        }
    }

    return {};
}


optional<vector<Range>>
analysis::findLocalVariableUsages(const std::string &filePath, const std::string &contextPath, FilePos location,
                                  Symbols &symbols) {
    auto maybeVariable = findVariableAtLocation(symbols.rootFileSymbols, location);
    if (maybeVariable.has_value()) {
        // Was a local variable
        return maybeVariable.value().usages;
    }

    return {};
}

optional<unordered_map<string, vector<GlobalVariable>>> analysis::findGlobalVariableUsages(const std::string &filePath,
                                                                                           const std::string &contextPath,
                                                                                           FilePos location,
                                                                                           Symbols &symbols) {
    auto maybeGlobal = findGlobalVariable(filePath, contextPath, location, symbols);
    if (maybeGlobal.has_value()) {
        return symbols.globalVariablesMap.globalsMap[maybeGlobal.value()];
    }
    return {};
}


optional<unordered_map<string, vector<Range>>>
analysis::findVariableUsages(const std::string &filePath, const std::string &contextPath, FilePos location,
                             Symbols &symbols) {
    if (auto localUsages = findLocalVariableUsages(filePath, contextPath, location, symbols)) {
        return unordered_map<string, vector<Range>>{{symbols.rootFilePath, localUsages.value()}};
    } else if (auto globalUsages = findGlobalVariableUsages(filePath, contextPath, location, symbols)) {
        unordered_map<string, vector<Range>> usages;
        for (auto fileWithGlobals : globalUsages.value()) {
            usages[fileWithGlobals.first] = vector<Range>();
            for (const auto &usage : fileWithGlobals.second) {
                usages[fileWithGlobals.first].emplace_back(usage.getLocation());
            }
        }

        return usages;
    }

    return {};
}


std::optional<SubroutineDecl>
analysis::doFindSubroutineDeclaration(std::string contextPath, FilePos location, Symbols &symbols) {
    auto subMap = symbols.subroutineMap.subsMap;

    for (const auto &subMapItem : subMap) {
        auto fileToRangeMap = subMapItem.second;
        if (fileToRangeMap.count(contextPath) == 0) continue;

        for (auto range : fileToRangeMap[contextPath]) {
            if (insideRange(range.location, location)) {
                // We've found the subroutine symbol, now get declaration
                return subMapItem.first;
            }
        }
    }

    return {};
}

optional<unordered_map<string, vector<SubroutineCode>>>
analysis::findSubroutineUsagesCode(const std::string &filePath, const std::string &contextPath, FilePos location,
                                   Symbols &symbols) {
    auto subMap = symbols.subroutineMap.subsMap;

    for (const auto &subMapItem : subMap) {
        auto fileToRangeMap = subMapItem.second;
        if (fileToRangeMap.count(contextPath) == 0) continue;

        for (auto range : fileToRangeMap[contextPath]) {
            if (insideRange(range.location, location)) {
                // We've found the subroutine symbol, now get usages
                return subMap[subMapItem.first];
            }
        }
    }

    return {};


}

std::optional<std::unordered_map<std::string, std::vector<Range>>>
analysis::findSubroutineUsages(const std::string &filePath, const std::string &contextPath, FilePos location,
                               Symbols &symbols) {
    unordered_map<string, vector<Range>> rangeMap;
    if (auto usages = findSubroutineUsagesCode(filePath, contextPath, location, symbols)) {
        for (auto fileUsages : usages.value()) {
            rangeMap[fileUsages.first] = vector<Range>();
            for (auto subCode : fileUsages.second) rangeMap[fileUsages.first].emplace_back(subCode.location);
        }

        return rangeMap;
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
    auto declMaybe = doFindSubroutineDeclaration(contextPath, location, symbols);
    if (!declMaybe.has_value()) return {};
    auto subDecl = declMaybe.value();
    analysis::Declaration declaration;
    declaration.path = subDecl.path;
    declaration.pos = subDecl.subroutine.location.from;
    return declaration;
}


optional<string>
analysis::getSymbolName(std::string &filePath, FilePos location, vector<string> projectFiles, Cache &cache) {
    auto symbolsMaybe = buildProjectSymbols(filePath, filePath, std::move(projectFiles), cache, true, false, true);
    if (!symbolsMaybe.has_value()) {
        return {};
    }
    auto symbols = symbolsMaybe.value();

    if (auto localVar = findVariableAtLocation(symbols.rootFileSymbols, location)) {
        return localVar.value().declaration->name;
    }

    if (auto sub = analysis::doFindSubroutineDeclaration(filePath, location, symbols)) {
        return sub.value().subroutine.code;
    }

    if (auto glob = analysis::findGlobalVariable(filePath, filePath, location, symbols)) {
        return glob.value().getCodeName();
    }

    return {};
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

analysis::RenameResult analysis::renameSymbol(const string &filePath, FilePos location, string renameTo,
                                              vector<string> projectFiles, Cache &cache) {
    auto symbolsMaybe = buildProjectSymbols(filePath, filePath, std::move(projectFiles), cache, true, false, true);
    if (!symbolsMaybe.has_value()) {
        return RenameResult(false, "Rename error");
    }
    auto symbols = symbolsMaybe.value();

    auto maybeSubDecl = analysis::doFindSubroutineDeclaration(filePath, location, symbols);
    std::string existingPackage;

    bool isSub = false;
    bool isGlobal = false;

    if (maybeSubDecl.has_value()) {
        existingPackage = maybeSubDecl.value().subroutine.package;
        isSub = true;
    } else {
        auto maybeGlobal = analysis::findGlobalVariable(filePath, filePath, location, symbols);
        if (maybeGlobal.has_value()) existingPackage = maybeGlobal.value().getPackage();
        isGlobal = true;
    }

    string packageAtLocation = findPackageAtPos(symbols.rootFileSymbols.packages, location);

    // Map of replacements from file->replacements
    unordered_map<string, vector<Replacement>> replacementMap;

    if (existingPackage.empty()) {
        // In this case we have a local variable
        // Can just to basic rename here
        auto localUsages = analysis::findLocalVariableUsages(filePath, filePath, location, symbols);
        if (localUsages.has_value()) {
            replacementMap[filePath] = vector<Replacement>();

            for (auto usage : localUsages.value()) {
                // No package to worry about, just a simple replacement
                replacementMap[filePath].emplace_back(Replacement(usage, renameTo));
            }
        }
    } else {
        // Here we have a packaged symbol - e.g. Subroutine or global variable
        // Must check packages and take care during renames
        std::string canonicalReplacement = getCanonicalPackageName(renameTo);
        // If global remove sigil
        if (!canonicalReplacement.empty() && isGlobal) {
            canonicalReplacement = canonicalReplacement.substr(1, canonicalReplacement.size() - 1);
        }
        PackagedSymbol packagedSymbol = splitOnPackage(canonicalReplacement, packageAtLocation);

        if (packagedSymbol.package != existingPackage) {
            std::cerr << "Attempted change of package on rename: " << renameTo << " from " << existingPackage
                      << " to " << packagedSymbol.package << std::endl;
            return RenameResult(false, "Rename changes symbol's package");
        }


        if (isGlobal) {
            Tokeniser tokeniser(renameTo, false);
            if (tokeniser.matchIdentifier() != renameTo) {
                return RenameResult(false, "Invalid global name");
            }

            if (auto globalUsages = analysis::findGlobalVariableUsages(filePath, filePath, location, symbols)) {
                for (auto fileGlobals : globalUsages.value()) {
                    if (isSystemPath(fileGlobals.first)) {
                        cerr << "Got system path in rename analysis, ignoring - " << fileGlobals.first << endl;
                    }

                    replacementMap[fileGlobals.first] = vector<Replacement>();
                    for (auto global : fileGlobals.second) {
                        // We know the package is the same, so only alter the new part
                        auto canonicalName = getCanonicalVariableName(global.getCodeName());
                        auto packageParts = splitPackage(canonicalName);
                        if (packageParts.empty()) {
                            cerr << "Package parts empty, something gone very wrong" << endl;
                            return RenameResult(false, "Rename error");
                        }

                        // Do the actual rename
                        packageParts[packageParts.size() - 1] = packagedSymbol.symbol;
                        auto rep = join(packageParts, "::");
                        replacementMap[fileGlobals.first].emplace_back(Replacement(global.getLocation(), rep));
                    }
                }
            }
        }

        if (isSub) {
            Tokeniser tokeniser(renameTo, false);
            if (tokeniser.matchIdentifier() != renameTo) {
                return RenameResult(false, "Invalid subroutine name");
            }

            if (auto subUsages = analysis::findSubroutineUsagesCode(filePath, filePath, location, symbols)) {
                for (auto subUsage : subUsages.value()) {
                    if (isSystemPath(subUsage.first)) {
                        cerr << "Got system path in rename analysis, ignoring - " << subUsage.first << endl;
                    }

                    replacementMap[subUsage.first] = vector<Replacement>();
                    for (auto sub : subUsage.second) {
                        // We know the package is the same, so only alter the new part
                        auto canonicalName = getCanonicalPackageName(sub.code);
                        auto packageParts = splitPackage(canonicalName);
                        if (packageParts.empty()) {
                            cerr << "Package parts empty, something gone very wrong" << endl;
                            return RenameResult(false, "Rename error");
                        }

                        // Do the actual rename
                        packageParts[packageParts.size() - 1] = packagedSymbol.symbol;
                        auto rep = join(packageParts, "::");
                        replacementMap[subUsage.first].emplace_back(Replacement(sub.location, rep));
                    }
                }
            }
        }

    }

    cout << endl;
    for (auto fileRep : replacementMap) {
        cout << fileRep.first << endl;
        for (auto rep: fileRep.second) {
            cout << "\t " << rep.location.toStr() << " -> " << rep.replacement << endl;
        }
    }

    // Now do the actual renaming
    for (auto fileReplacement : replacementMap) {
        if (isSystemPath(fileReplacement.first)) continue;
        auto fileContents = readFile(fileReplacement.first);
        auto newFile = doReplacements(fileContents, fileReplacement.second);
        writeFile(fileReplacement.first, newFile);
    }

    return RenameResult(true, "");
}

analysis::RenameResult::RenameResult(bool success, const string &error) : success(success), error(error) {}
