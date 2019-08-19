//
// Created by Ben Banerjee-Richards on 2019-08-19.
//

#ifndef PERLPARSER_TOKEN_H
#define PERLPARSER_TOKEN_H


#include <string>

class Token {

public:
    virtual std::string toString() {
        if (this->data.empty()) {
            return this->type;
        }

        return this->type + "(" + this->data + ")";
    }

    // When data is identical to code
    Token(const std::string &type, const std::string &data, unsigned int line, unsigned int startCol) {
        this->type = type;
        this->data = data;
        this->startLine = line;
        this->startCol = startCol;

        // Compute ending position from data
        this->endLine = line;
        this->endCol = startCol + data.size();
    }

    Token(const std::string &type, const std::string &data, unsigned int line, unsigned int startCol, unsigned int endCol) {
        this->type = type;
        this->data = data;
        this->startLine = line;
        this->startCol = startCol;

        // Compute ending position from data
        this->endLine = line;
        this->endCol = endCol;
    }


protected:
    // Readable name used in tostring
    std::string type;
    // Optional data used. e.g. $ident has 'ident' as it's data, but keyword my has no data
    std::string data;

    // Position of token in file
    unsigned int startLine;
    unsigned int startCol;
    unsigned int endLine;
    unsigned int endCol;
};

class KeywordToken : public Token {

public:
    explicit KeywordToken(const std::string &keyword, int startLine, int startCol)
            : Token("KEYWORD", keyword, startLine, startCol) {}
};

class StringToken : public Token {

public:
    explicit StringToken(const std::string &contents, int startLine, int startCol)
            : Token("STRING", contents, startLine, startCol) {}
};

// Dollar variable such as $thing
class ScalarVariableToken : public Token {

public:
    explicit ScalarVariableToken(const std::string &name, int startLine, int startCol)
            : Token("$SCALAR", name, startLine, startCol, startCol + 1) {}
};

class ArrayVariableToken : public Token {

public:
    explicit ArrayVariableToken(const std::string &name, int startLine, int startCol)
            : Token("@ARRAY", name, startLine, startCol, startCol + 1) {}
};

class HashVariableToken : public Token {

public:
    explicit HashVariableToken(const std::string &name, int startLine, int startCol)
            : Token("%ARRAY", name, startLine, startCol, startCol + 1) {}
};


// Function calls, constant names etc..
// We need more context to figure out what it is
class WordToken : public Token {
public:
    explicit WordToken(const std::string &word, int startLine, int startCol)
            : Token("WORD", word, startLine, startCol) {}
};

// Operators
// For our use case I don't care much about the operators so we just use a single class
// with the data as the operator (e.g. we don't have AddOperator, SubtractOperator, ...)
// This may change in the future
class OperatorToken : public Token {
    explicit OperatorToken(const std::string &op, int startLine, int startCol)
            : Token("OP", op, startLine, startCol) {}
};

// {
class LBracketToken : public Token {
    explicit LBracketToken(int startLine, int startCol)
            : Token("LBRACKET", "", startLine, startCol, startCol + 1) {}
};

// }
class RBracketToken : public Token {
    explicit RBracketToken(int startLine, int startCol)
            : Token("RBRACKET", "", startLine, startCol, startCol + 1) {}
};

// (
class LParenToken : public Token {
    explicit LParenToken(int startLine, int startCol)
            : Token("LPAREN", "", startLine, startCol, startCol + 1) {}
};

// )
class RParenToken : public Token {
    explicit RParenToken(int startLine, int startCol)
            : Token("RPAREN", "", startLine, startCol, startCol + 1) {}
};

class LSquareBracketToken : public Token {
    explicit LSquareBracketToken(int startLine, int startCol)
            : Token("RPAREN", "", startLine, startCol, startCol + 1) {}
};

class RSquareBracketToken : public Token {
    explicit RSquareBracketToken(int startLine, int startCol)
            : Token("RPAREN", "", startLine, startCol, startCol + 1) {}
};

#endif //PERLPARSER_TOKEN_H
