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

protected:
    // Readable name used in tostring
    std::string type;
    // Optional data used. e.g. $ident has 'ident' as it's data, but keyword my has no data
    std::string data;
};

class KeywordToken : public Token {

public:
    explicit KeywordToken(const std::string &keyword) {
        this->type = "KEYWORD";
        this->data = keyword;
    }
};

class StringToken : public Token {

public:
    explicit StringToken(const std::string &contents) {
        this->type = "STRING";
        this->data = contents;
    }
};

// Dollar variable such as $thing
class ScalarVariableToken : public Token {

public:
    explicit ScalarVariableToken(const std::string &name) {
        this->type = "$SCALAR";
        this->data = name;
    }
};

class ArrayVariableToken : public Token {

public:
    explicit ArrayVariableToken(const std::string &name) {
        this->type = "@ARRAY";
        this->data = name;
    }
};

class HashVariableToken : public Token {

public:
    explicit HashVariableToken(const std::string &name) {
        this->type = "%HASH";
        this->data = name;
    }
};


// Function calls, constant names etc..
// We need more context to figure out what it is
class WordToken : public Token {
public:
    explicit WordToken(const std::string &contents) {
        this->type = "WORD";
        this->data = contents;
    }
};

// Operators
// For our use case I don't care much about the operators so we just use a single class
// with the data as the operator (e.g. we don't have AddOperator, SubtractOperator, ...)
// This may change in the future
class OperatorToken : public Token {
    explicit OperatorToken(const std::string &token) {
        this->type = "OP";
        this->data = token;
    }
};

// {
class LBracketToken : public Token {
    explicit LBracketToken() {
        this->type = "LBRACKET";
    }
};

// }
class RBracketToken : public Token {
    explicit RBracketToken() {
        this->type = "RBRACKET";
    }
};

// (
class LParenToken : public Token {
    explicit LParenToken() {
        this->type = "LPAREN";
    }
};

// )
class RParenToken : public Token {
    explicit RParenToken() {
        this->type = "RPAREN";
    }
};

class LSquareBracketToken : public Token {
    explicit LSquareBracketToken() {
        this->type = "LSQUARE";
    }
};

class RSquareBracketToken : public Token {
    explicit RSquareBracketToken() {
        this->type = "RSQUARE";
    }
};


#endif //PERLPARSER_TOKEN_H
