//
// Created by Ben Banerjee-Richards on 2019-08-19.
//

#ifndef PERLPARSER_TOKENISER_H
#define PERLPARSER_TOKENISER_H

#include <string>
#include <functional>
#include "token/Token.h"
#include <memory>

class Tokeniser {
public:
    explicit Tokeniser(std::string programStream);
    std::unique_ptr<Token> nextToken();
private:
    char nextChar();

    char peek();

    char peekAhead(int i);

    bool isEof();

    // Whitespace such as tabs, empty spaces. Does NOT include new lines
    static bool isWhitespace(char c);
    static bool isNewline(char c);
    static bool isLowercase(char c);
    static bool isUppercase(char c);
    static bool isNumber(char c);
    static bool isVariableBody(char c);

    std::string getUntil(const std::function<bool(char)>& nextCharTest);


    std::string program;
    int position = 0;
};


#endif //PERLPARSER_TOKENISER_H
