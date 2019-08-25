//
// Created by Ben Banerjee-Richards on 2019-08-19.
//

#ifndef PERLPARSER_TOKENISER_H
#define PERLPARSER_TOKENISER_H

#include <string>
#include <functional>
#include "Token.h"
#include <memory>
#include <vector>
#include "TokeniseException.h"

class Tokeniser {
public:
    explicit Tokeniser(std::string programStream);

    Token nextToken();

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

    bool matchKeyword(const std::string& keyword);

    static bool isVariableBody(char c);

    std::string getUntil(const std::function<bool(char)> &nextCharTest);

    // options should be sorted longest to shortest and in preference of match
    std::string matchString(const std::vector<std::string>& options);


    std::string program;
    int position = -1;
};


#endif //PERLPARSER_TOKENISER_H
