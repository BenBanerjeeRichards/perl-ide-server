//
// Created by Ben Banerjee-Richards on 2019-08-19.
//

#include "Tokeniser.h"

#include <utility>
#include <iostream>

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
    return c >= '!' && c != ';';
}

std::string Tokeniser::matchString(const std::vector<std::string> &options) {
    for (const std::string &option : options) {
        bool match = true;  // Assume match until proven otherwise
        for (int i = 0; i < (int) option.length(); i++) {
            match = match && (this->peekAhead(i + 1) == option[i]);
        }

        if (match) {
            this->position += option.length();
            return option;
        }
    }

    return "";
}

bool Tokeniser::matchKeyword(const std::string& keyword) {
    for (int i = 0; i < (int)keyword.size(); i++) {
        if (this->peekAhead(i + 1) != keyword[i]) {
            return false;
        }
    }

    // Keyword has been matched
    // Keyword must be followed by non a-zA-Z0-9 character
    char nextChar = this->peekAhead((int)keyword.size() + 1);
    if (this->isNumber(nextChar) || this->isUppercase(nextChar) || this->isLowercase(nextChar)) return false;

    // We have the keyword
    this->position += keyword.size();
    return true;
}

Token Tokeniser::nextToken() {
    if (this->peek() == EOF) {
        return EndOfInputToken(0, 0);
    }

    // Devour any whitespace
    std::string whitespace = this->getUntil(this->isWhitespace);
    if (whitespace.length() > 0) {
        return WhitespaceToken(whitespace, 1, 2, 3, 4);
    }

    // Now for newlines
    // TODO Ensure this works on all platforms
    std::string newlineTokens = this->getUntil(this->isNewline);
    if (newlineTokens.length() > 0) {
        return NewlineToken(newlineTokens, 1, 2);
    }

    char sigil = this->peek();
    // Variables
    if (sigil == '$' || sigil == '@' || sigil == '%') {
        this->nextChar();   // Consume Sigil

        std::string variableRest = getUntil(isVariableBody);
        std::string fullName;
        fullName += sigil;
        fullName += variableRest;

        if (sigil == '$') return ScalarVariableToken(fullName, 0, 0);
        if (sigil == '%') return HashVariableToken(fullName, 0, 0);
        return ArrayVariableToken(fullName, 0, 0);
    }

    // Perl has so many operators...
    // Thankfully we don't actually care what the do, just need to recognise them
    // TODO complete this list
    auto operators = std::vector<std::string>{
            "->", "+=", "++", "+", "--", "-=", "-", "**=", "*=", "**", "*", "!=", "!~", "!", "-", "~", "\\", "==", "=~",
            "=", "/=", "//=", "//", "/", "%=", "%", "x=", "x", ">>=", ">>", ">", ">=", "<=>", "<<=", "<<", "<", ">=",
            "lt", "gt", "le", "ge", "eq", "ne", "cmp", "~~", "&=", "&.=", "&&=", "&&", "&", "||=", "|.=", "|=", "||",
            "~", "^=", "^.=", "^", "and", "or", "...", "..", "?:", ":", ".=", "not", "xor"
    };

    std::string op = matchString(operators);
    if (!op.empty()) {
        return (OperatorToken(op, 0, 0));
    }

    // Now consider the really easy single character tokens
    char peek = this->peek();

    if (peek == ';') {
        this->nextChar();
        return SemicolonToken(0, 0);
    }
    if (peek == '{') {
        this->nextChar();
        return LBracketToken(0, 0);
    }
    if (peek == '}') {
        this->nextChar();
        return RBracketToken(0, 0);
    }
    if (peek == '(') {
        this->nextChar();
        return LParenToken(0, 0);
    }
    if (peek == ')') {
        this->nextChar();
        return RParenToken(0, 0);
    }
    if (peek == '[') {
        this->nextChar();
        return LSquareBracketToken(0, 0);
    }
    if (peek == ']') {
        this->nextChar();
        return RSquareBracketToken(0, 0);
    }
    if (peek == '.') {
        this->nextChar();
        return DotToken(0, 0);
    }
    if (peek == '=') {
        this->nextChar();
        return AssignmentToken(0, 0);
    }

    // Control flow keyword
    if (this->matchKeyword("use")) return UseToken(0, 0);
    if (this->matchKeyword("if")) return IfToken(0, 0);
    if (this->matchKeyword("else")) return ElseToken(0, 0);
    if (this->matchKeyword("elseif")) return ElseIfToken(0, 0);
    if (this->matchKeyword("unless")) return UnlessToken(0, 0);
    if (this->matchKeyword("while")) return WhileToken(0, 0);
    if (this->matchKeyword("until")) return UntilToken(0, 0);
    if (this->matchKeyword("for")) return ForToken(0, 0);
    if (this->matchKeyword("foreach")) return ForeachToken(0, 0);
    if (this->matchKeyword("when")) return WhenToken(0, 0);
    if (this->matchKeyword("do")) return DoToken(0, 0);
    if (this->matchKeyword("next")) return NextToken(0, 0);
    if (this->matchKeyword("redo")) return RedoToken(0, 0);
    if (this->matchKeyword("last")) return LastToken(0, 0);
    if (this->matchKeyword("my")) return MyToken(0, 0);
    if (this->matchKeyword("state")) return StateToken(0, 0);
    if (this->matchKeyword("our")) return OurToken(0, 0);
    if (this->matchKeyword("break")) return BreakToken(0, 0);
    if (this->matchKeyword("continue")) return ContinueToken(0, 0);
    if (this->matchKeyword("given")) return GivenToken(0, 0);


    throw TokeniseException(std::string("Remaining code exists"));
}

