//
// Created by Ben Banerjee-Richards on 2019-10-15.
//

#include "Token.h"

bool Token::isWhitespaceNewlineOrComment() {
    return type == TokenType::Whitespace || type == TokenType::Newline || type == TokenType::Comment;
}

Token::Token(const TokenType &type, FilePos start, int endCol, const std::string &data) {
    this->type = type;
    this->data = data;
    this->startPos = start;
    this->endPos = FilePos(start.line, endCol, startPos.position + (endCol - start.col));
}

Token::Token(const TokenType &type, FilePos start, FilePos end, const std::string &data) {
    this->type = type;
    this->data = data;
    this->startPos = start;

    if (end.position == -1) end.position = startPos.position + data.size() - 1;
    this->endPos = end;
}

Token::Token(const TokenType &type, FilePos start, const std::string &data) {
    this->type = type;
    this->data = data;
    this->startPos = start;

    if (data.size() > 0) {
        this->endPos = FilePos(start.line, start.col + (int) data.size() - 1, startPos.position + data.size() - 1);
    } else {
        this->endPos = FilePos(start.line, start.col, start.position);
    }
}

int TokenIterator::getIndex() { return i; }

TokenIterator::TokenIterator(const std::vector<Token> &tokens, std::vector<TokenType> ignoreTokens, int offset)
        : tokens(tokens) {
    this->ignoreTokens = ignoreTokens;
    this->i = offset;
}

TokenIterator::TokenIterator(const std::vector<Token> &tokens, std::vector<TokenType> ignoreTokens) : tokens(tokens) {
    this->ignoreTokens = ignoreTokens;
}


