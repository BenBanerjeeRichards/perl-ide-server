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
analysis::autocompleteVariables(const std::string &filePath, FilePos location, char sigilContext) {
    FileSymbols fileSymbols = getFileSymbols(filePath);
    return variableNamesAtPos(fileSymbols, location, sigilContext);
}

std::vector<AutocompleteItem> analysis::autocompleteSubs(const std::string &filePath, FilePos location) {
    FileSymbols fileSymbols = getFileSymbols(filePath);
    std::string currentPackage = findPackageAtPos(fileSymbols.packages, location);
    std::vector<AutocompleteItem> completions;

    for (const auto &sub : fileSymbols.subroutines) {
        if (sub.package == currentPackage) {
            completions.emplace_back(AutocompleteItem(sub.name, sub.name + "()"));
        } else {
            completions.emplace_back(AutocompleteItem(sub.package + "::" + sub.name, sub.name + "()"));
        }
    }

    return completions;
}

std::unordered_map<std::string, std::vector<Range>>
analysis::findVariableUsages(const std::string &filePath, std::string contextPath, FilePos location) {
    std::unordered_map<std::string, std::vector<Range>> usages;
    auto symbolsMaybe = buildSymbols(filePath, contextPath);
    if (!symbolsMaybe.has_value()) {
        return usages;
    }

    auto symbols = symbolsMaybe.value();

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
            if (globalWithFiles.second.count(filePath) == 0) {
                continue;
            }

            // Now check every usage to see if location is within usage
            for (Range &usage : globalWithFiles.second[filePath]) {
                if (insideRange(usage, location)) {
                    // We've found a global
                    return globals[globalVariable];
                }
            }
        }
    }

    return usages;
}

std::optional<FilePos> analysis::findVariableDeclaration(const std::string &filePath, FilePos location) {
    FileSymbols fileSymbols = getFileSymbols(filePath);
    return findVariableDeclaration(fileSymbols, location);
}


analysis::SymbolUsage::SymbolUsage(int line, int col, const std::string &sourceLine) : line(line), col(col),
                                                                                       sourceLine(sourceLine) {}
