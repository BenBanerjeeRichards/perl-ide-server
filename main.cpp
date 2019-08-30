#include <iostream>
#include "Tokeniser.h"
#include "search/Search.h"


void printTokenStandard(const Token& token, bool printLocation = true) {
    std::string tokenStr = tokenTypeToString(token.type);
    if (!token.data.empty()) {
        auto d1 = replace(token.data, "\n", "\\n");
        auto d2 = replace(d1, "\r", "\\r");
        tokenStr += "(" + d2 + ")";
    }

    if (printLocation) {
        std::cout << token.startPos.line << ":" << token.startPos.col << " " << token.endPos.line << ":"
                  << token.endPos.col << " ";
    }

    std::cout << tokenStr << std::endl;
}

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

    std::ifstream fileStream("../perl/test.pl");
    if (!fileStream.is_open()) {
        std::cerr << "Failed to open file!" << std::endl;
        return 1;
    }
    std::string program((std::istreambuf_iterator<char>(fileStream)), (std::istreambuf_iterator<char>()));
    Tokeniser tokeniser(program);
    auto token = tokeniser.nextToken();

    while (token.type != TokenType::EndOfInput) {
        std::cout << token.toStr(true) << std::endl;
        token = tokeniser.nextToken();
    }

    return 0;
}