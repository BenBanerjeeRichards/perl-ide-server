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

std::vector<FilePos> analysis::findVariableUsages(const std::string &filePath, FilePos location) {
    FileSymbols fileSymbols = getFileSymbols(filePath);
    return findVariableUsages(fileSymbols, location);
}

std::optional<FilePos> analysis::findVariableDeclaration(const std::string &filePath, FilePos location) {
    FileSymbols fileSymbols = getFileSymbols(filePath);
    return findVariableDeclaration(fileSymbols, location);
}


analysis::SymbolUsage::SymbolUsage(int line, int col, const std::string &sourceLine) : line(line), col(col),
                                                                                       sourceLine(sourceLine) {}
