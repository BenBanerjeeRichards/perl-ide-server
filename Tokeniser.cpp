//
// Created by Ben Banerjee-Richards on 2019-08-19.
//

#include "Tokeniser.h"

#include <utility>

Tokeniser::Tokeniser(std::string perl) {
    this->program = std::move(perl);
}

char Tokeniser::nextChar() {
    if (this->position == this->program.length() - 1) {
        return EOF;
    }

    this->position += 1;
    return this->program[this->position];
}

char Tokeniser::peekAhead(int i) {
    if (this->position + i > this->program.length() - 1) {
        return EOF;
    }

    return this->program[this->position + i];
}

char Tokeniser::peek() {
    return this->peekAhead(1);
}

bool Tokeniser::isEof() {
    return this->position > this->program.length() - 1;
}

std::string Tokeniser::getUntil(const std::function<bool(char)> &nextCharTest) {
    std::string acc;
    while (nextCharTest(this->peek()) && !this->isEof()) {
        acc += this->nextChar();
    }

    return acc;
}

bool Tokeniser::isWhitespace(char c) {
    return c == ' ' || c == '\t';
}

bool Tokeniser::isNewline(char c) {
    return c == '\n' || c == '\r';
}

bool Tokeniser::isLowercase(char c) {
    return c >= 'a' && c <= 'z';
}

bool Tokeniser::isUppercase(char c) {
    return c >= 'A' && c <= 'Z';
}

bool Tokeniser::isNumber(char c) {
    return c >= '0' && c <= '9';
}

// Variable body i.e. variable name after any Sigil and then first char
bool Tokeniser::isVariableBody(char c) {
    return c == '_' || isNumber(c) || isUppercase(c) || isLowercase(c);
}

std::unique_ptr<Token> Tokeniser::nextToken() {
    // First devour any whitespace
    std::string whitespace = this->getUntil(this->isWhitespace);
    if (whitespace.length() > 0) {
        return std::make_unique<Token>(WhitespaceToken(whitespace, 1, 2, 3, 4));
    }

    // Now for newlines
    // TODO Ensure this works on all platforms
    std::string newlineTokens = this->getUntil(this->isNewline);
    if (newlineTokens.length() > 0) {
        return std::make_unique<Token>(NewlineToken(newlineTokens, 1, 2));
    }

    char peekChar = this->peek();
    // Variables
    if (peekChar == '$' || peekChar == '@' || peekChar == '%') {
        char firstVariableChar = this->peekAhead(2);
        if (firstVariableChar == '_' || isLowercase(firstVariableChar)) {
            // At this point we have a valid variable
            this->nextChar(); // Sigil
            this->nextChar(); // First char
            std::string variableRest = getUntil(isVariableBody);
            std::string fullName;
            fullName += peekChar;
            fullName += firstVariableChar;
            fullName += variableRest;

            if (peekChar == '$') return std::make_unique<Token>(ScalarVariableToken(fullName, 0, 0));
            if (peekChar == '%') return std::make_unique<Token>(HashVariableToken(fullName, 0, 0));
            return std::make_unique<Token>(ArrayVariableToken(fullName, 0, 0));
        }
    }

    // Now for some fairly easy operators

    return nullptr;
}

