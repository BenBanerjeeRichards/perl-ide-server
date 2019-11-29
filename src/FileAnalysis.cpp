//
// Created by Ben Banerjee-Richards on 2019-11-28.
//

#include "FileAnalysis.h"

FileSymbols analysis::getFileSymbols(const std::string &path) {
    auto program = readFile(path);
    Tokeniser tokeniser(program);
    std::vector<Token> tokens = tokeniser.tokenise();
    FileSymbols fileSymbols;

    FilePos currLineStart = FilePos(1, 1, 0);
    for (const auto &token : tokens) {
        if (token.type == TokenType::Newline) {
            int startPos = currLineStart.position + 1;
            int endPos = (token.endPos.position - currLineStart.position) - 1;
            auto line = program.substr(startPos, endPos);
            fileSymbols.sourceCode.emplace_back(line);
            currLineStart = token.endPos;
        }
    }

    int partial = -1;
    auto parseTree = parse(tokens, partial);
    fileSymbols.packages = parsePackages(parseTree);
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
    return variable::findVariableUsages(fileSymbols, location);
}

analysis::SymbolUsage::SymbolUsage(int line, int col, const std::string &sourceLine) : line(line), col(col),
                                                                                       sourceLine(sourceLine) {}
