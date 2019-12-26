//
// Created by Ben Banerjee-Richards on 2019-11-01.
//

#include "Test.h"


bool runTest(std::string &testFile) {
    auto testName = fileName(testFile);
    std::string perlFile = "../test/pl/" + testName + ".pl";
    std::string perlFileContents;
    try {
        perlFileContents = readFile(perlFile);
    } catch (IOException &ex) {
        std::cout << console::red << console::bold << "[" << testName
                  << "] FAILED - Could not open perl file with path " << perlFile << console::clear << std::endl;
        return false;
    }

    // Load tokens from test file
    std::string line;
    std::ifstream testFileStream(testFile);
    std::vector<std::string> expectedTokens;
    while (std::getline(testFileStream, line)) {
        if (!line.empty()) expectedTokens.emplace_back(line);
    }


    // Run tokeniser
    Tokeniser tokeniser(readFile(perlFile));
    auto tokensWithSpaces = tokeniser.tokenise();
    std::vector<Token> tokens;
    for (const auto &token : tokensWithSpaces) {
        if (token.type != TokenType::Whitespace && token.type != TokenType::Newline) tokens.emplace_back(token);
    }

    // Of course if they are of different size then test will fail, but let it run so we can find out
    // location of failure
    int numTokens = tokens.size() < expectedTokens.size() ? tokens.size() : expectedTokens.size();

    for (int i = 0; i < numTokens; i++) {
        std::string actualTokenString = tokens[i].toStr(false);
        if (expectedTokens[i] != actualTokenString) {
            std::cout << console::bold << console::red << "[" << testName << "] FAILED - Mismatched token at "
                      << tokens[i].endPos.toStr() << " expected row = " << i + 1 << console::clear << console::red
                      << std::endl;

            std::cout << "\tExpected: " << expectedTokens[i] << std::endl << "\tActual:  " << actualTokenString
                      << console::clear << std::endl;
            return false;
        }
    }

    // Now check number of tokens
    if (tokens.size() != expectedTokens.size()) {
        std::cout << console::bold << console::red << "[" << testName
                  << "] FAILED - Token count mismatch. |Expected| = " << expectedTokens.size() << " |Actual| = "
                  << tokens.size() << console::clear << std::endl;
        return false;
    }

    std::cout << "[" << testName << "] passed" << std::endl;
    return true;
}

void runTests() {
    auto tokenFiles = globglob("../test/expected/*");
    int total = tokenFiles.size();
    int success = 0;

    for (auto &testFile : tokenFiles) {
        if (runTest(testFile)) {
            success++;
        }
    }

    std::cout << std::endl << console::clear << console::bold;
    if (success == total) {
        std::cout << "All tests passed!";
    } else {
        std::cout << success << "/" << total << " tests passed";
    }

    std::cout << console::clear << std::endl;
}

void makeTest(std::string &name) {
    std::string perlContents;
    try {
        perlContents = readFile("../test/pl/" + name + ".pl");
    } catch (IOException &ex) {
        std::cerr << "Failed to open perl file" << std::endl;
    }

    std::fstream tokenFile;
    tokenFile.open("../test/expected/" + name + ".txt", std::ios::out);

    Tokeniser tokeniser(perlContents);
    auto tokens = tokeniser.tokenise();
    TokenIterator tokenIterator(tokens,
                                std::vector<TokenType>{TokenType::Whitespace, TokenType::Newline});

    Token token(TokenType::Newline, FilePos(0, 0, 0));
    do {
        token = tokenIterator.next();

        tokenFile << token.toStr(false) << std::endl;
    } while (token.type != TokenType::EndOfInput);
}
