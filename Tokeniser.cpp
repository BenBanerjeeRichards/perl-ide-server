//
// Created by Ben Banerjee-Richards on 2019-08-19.
//

#include "Tokeniser.h"

static std::regex NUMERIC_REGEX(R"(^(\+|-)?(\d+\.?\d{0,}(e(\+|-)?\d+?)?|0x[\dabcdefABCDEF]+|0b[01]+|)$)");

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

bool Token::isWhitespaceNewlineOrComment() {
    return type == Whitespace || type == Newline || type == Comment;
}

Tokeniser::Tokeniser(std::string perl) {
    this->program = std::move(perl);
}

int Tokeniser::nextLine() {
    this->currentLine += 1;
    this->currentCol = 1;
    return this->currentLine;
}

void Tokeniser::advancePositionSameLine(int i) {
    this->_position += i;
    this->currentCol += i;
}

char Tokeniser::nextChar() {
    if (this->_position == this->program.length() - 1) {
        return EOF;
    }

    if ((this->peek() == '\r' && this->peekAhead(2) == '\n') || this->peek() == '\n') {
        // Newline coming up, other code will handle token
        this->nextLine();
    } else {
        this->currentCol += 1;
    }

    this->_position += 1;
    return this->program[this->_position];
}

char Tokeniser::peekAhead(int i) {
    if (this->_position + i > this->program.length() - 1) {
        return EOF;
    }

    return this->program[this->_position + i];
}

// TODO probably useless
std::string Tokeniser::substring(int fromIdx, int length) {
    return this->program.substr(fromIdx, length);
}

// Returns null for start of file
char Tokeniser::prevChar(int i) {
    // So i = 0 gets the same char as peekAhead(0) would except handles start of files correctly
    if (this->_position - i <= -1) {
        // Start of file
        return 0;
    }

    if (this->_position - i > this->program.size() - 1) return EOF;
    return this->program[this->_position - i];
}

char Tokeniser::peek() {
    return this->peekAhead(1);
}

bool Tokeniser::isEof() {
    return this->_position > this->program.length() - 1;
}

