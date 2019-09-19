//
// Created by Ben Banerjee-Richards on 2019-09-19.
//

#include "Autocomplete.h"

std::vector<std::string> autocomplete(std::string filePath, FilePos location) {
    std::vector<Token> tokens;
    Tokeniser tokeniser(readFile(filePath));
    auto token = tokeniser.nextToken();

    while (token.type != TokenType::EndOfInput) {
        tokens.emplace_back(token);
        token = tokeniser.nextToken();
    }

    auto parseTree = parse(tokens);
    auto variables = findVariableDeclarations(parseTree);

    std::vector<std::string> completion;
    completion.reserve(variables.size());

    for (auto &var: variables) {
        if (var.isAccessibleAt(location)) {
            completion.emplace_back(var.name);
        }
    }

    return completion;
}
