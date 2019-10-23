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

std::string Token::toStr(bool includeLocation) {
    std::string tokenStr;

    if (includeLocation) {
        tokenStr += this->startPos.toStr() + " " + this->endPos.toStr() + " ";
    }

    tokenStr += tokenTypeToString(this->type);

    if (!this->data.empty()) {
        auto d1 = replace(this->data, "\n", "\\n");
        auto d2 = replace(d1, "\r", "\\r");
        tokenStr += "(" + d2 + ")";
    }

    return tokenStr;
}

std::string tokenTypeToString(const TokenType &t) {
    if (t == TokenType::String) return "String";
    if (t == TokenType::ScalarVariable) return "ScalarVariable";
    if (t == TokenType::ArrayVariable) return "ArrayVariable";
    if (t == TokenType::HashVariable) return "HashVariable";
    if (t == TokenType::Operator) return "Operator";
    if (t == TokenType::LBracket) return "LBracket";
    if (t == TokenType::RBracket) return "RBracket";
    if (t == TokenType::LParen) return "LParen";
    if (t == TokenType::RParen) return "RParen";
    if (t == TokenType::LSquareBracket) return "LSquareBracket";
    if (t == TokenType::RSquareBracket) return "RSquareBracket";
    if (t == TokenType::Comment) return "Comment";
    if (t == TokenType::Newline) return "Newline";
    if (t == TokenType::Whitespace) return "Whitespace";
    if (t == TokenType::Dot) return "Dot";
    if (t == TokenType::Assignment) return "Assignment";
    if (t == TokenType::Semicolon) return "Semicolon";
    if (t == TokenType::EndOfInput) return "EndOfInput";
    if (t == TokenType::If) return "If";
    if (t == TokenType::Else) return "Else";
    if (t == TokenType::ElsIf) return "ElseIf";
    if (t == TokenType::Unless) return "Unless";
    if (t == TokenType::While) return "While";
    if (t == TokenType::Until) return "Until";
    if (t == TokenType::For) return "For";
    if (t == TokenType::Foreach) return "Foreach";
    if (t == TokenType::When) return "When";
    if (t == TokenType::Do) return "Do";
    if (t == TokenType::Next) return "Next";
    if (t == TokenType::Redo) return "Redo";
    if (t == TokenType::Last) return "Last";
    if (t == TokenType::My) return "My";
    if (t == TokenType::State) return "State";
    if (t == TokenType::Our) return "Our";
    if (t == TokenType::Break) return "Break";
    if (t == TokenType::Continue) return "Continue";
    if (t == TokenType::Given) return "Given";
    if (t == TokenType::Use) return "Use";
    if (t == TokenType::Sub) return "Sub";
    if (t == TokenType::Name) return "Name";
    if (t == TokenType::NumericLiteral) return "NumericLiteral";
    if (t == TokenType::Pod) return "Pod";
    if (t == TokenType::Comma) return "Comma";
    if (t == TokenType::Package) return "Package";
    if (t == TokenType::Local) return "Local";
    if (t == TokenType::Prototype) return "Prototype";
    if (t == TokenType::Signature) return "Signature";
    if (t == TokenType::SubName) return "SubName";
    if (t == TokenType::Attribute) return "Attribute";
    if (t == TokenType::AttributeArgs) return "AttributeArgs";
    if (t == TokenType::AttributeColon) return "AttributeColon";
    if (t == TokenType::StringStart) return "StringStart";
    if (t == TokenType::StringEnd) return "StringEnd";
    if (t == TokenType::HereDoc) return "HereDoc";
    if (t == TokenType::HereDocEnd) return "HereDocEnd";
    if (t == TokenType::HashSubStart) return "HashSubStart";
    if (t == TokenType::HashSubEnd) return "HashSubEnd";
    if (t == TokenType::HashDerefStart) return "HashDerefStart";
    if (t == TokenType::HashDerefEnd) return "HashDerefEnd";
    return "TokenType toString NOT IMPLEMENTED";
}

bool isVariable(const TokenType &tokenType) {
    return tokenType == TokenType::HashVariable || tokenType == TokenType::ArrayVariable ||
           tokenType == TokenType::ScalarVariable;
}

int TokenIterator::getIndex() { return i; }

TokenIterator::TokenIterator(const std::vector<Token> &tokens, std::vector<TokenType> ignoreTokens, int offset)
        : tokens(tokens) {
    this->ignoreTokens = ignoreTokens;
    this->i = offset;
}

TokenIterator::TokenIterator(const std::vector<Token> &tokens, std::vector<TokenType> ignoreTokens) : tokens(tokens) {
    this->ignoreTokens = ignoreTokens;
    this->i = 0;
}

Token TokenIterator::next() {
    while (i < tokens.size()) {
        bool shouldIgnore = false;
        for (auto ignore: ignoreTokens) {
            if (tokens[i].type == ignore) {
                // Try next token
                i++;
                shouldIgnore = true;
                break;
            }
        }

        if (shouldIgnore) continue;
        auto token = tokens[i];
        i++;
        return token;
    }

    return Token(TokenType::EndOfInput, FilePos(0, 0));
}