std::string Tokeniser::getWhile(const std::function<bool(char)> &nextCharTest) {
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


bool Tokeniser::isPunctuation(char c) {
    return (c >= '!' && c <= '/') || (c >= ':' && c <= '@') || (c >= '[' && c <= '`') || (c >= '{' && c <= '~');
}

bool Tokeniser::isNumber(char c) {
    return c >= '0' && c <= '9';
}

bool Tokeniser::isAlphaNumeric(char c) {
    // TODO make more efficient by using better ASCII ranges
    return isNumber(c) || isUppercase(c) || isLowercase(c);
}

// Variable body i.e. variable name after any Sigil and then first char
bool Tokeniser::isNameBody(char c) {
    return c >= '!' && c != ';' && c != ',' && c != '>' && c != '-' && c != '.' && c != '{' && c != '}' && c != '(' &&
           c != ')' && c != '[' && c != ']';
}

std::string Tokeniser::matchString(const std::vector<std::string> &options, bool requireTrailingNonAN) {
    for (const std::string &option : options) {
        bool match = true;  // Assume match until proven otherwise
        for (int i = 0; i < (int) option.length(); i++) {
            match = match && (this->peekAhead(i + 1) == option[i]);
        }

        // If requireTrailingNonAN check next char is not alphanumeric
        // This fixes issues with `sub length() {...}` being translated to NAME(SUB) OP(LE) NAME(GTH) ...
        if (match && (!requireTrailingNonAN || !this->isAlphaNumeric(this->peekAhead((int) option.length() + 1)))) {
            if (requireTrailingNonAN) {

            }
            this->advancePositionSameLine(option.length());
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
    this->advancePositionSameLine(keyword.size());
    return true;
}

std::string Tokeniser::matchName() {
    std::string acc;
    while (this->isNameBody(this->peek())) {
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
        contents = '"' + contents = '"';
    } else if (this->peek() == '\'') {
        this->nextChar();
        while (this->peek() != '\'' || (this->peek() == '\'' && this->peekAhead(0) == '\\')) {
            contents += this->peek();
            this->nextChar();
        }
        this->nextChar();
        contents = '\'' + contents + '\'';
    }

    return contents;
}

std::string Tokeniser::matchNumeric() {
    std::string testString;
    int i = 0;
    while (isAlphaNumeric(peekAhead(i + 1)) || peekAhead(i + 1) == '.' || peekAhead(i + 1) == '+' ||
           peekAhead(i + 1) == '-') {
        testString += peekAhead(i + 1);
        i += 1;
    }
    if (testString.empty()) return "";
    std::smatch regexMatch;

    if (std::regex_match(testString, regexMatch, NUMERIC_REGEX)) {
        this->advancePositionSameLine(testString.size());
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
            pod += this->getWhile(isNewline);

            // Now consume until ending
            while (true) {
                char c1 = this->peek();
                char c2 = this->peekAhead(2);
                char c3 = this->peekAhead(3);
                char c4 = this->peekAhead(4);
                pod += this->nextChar();
                if ((c1 == '=' && c2 == 'c' && c3 == 'u' && c4 == 't')) {
                    this->advancePositionSameLine(3);
                    pod += "cut";
                    break;
                }
            }
        }
    }

    return pod;
}


int Tokeniser::peekPackageTokens(int i) {
    // ::' is ok but ':: is not
    int start = i;
    while (true) {
        if (i - start > 10000) throw TokeniseException("BUG: peekPackageTokens in assumed infinite loop");
        if (this->peekAhead(i) == ':' && this->peekAhead(i + 1) == ':') {
            i += 2;
        } else if (this->peekAhead(i) == '\'' && (this->peekAhead(i + 1) != ':' && this->peekAhead(i + 2) != ':')) {
            i += 1;
        } else {
            return i - start;
        }
    }
}

// Too complicated to use regex (could do it but regex would be horrible to write and debug)
std::string Tokeniser::matchVariable() {
    int i = 1;
    // Must start with sigil
    if (peekAhead(i) != '$' && peekAhead(i) != '@' && peekAhead(i) != '%') return "";
    i += 1;

    // Consider a standard variable
    // Main complication here is the package rules (:: and ')
    int packageDelta = peekPackageTokens(i);
    i += packageDelta;
    if (isUppercase(this->peekAhead(i)) || isLowercase(this->peekAhead(i)) || this->peekAhead(i) == '_') {
        // Valid starting
        i += 1;
        while (true) {
            int start = i;
            i += peekPackageTokens(i);
            while (this->isAlphaNumeric(this->peekAhead(i)) || this->peekAhead(i) == '_') i += 1;

            // If we don't progress any more, then the variable is finished
            if (i == start) {
                break;
            }
        }
    } else {
        // Now try the special variables
        i -= packageDelta;

        if (this->peekAhead(i) == '^') {
            // Variable in the form $^A;
            char second = this->peekAhead(i + 1);
            if (this->isUppercase(second) || second == '[' || second == ']' || second == '^' || second == '_' ||
                second == '?' || second == '\\') {
                i += 2;
                goto done;
            }
        }

        if (this->isNumber(this->peekAhead(i))) {
            // Numeric variable
            i += 1;
            while (this->isNumber(this->peekAhead(i))) i += 1;
            goto done;
        }

        if (this->isPunctuation(this->peekAhead(i)) && this->peekAhead(i) != '{') {
            // Single punctuation variable
            i += 1;
            goto done;
        }

        int beforeBracket = i;      // If we don't find a matching bracket then don't count this as a valid variable
        if (this->peekAhead(i) == '{' && this->peekAhead(i + 1) == '^') {
            // The square bracket ones
            i += 2;
            if (this->peekAhead(i) == '_') i += 1;      // Optional underscore
            while (this->isAlphaNumeric(this->peekAhead(i))) i += 1;
            if (this->peekAhead(i) == '}') {
                i += 1;
                goto done;
            } else {
                i = beforeBracket;
            }
        }
    }

    done:
    // If nothing after sigil matched, then don't match as a variable
    if (i == 2) return "";
    std::string var = this->substring(this->_position + 1, i - 1);
    this->advancePositionSameLine(i - 1);
    return var;
}

Token Tokeniser::nextToken() {
    // Position before anything is consumed
    auto startPos = FilePos(this->currentLine, this->currentCol);

    if (this->peek() == EOF) {
        return Token(TokenType::EndOfInput, startPos);
    }

    // Devour any whitespace
    std::string whitespace = this->getWhile(this->isWhitespace);
    if (whitespace.length() > 0) {
        return Token(TokenType::Whitespace, startPos, whitespace);
    }

    // Now for newlines
    if (this->peek() == '\n') {
        // Linux/mac newline
        this->nextChar();
        return Token(TokenType::Newline, startPos, "\n");
    } else if (this->peek() == '\r' && this->peekAhead(2) == '\n') {
        // windows
        this->nextChar();
        this->nextChar();
        return Token(TokenType::Newline, startPos, "\r\n");
    }

    auto var = this->matchVariable();
    if (!var.empty()) {
        auto type = TokenType::HashVariable;
        if (var[0] == '$') type = ScalarVariable;
        if (var[0] == '@') type = ArrayVariable;
        return Token(type, startPos, var);
    }

    // Perl has so many operators...
    // Thankfully we don't actually care what the do, just need to recognise them
    // TODO complete this list
    auto operators = std::vector<std::string>{
            "->", "+=", "++", "+", "--", "-=", "**=", "*=", "**", "*", "!=", "!~", "!", "~", "\\", "==", "=~",
            "/=", "//=", "=>", "//", "/", "%=", "%", "x=", "x", ">>=", ">>", ">", ">=", "<=>", "<<=", "<<", "<", ">=",
            "~~", "&=", "&.=", "&&=", "&&", "&", "||=", "|.=", "|=", "||",
            "~", "^=", "^.=", "^", "...", "..", "?:", ":", ".=",
    };

    // These operators must be followed by a non alphanumeric
    auto wordOperators = std::vector<std::string>{
            "lt", "gt", "le", "ge", "eq", "ne", "cmp", "and", "or", "not", "xor"
    };


    std::string op = matchString(operators);
    if (!op.empty()) {
        return (Token(TokenType::Operator, startPos, op));
    }

    std::string op2 = matchString(wordOperators, true);
    if (!op2.empty()) {
        return (Token(TokenType::Operator, startPos, op2));
    }

    // Now consider the really easy single character tokens
    char peek = this->peek();

    if (peek == ';') {
        this->nextChar();
        return Token(TokenType::Semicolon, startPos, startPos.col);
    }
    if (peek == ',') {
        this->nextChar();
        return Token(TokenType::Comma, startPos, startPos.col);
    }
    if (peek == '{') {
        this->nextChar();
        return Token(TokenType::LBracket, startPos, startPos.col);
    }
    if (peek == '}') {
        this->nextChar();
        return Token(TokenType::RBracket, startPos, startPos.col);
    }
    if (peek == '(') {
        this->nextChar();
        return Token(TokenType::LParen, startPos, startPos.col);
    }
    if (peek == ')') {
        this->nextChar();
        return Token(TokenType::RParen, startPos, startPos.col);
    }
    if (peek == '[') {
        this->nextChar();
        return Token(TokenType::LSquareBracket, startPos, startPos.col);
    }
    if (peek == ']') {
        this->nextChar();
        return Token(TokenType::RSquareBracket, startPos, startPos.col);
    }
    if (peek == '.') {
        this->nextChar();
        return Token(TokenType::Dot, startPos, startPos.col + 1);
    }
    // Control flow keyword
    if (this->matchKeyword("use")) return Token(TokenType::Use, startPos, startPos.col + 2);
    if (this->matchKeyword("if")) return Token(TokenType::If, startPos, startPos.col + 1);
    if (this->matchKeyword("else")) return Token(TokenType::Else, startPos, startPos.col + 3);
    if (this->matchKeyword("elseif")) return Token(TokenType::ElseIf, startPos, startPos.col + 5);
    if (this->matchKeyword("unless")) return Token(TokenType::Unless, startPos, startPos.col + 5);
    if (this->matchKeyword("while")) return Token(TokenType::While, startPos, startPos.col + 4);
    if (this->matchKeyword("until")) return Token(TokenType::Until, startPos, startPos.col + 4);
    if (this->matchKeyword("for")) return Token(TokenType::For, startPos, startPos.col + 2);
    if (this->matchKeyword("foreach")) return Token(TokenType::Foreach, startPos, startPos.col + 6);
    if (this->matchKeyword("when")) return Token(TokenType::When, startPos, startPos.col + 3);
    if (this->matchKeyword("do")) return Token(TokenType::Do, startPos, startPos.col + 1);
    if (this->matchKeyword("next")) return Token(TokenType::Next, startPos, startPos.col + 3);
    if (this->matchKeyword("redo")) return Token(TokenType::Redo, startPos, startPos.col + 3);
    if (this->matchKeyword("last")) return Token(TokenType::Last, startPos, startPos.col + 3);
    if (this->matchKeyword("my")) return Token(TokenType::My, startPos, startPos.col + 1);
    if (this->matchKeyword("local")) return Token(TokenType::Local, startPos, startPos.col + 4);
    if (this->matchKeyword("state")) return Token(TokenType::State, startPos, startPos.col + 4);
    if (this->matchKeyword("our")) return Token(TokenType::Our, startPos, startPos.col + 2);
    if (this->matchKeyword("break")) return Token(TokenType::Break, startPos, startPos.col + 4);
    if (this->matchKeyword("continue")) return Token(TokenType::Continue, startPos, startPos.col + 7);
    if (this->matchKeyword("given")) return Token(TokenType::Given, startPos, startPos.col + 4);
    if (this->matchKeyword("sub")) return Token(TokenType::Sub, startPos, startPos.col + 2);
    if (this->matchKeyword("package")) return Token(TokenType::Package, startPos, startPos.col + 6);

    auto numeric = this->matchNumeric();
    if (!numeric.empty()) return Token(TokenType::NumericLiteral, startPos, numeric);


    // Numeric first
    if (this->peek() == '-') {
        this->nextChar();
        return Token(TokenType::Operator, startPos, "-");
    }

    auto pod = this->matchPod();
    if (!pod.empty()) {
        // One of the few tokens that can span multiple lines
        return Token(TokenType::Pod, startPos, FilePos(this->currentLine, this->currentCol - 1), pod);
    }

    // POD takes priority
    if (this->peek() == '=') {
        this->nextChar();
        return Token(TokenType::Assignment, startPos, startPos.col);
    }


    auto string = this->matchString();
    if (!string.empty()) return Token(TokenType::String, startPos, string);

    auto comment = this->matchComment();
    if (!comment.empty()) return Token(TokenType::Comment, startPos, comment);

    auto name = this->matchName();
    if (!name.empty()) {
        return Token(TokenType::Name, startPos, name);
    }

    throw TokeniseException(std::string("Remaining code exists"));
}


std::string tokenTypeToString(const TokenType &t) {
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
    if (t == Package) return "Package";
    if (t == Local) return "Local";
    return "TokenType toString NOT IMPLEMENTED";
}
