#include <iostream>
#include "Tokeniser.h"

void printFileTokens(const std::string& file, bool includeLocation) {
    Tokeniser tokeniser(readFile(file));
    auto token = tokeniser.nextToken();

    while (token.type != TokenType::EndOfInput) {
        std::cout << token.toStr(includeLocation) << std::endl;
        token = tokeniser.nextToken();
    }
}

int main(int argc, char **args) {
    std::string file = "../perl/variables.pl";

    if (argc == 2) file = args[1];
    printFileTokens(file, argc == 2);
    return 0;
}