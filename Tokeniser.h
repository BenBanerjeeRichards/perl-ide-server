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
    Comma
};

std::string tokenToString(const TokenType &t);

class Token {

public:
    std::string toString() {
        if (this->data.empty()) {
            return tokenToString(this->type);
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


        return tokenToString(this->type) + "(" + dataToShow + ")";
    }

    // When data is identical to code
    Token(const TokenType &type, const std::string &data, unsigned int line, unsigned int startCol) {
        this->type = type;
        this->data = data;
        this->startLine = line;
        this->startCol = startCol;

        // Compute ending position from data
        this->endLine = line;
        this->endCol = startCol + data.size();
    }

    Token(const TokenType &type, unsigned int line, unsigned int startCol) {
        this->type = type;
        this->startLine = line;
        this->startCol = startCol;
        this->data = "";

        // Compute ending position from data
        this->endLine = line;
        this->endCol = startCol + data.size();
    }


    Token(const TokenType &type, const std::string &data, unsigned int line, unsigned int startCol,
          unsigned int endCol) {
        this->type = type;
        this->data = data;
        this->startLine = line;
        this->startCol = startCol;

        // Compute ending position from data
        this->endLine = line;
        this->endCol = endCol;
    }

    Token(const TokenType &type, const std::string &data, unsigned int startLine, unsigned int startCol,
          unsigned int endLine, unsigned int endCol) {
        this->type = type;
        this->data = data;
        this->startLine = startLine;
        this->startCol = startCol;

        // Compute ending position from data
        this->endLine = endLine;
        this->endCol = endCol;
    }

    TokenType type;

private:
    // Readable name used in tostring
    // Optional data used. e.g. $ident has 'ident' as it's data, but keyword my has no data
    std::string data;

    // Position of token in file
    unsigned int startLine;
    unsigned int startCol;
    unsigned int endLine;
    unsigned int endCol;
};

struct FilePos {
    FilePos(int line, int pos) {
        this->line = line;
        this->pos = pos;
    }
    int line;
    int pos;
};


class Tokeniser {
public:
    explicit Tokeniser(std::string programStream);

    Token nextToken();

// Place where newlines start
std::vector<int> lineStartPositions{0};
private:
    char nextChar();

    char peek();

    char peekAhead(int i);

    bool isEof();

    char prevChar(int i);

    // Whitespace such as tabs, empty spaces. Does NOT include new lines
    static bool isWhitespace(char c);

    static bool isNewline(char c);

    static bool isLowercase(char c);

    static bool isUppercase(char c);

    static bool isNumber(char c);

    static bool isAlphaNumeric(char c);

    bool matchKeyword(const std::string &keyword);

    static bool isVariableBody(char c);

    std::string getUntil(const std::function<bool(char)> &nextCharTest);

    // options should be sorted longest to shortest and in preference of match
    std::string matchString(const std::vector<std::string> &options);

    // Match some perl 'name' - could be a function name, function call, etc... We just don't know yet
    std::string matchName();

    std::string matchString();

    std::string matchNumeric();

    std::string matchComment();

    std::string matchPod();

    FilePos getLineCol(int pos);

    std::string program;
    int position = -1;
};

#endif //PERLPARSER_TOKENISER_H
