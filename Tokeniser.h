//
// Created by Ben Banerjee-Richards on 2019-08-19.
//

#ifndef PERLPARSER_TOKENISER_H
#define PERLPARSER_TOKENISER_H

#include <string>
#include <functional>
#include <memory>
#include <vector>
#include <regex>
#include "TokeniseException.h"
#include <utility>
#include <iostream>
#include "FilePos.h"
#include "Util.h"



enum TokenType {
    String,
    ScalarVariable,
    ArrayVariable,
    HashVariable,
    Operator,
    LBracket,
    RBracket,
    LParen,
    RParen,
    LSquareBracket,
    RSquareBracket,
    Comment,
    Newline,
    Whitespace,
    Dot,
    Assignment,
    Semicolon,
    EndOfInput,
    If,
    Else,
    ElseIf,
    Unless,
    While,
    Until,
    For,
    Foreach,
    When,
    Do,
    Package,
    Next,
    Redo,
    Last,
    My,
    State,
    Our,
    Break,
    Continue,
    Given,
    Use,
    Sub,
    Name,
    NumericLiteral,
    Pod,
    Comma,
    Local
};

std::string tokenTypeToString(const TokenType &t);


class Token {

public:
    std::string toString() {
        if (this->data.empty()) {
            return tokenTypeToString(this->type);
        }

        // Don't print out newlines to console
        std::string dataToShow = this->data;

        // TODO move this into until function
        while (dataToShow.find("\n") != std::string::npos) {
            dataToShow.replace(dataToShow.find("\n"), 1, "\\n");
        }

        while (dataToShow.find("\t") != std::string::npos) {
            dataToShow.replace(dataToShow.find("\t"), 1, "\\t");
        }


        return tokenTypeToString(this->type) + "(" + dataToShow + ")";
    }

    // When data is identical to code
    Token(const TokenType &type, FilePos start, const std::string &data = "") {
        this->type = type;
        this->data = data;
        this->startPos = start;
        this->endPos = FilePos(start.line, start.col + (int) data.size() - 1);
    }


    Token(const TokenType &type, FilePos start, int endCol, const std::string &data = "") {
        this->type = type;
        this->data = data;
        this->startPos = start;
        this->endPos = FilePos(start.line, endCol);
    }

    Token(const TokenType &type, FilePos start, FilePos end, const std::string &data = "") {
        this->type = type;
        this->data = data;
        this->startPos = start;
        this->endPos = end;
    }

    std::string toStr(bool includeLocation = false);

    bool isWhitespaceNewlineOrComment();

    TokenType type;
    // Position of token in file
    FilePos startPos;
    FilePos endPos;
    // Readable name used in tostring
    // Optional data used. e.g. $ident has 'ident' as it's data, but keyword my has no data
    std::string data;



private:

};

class Tokeniser {
public:
    explicit Tokeniser(std::string programStream);

    Token nextToken();

private:
    char nextChar();

    char peek();

    char peekAhead(int i);

    bool isEof();

    std::string substring(int fromIdx, int length);

    char prevChar(int i);

    // Whitespace such as tabs, empty spaces. Does NOT include new lines
    static bool isWhitespace(char c);

    static bool isNewline(char c);

    static bool isLowercase(char c);

    static bool isUppercase(char c);

    static bool isNumber(char c);

    static bool isAlphaNumeric(char c);

    static bool isPunctuation(char c);

    bool matchKeyword(const std::string &keyword);

    static bool isNameBody(char c);

    std::string getWhile(const std::function<bool(char)> &nextCharTest);

    // options should be sorted longest to shortest and in preference of match
    std::string matchString(const std::vector<std::string> &options, bool requireTrailingNonAN = false);

    // Match some perl 'name' - could be a function name, function call, etc... We just don't know yet
    std::string matchName();

    std::string matchString();

    std::string matchNumeric();

    std::string matchComment();

    std::string matchPod();

    std::string matchVariable();

    int peekPackageTokens(int i);

    int nextLine();

    void advancePositionSameLine(int i);

    int _position = -1;
    int currentLine = 1;
    int currentCol = 1;
    std::string program;


};

#endif //PERLPARSER_TOKENISER_H
