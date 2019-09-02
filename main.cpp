#include <iostream>
#include "Tokeniser.h"
#include "search/Search.h"


int main(int argc, char **args) {
    if (argc == 2) {
        Tokeniser tokeniser(readFile(args[1]));
        auto token = tokeniser.nextToken();

        while (token.type != TokenType::EndOfInput) {
            std::cout << token.toStr(true) << std::endl;
            token = tokeniser.nextToken();
        }

        return 0;
    }

    std::ifstream fileStream("../perl/variables.pl");
    if (!fileStream.is_open()) {
        std::cerr << "Failed to open file!" << std::endl;
        return 1;
    }
    std::string program((std::istreambuf_iterator<char>(fileStream)), (std::istreambuf_iterator<char>()));
    Tokeniser tokeniser(program);
    auto token = tokeniser.nextToken();

    while (token.type != TokenType::EndOfInput) {
        std::cout << token.toStr(false) << std::endl;
        token = tokeniser.nextToken();
    }

    return 0;
}