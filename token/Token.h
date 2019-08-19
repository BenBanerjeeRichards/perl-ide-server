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




#endif //PERLPARSER_TOKEN_H
