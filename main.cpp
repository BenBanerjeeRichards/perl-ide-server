#include <iostream>
#include "Token.h"
#include "Tokeniser.h"
#include <fstream>

int main() {
    std::ifstream fileStream("../test.pl");
    if (!fileStream.is_open()) {
        std::cerr << "Failed to open file!" << std::endl;
        return 1;
    }
    std::string program((std::istreambuf_iterator<char>(fileStream) ), (std::istreambuf_iterator<char>()    ) );
    Tokeniser tokeniser(program);
    auto token = tokeniser.nextToken();
    while (true) {
        std::cout << token.toString() << std::endl;
        token = tokeniser.nextToken();
    }

    std::cout << "DONE" << std::endl;
    return 0;
}