//
// Created by Ben Banerjee-Richards on 2019-09-19.
//

#include "Autocomplete.h"

std::vector<AutocompleteItem> autocomplete(const std::string& filePath, FilePos location) {
    Tokeniser tokeniser(readFile(filePath));
    std::vector<Token> tokens = tokeniser.tokenise();

    FileSymbols fileSymbols;
    int partial = -1;
    auto parseTree = parse(tokens, partial);
    fileSymbols.packages = parsePackages(parseTree);
    buildVariableSymbolTree(parseTree, fileSymbols);
    std::vector<AutocompleteItem> completion;
    return variableNamesAtPos(fileSymbols, location);
}

