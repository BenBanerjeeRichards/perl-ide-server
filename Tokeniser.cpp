//
// Created by Ben Banerjee-Richards on 2019-08-19.
//

#include "Tokeniser.h"

#include <utility>
#include <iostream>

static std::regex NUMERIC_REGEX(R"(^(\+|-)?(\d+\.?\d{0,}(e(\+|-)?\d+?)?|0x[\dabcdefABCDEF]+|0b[01]+|)$)");

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

// Returns null for start of file
char Tokeniser::prevChar(int i) {
    // So i = 0 gets the same char as peekAhead(0) would except handles start of files correctly
    if (this->position - i <= -1) {
        // Start of file
        return 0;
    }

    if (this->position - i > this->program.size() - 1) return EOF;
    return this->program[this->position - i];
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

bool Tokeniser::isAlphaNumeric(char c) {
    // TODO make more efficient by using better ASCII ranges
    return isNumber(c) || isUppercase(c) || isLowercase(c);
}

// Variable body i.e. variable name after any Sigil and then first char
bool Tokeniser::isVariableBody(char c) {
    return c >= '!' && c != ';' && c != ',';
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

bool Tokeniser::matchKeyword(const std::string &keyword) {
    for (int i = 0; i < (int) keyword.size(); i++) {
        if (this->peekAhead(i + 1) != keyword[i]) {
            return false;
        }
    }

    // Keyword has been matched
    // Keyword must be followed by non a-zA-Z0-9 character
    char nextChar = this->peekAhead((int) keyword.size() + 1);
    if (isAlphaNumeric(nextChar)) return false;

    // We have the keyword
    this->position += keyword.size();
    return true;
}

std::string Tokeniser::matchName() {
    std::string acc;
    while (this->isVariableBody(this->peek())) {
        acc += this->peek();
        this->nextChar();
    }

    return acc;
}

std::string Tokeniser::matchString() {
    std::string contents;
    if (this->peek() == '"') {
        this->nextChar();
        while (this->peek() != '"' || (this->peek() == '"' && this->peekAhead(0) == '\\')) {
            contents += this->peek();
            this->nextChar();
        }
        this->nextChar();
    } else if (this->peek() == '\'') {
        this->nextChar();
        while (this->peek() != '\'' || (this->peek() == '\'' && this->peekAhead(0) == '\\')) {
            contents += this->peek();
            this->nextChar();
        }
        this->nextChar();
    }

    return contents;
}

std::string Tokeniser::matchNumeric() {
    std::string testString;
    int i = 0;
    while (isAlphaNumeric(peekAhead(i + 1))) {
        testString += peekAhead(i + 1);
        i += 1;
    }
    if (testString.empty()) return "";
    std::smatch regexMatch;

    if (std::regex_match(testString, regexMatch, NUMERIC_REGEX)) {
        this->position += testString.size();
        return testString;
    }

    return "";
}

std::string Tokeniser::matchComment() {
    // TODO this may be too simple and lead to too much being commented - check
    std::string comment;
    if (this->peek() == '#') {
        comment += this->nextChar();
        while (this->peek() != '\n' && this->peek() != '\r' && this->peek() != EOF) {
            comment += this->nextChar();
        }
    }

    return comment;
}

std::string Tokeniser::matchPod() {
    char prevChar = this->prevChar(0);
    std::string pod;
    if (prevChar == 0 || prevChar == '\n' || prevChar == '\r') {
        // Valid place to put a pod, now check if there is one
        if (this->peek() == '=') {
            // Yes
            pod += this->nextChar();

            // Consume until end of line (we can't start and end POD on same line)
            pod += this->getUntil(isNewline);

            // Now consume until ending
            while (true) {
                char c1 = this->peek();
                char c2 = this->peekAhead(2);
                char c3 = this->peekAhead(3);
                char c4 = this->peekAhead(4);
                pod += this->nextChar();
                if ((c1 == '=' && c2=='c' && c3=='u' && c4 == 't')) {
                    this->position += 3;
                    pod += "cut";
                    break;
                }
            }
        }
    }

    return pod;
}


Token Tokeniser::nextToken() {
    if (this->peek() == EOF) {
        return Token(TokenType::EndOfInput, 0, 0);
    }

    // Devour any whitespace
    std::string whitespace = this->getUntil(this->isWhitespace);
    if (whitespace.length() > 0) {
        return Token(TokenType::Whitespace, whitespace, 1, 2, 3, 4);
    }

    // Now for newlines
    // TODO Ensure this works on all platforms
    std::string newlineTokens = this->getUntil(this->isNewline);
    if (newlineTokens.length() > 0) {
        return Token(TokenType::Newline, newlineTokens, 1, 2);
    }

    char sigil = this->peek();
    // Variables
    if (sigil == '$' || sigil == '@' || sigil == '%') {
        this->nextChar();   // Consume Sigil

        std::string variableRest = getUntil(isVariableBody);
        std::string fullName;
        fullName += sigil;
        fullName += variableRest;

        if (sigil == '$') return Token(TokenType::ScalarVariable, fullName, 0, 0);
        if (sigil == '%') return Token(TokenType::HashVariable, fullName, 0, 0);
        return Token(TokenType::ArrayVariable, fullName, 0, 0);
    }

    // Perl has so many operators...
    // Thankfully we don't actually care what the do, just need to recognise them
    // TODO complete this list
    auto operators = std::vector<std::string>{
            "->", "+=", "++", "+", "--", "-=", "-", "**=", "*=", "**", "*", "!=", "!~", "!", "-", "~", "\\", "==", "=~",
            "/=", "//=", "//", "/", "%=", "%", "x=", "x", ">>=", ">>", ">", ">=", "<=>", "<<=", "<<", "<", ">=",
            "lt", "gt", "le", "ge", "eq", "ne", "cmp", "~~", "&=", "&.=", "&&=", "&&", "&", "||=", "|.=", "|=", "||",
            "~", "^=", "^.=", "^", "and", "or", "...", "..", "?:", ":", ".=", "not", "xor"
    };

    std::string op = matchString(operators);
    if (!op.empty()) {
        return (Token(TokenType::Operator, op, 0, 0));
    }

    // Now consider the really easy single character tokens
    char peek = this->peek();

    if (peek == ';') {
        this->nextChar();
        return Token(TokenType::Semicolon, 0, 0);
    }
    if (peek == ',') {
        this->nextChar();
        return Token(TokenType::Comma, 0, 0);
    }
    if (peek == '{') {
        this->nextChar();
        return Token(TokenType::LBracket, 0, 0);
    }
    if (peek == '}') {
        this->nextChar();
        return Token(TokenType::RBracket, 0, 0);
    }
    if (peek == '(') {
        this->nextChar();
        return Token(TokenType::LParen, 0, 0);
    }
    if (peek == ')') {
        this->nextChar();
        return Token(TokenType::RParen, 0, 0);
    }
    if (peek == '[') {
        this->nextChar();
        return Token(TokenType::LSquareBracket, 0, 0);
    }
    if (peek == ']') {
        this->nextChar();
        return Token(TokenType::RSquareBracket, 0, 0);
    }
    if (peek == '.') {
        this->nextChar();
        return Token(TokenType::Dot, 0, 0);
    }
    // Control flow keyword
    if (this->matchKeyword("use")) return Token(TokenType::Use, 0, 0);
    if (this->matchKeyword("if")) return Token(TokenType::If, 0, 0);
    if (this->matchKeyword("else")) return Token(TokenType::Else, 0, 0);
    if (this->matchKeyword("elseif")) return Token(TokenType::ElseIf, 0, 0);
    if (this->matchKeyword("unless")) return Token(TokenType::Unless, 0, 0);
    if (this->matchKeyword("while")) return Token(TokenType::While, 0, 0);
    if (this->matchKeyword("until")) return Token(TokenType::Until, 0, 0);
    if (this->matchKeyword("for")) return Token(TokenType::For, 0, 0);
    if (this->matchKeyword("foreach")) return Token(TokenType::Foreach, 0, 0);
    if (this->matchKeyword("when")) return Token(TokenType::When, 0, 0);
    if (this->matchKeyword("do")) return Token(TokenType::Do, 0, 0);
    if (this->matchKeyword("next")) return Token(TokenType::Next, 0, 0);
    if (this->matchKeyword("redo")) return Token(TokenType::Redo, 0, 0);
    if (this->matchKeyword("last")) return Token(TokenType::Last, 0, 0);
    if (this->matchKeyword("my")) return Token(TokenType::My, 0, 0);
    if (this->matchKeyword("state")) return Token(TokenType::State, 0, 0);
    if (this->matchKeyword("our")) return Token(TokenType::Our, 0, 0);
    if (this->matchKeyword("break")) return Token(TokenType::Break, 0, 0);
    if (this->matchKeyword("continue")) return Token(TokenType::Continue, 0, 0);
    if (this->matchKeyword("given")) return Token(TokenType::Given, 0, 0);
    if (this->matchKeyword("sub")) return Token(TokenType::Sub, 0, 0);

    auto numeric = this->matchNumeric();
    if (!numeric.empty()) return Token(TokenType::NumericLiteral, numeric, 0, 0);

    auto pod = this->matchPod();
    if (!pod.empty()) return Token(TokenType::Pod, pod, 0, 0);

    // POD takes priority
    if (peek == '=') {
        this->nextChar();
        return Token(TokenType::Assignment, 0, 0);
    }


    auto name = this->matchName();
    if (!name.empty()) {
        return Token(TokenType::Name, name, 0, 0);
    }

    auto string = this->matchString();
    if (!string.empty()) return Token(TokenType::String, string, 0, 0);

    auto comment = this->matchComment();
    if (!comment.empty()) return Token(TokenType::Comment, comment, 0, 0, 0, 0);
    throw TokeniseException(std::string("Remaining code exists"));
}

std::string tokenToString(const TokenType &t) {
    if (t == String) return "String";
    if (t == ScalarVariable) return "ScalarVariable";
    if (t == ArrayVariable) return "ArrayVariable";
    if (t == HashVariable) return "HashVariable";
    if (t == Operator) return "Operator";
    if (t == LBracket) return "LBracket";
    if (t == RBracket) return "RBracket";
    if (t == LParen) return "LParen";
    if (t == RParen) return "RParen";
    if (t == LSquareBracket) return "LSquareBracket";
    if (t == RSquareBracket) return "RSquareBracket";
    if (t == Comment) return "Comment";
    if (t == Newline) return "Newline";
    if (t == Whitespace) return "Whitespace";
    if (t == Dot) return "Dot";
    if (t == Assignment) return "Assignment";
    if (t == Semicolon) return "Semicolon";
    if (t == EndOfInput) return "EndOfInput";
    if (t == If) return "If";
    if (t == Else) return "Else";
    if (t == ElseIf) return "ElseIf";
    if (t == Unless) return "Unless";
    if (t == While) return "While";
    if (t == Until) return "Until";
    if (t == For) return "For";
    if (t == Foreach) return "Foreach";
    if (t == When) return "When";
    if (t == Do) return "Do";
    if (t == Next) return "Next";
    if (t == Redo) return "Redo";
    if (t == Last) return "Last";
    if (t == My) return "My";
    if (t == State) return "State";
    if (t == Our) return "Our";
    if (t == Break) return "Break";
    if (t == Continue) return "Continue";
    if (t == Given) return "Given";
    if (t == Use) return "Use";
    if (t == Sub) return "Sub";
    if (t == Name) return "Name";
    if (t == NumericLiteral) return "NumericLiteral";
    if (t == Pod) return "Pod";
    if (t == Comma) return "Comma";
    return "TokenType toString NOT IMPLEMENTED";
}


