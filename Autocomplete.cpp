//
// Created by Ben Banerjee-Richards on 2019-09-19.
//

#include "Autocomplete.h"

std::vector<AutocompleteItem> autocomplete(const std::string& filePath, FilePos location) {
    std::vector<Token> tokens;
    Tokeniser tokeniser(readFile(filePath));
    auto token = tokeniser.nextToken();

    while (token.type != TokenType::EndOfInput) {
        tokens.emplace_back(token);
        token = tokeniser.nextToken();
    }
    FileSymbols fileSymbols;
    auto parseTree = parse(tokens);
    fileSymbols.packages = parsePackages(parseTree);
     buildVariableSymbolTree(parseTree, fileSymbols);
    auto symbolTable = getSymbolMap(fileSymbols.symbolTree, location);
    std::vector<AutocompleteItem> completion;

    completion.reserve(symbolTable.size());
    for (const auto& var : symbolTable) {
        completion.emplace_back(AutocompleteItem(var.second->name, var.second->getDetail()));
    }

    return completion;
}

AutocompleteItem::AutocompleteItem(const std::string &name, const std::string &detail) : name(name), detail(detail) {}
