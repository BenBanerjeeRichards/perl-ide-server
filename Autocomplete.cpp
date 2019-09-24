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

    auto parseTree = parse(tokens);
    auto packages = parsePackages(parseTree);
//    auto variables = findVariableDeclarations(parseTree, packages);
//
//    std::vector<AutocompleteItem> completion;
//    completion.reserve(variables.size());
//
//    for (auto &var: variables) {
//        if (var->isAccessibleAt(location)) {
//            completion.emplace_back(AutocompleteItem(var->name, var->getDetail()));
//        }
//    }

//    return completion;
}

AutocompleteItem::AutocompleteItem(const std::string &name, const std::string &detail) : name(name), detail(detail) {}
