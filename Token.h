//
// Created by Ben Banerjee-Richards on 2019-10-15.
//

#ifndef PERLPARSER_TOKEN_H
#define PERLPARSER_TOKEN_H

#include <vector>
#include "FilePos.h"
#include "Util.h"

enum class TokenType {
    String,
    StringStart,
    StringEnd,
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
    ElsIf,
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
    Local,
    Prototype,
    Signature,
    SubName,
    Attribute,
    AttributeArgs,
    AttributeColon,
    HereDoc,
    HereDocEnd,
};

class Token {

public:
    // When data is identical to code
    Token(const TokenType &type, FilePos start, const std::string &data = "");

    Token(const TokenType &type, FilePos start, int endCol, const std::string &data = "");

    Token(const TokenType &type, FilePos start, FilePos end, const std::string &data = "");

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



// TODO make this an actual iterator
class TokenIterator {
public:
    TokenIterator(const std::vector<Token> &tokens, std::vector<TokenType> ignoreTokens);

    TokenIterator(const std::vector<Token> &tokens, std::vector<TokenType> ignoreTokens, int offset);

    Token next();

    int getIndex();

private:
    const std::vector<Token> &tokens;
    std::vector<TokenType> ignoreTokens;
    int i;
};


std::string tokenTypeToString(const TokenType &t);


#endif //PERLPARSER_TOKEN_H
