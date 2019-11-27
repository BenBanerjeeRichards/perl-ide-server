//
// Created by Ben Banerjee-Richards on 2019-09-19.
//

#include "Autocomplete.h"

std::vector<AutocompleteItem> autocompleteVariables(const std::string &filePath, FilePos location, char sigilContext) {
    Tokeniser tokeniser(readFile(filePath));
    std::vector<Token> tokens = tokeniser.tokenise();
    FileSymbols fileSymbols;
    int partial = -1;
    auto parseTree = parse(tokens, partial);
    fileSymbols.packages = parsePackages(parseTree);
    buildVariableSymbolTree(parseTree, fileSymbols);
    std::vector<AutocompleteItem> completion;

    return variableNamesAtPos(fileSymbols, location, sigilContext);
}

std::vector<AutocompleteItem> autocompleteSubs(const std::string &filePath, FilePos location) {
    Tokeniser tokeniser(readFile(filePath));
    std::vector<Token> tokens = tokeniser.tokenise();
    FileSymbols fileSymbols;
    int partial = -1;
    auto parseTree = parse(tokens, partial);
    fileSymbols.packages = parsePackages(parseTree);
    buildVariableSymbolTree(parseTree, fileSymbols);
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