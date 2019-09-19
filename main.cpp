#include <iostream>
#include "Tokeniser.h"
#include "Parser.h"
#include "VarAnalysis.h"

void printFileTokens(const std::string& file, bool includeLocation) {
    std::vector<Token> tokens;
    Tokeniser tokeniser(readFile(file));
    auto token = tokeniser.nextToken();

    while (token.type != TokenType::EndOfInput) {
        tokens.emplace_back(token);
        std::cout << token.toStr(includeLocation) << std::endl;
        token = tokeniser.nextToken();
    }

    auto parseTree = parse(tokens);
    printParseTree(parseTree);
    auto variables = findVariableDeclarations(parseTree);
    for (auto& var : variables) {
        std::cout << var.name << " "<< var.declaration.toStr() <<  std::endl;
    }
    std::cout << "DONE" << std::endl;
}

int main(int argc, char **args) {
    std::string file = "../perl/input.pl";

    if (argc == 2) file = args[1];
    printFileTokens(file, argc == 2);
    return 0;
}