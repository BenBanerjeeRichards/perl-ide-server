//
// Created by Ben Banerjee-Richards on 2019-08-28.
//


#include <iostream>
#include <vector>
#include "Util.h"
#include "Tokeniser.h"
#include <fstream>


int main(int argc, char *argv[]) {
    auto tokenFiles = globglob("../token/*");
    for (const auto &testFile : tokenFiles) {
        auto testName = fileName(testFile);
        std::string perlFile = "../perl/" + testName + ".pl";
        std::cout << testName;

        // Load tokens from test file
        std::string line;
        std::ifstream testFileStream(testFile);
        std::vector<std::string> expectedTokens;
        while (std::getline(testFileStream, line)) expectedTokens.emplace_back(line);

        // Run tokeniser
        Tokeniser tokeniser(readFile(perlFile));

        // Now check each token
        int i = 0;
        bool success = true;
        for (const auto &expectedToken : expectedTokens) {
            Token token = tokeniser.tokenise();

            if (token.toStr() != expectedToken) {
                std::cout << " [FAILED]: Incorrect token at token number " << i + 1  << " around position " << token.startPos.toStr() <<  " Got " << token.toStr()
                          << " expected " << expectedToken << std::endl;
                success = false;
                break;
            }

            if (token.type == TokenType::EndOfInput && i < expectedToken.length() - 1) {
                std::cout << " [FAILED]: Tokeniser reached EndOfInput before expected at position " << i << std::endl;
                success = false;
                break;
            }

            i++;
        }

        if (success) std::cout << " [PASS]" << std::endl;

    }
    return 0;
}