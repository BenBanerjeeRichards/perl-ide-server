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

Tokeniser::Tokeniser(std::string perl) {
    this->program = std::move(perl);

    this->keywordConfigs = std::vector<KeywordConfig>{KeywordConfig("use", TokenType::Use),
                                                      KeywordConfig("if", TokenType::If),
                                                      KeywordConfig("else", TokenType::Else),
                                                      KeywordConfig("elseif", TokenType::ElseIf),
                                                      KeywordConfig("unless", TokenType::Until),
                                                      KeywordConfig("while", TokenType::While),
                                                      KeywordConfig("until", TokenType::Until),
                                                      KeywordConfig("for", TokenType::For),
                                                      KeywordConfig("foreach", TokenType::Foreach),
                                                      KeywordConfig("when", TokenType::When),
                                                      KeywordConfig("do", TokenType::Do),
                                                      KeywordConfig("next", TokenType::Next),
                                                      KeywordConfig("redo", TokenType::Redo),
                                                      KeywordConfig("last", TokenType::Last),
                                                      KeywordConfig("my", TokenType::My),
                                                      KeywordConfig("local", TokenType::Local),
                                                      KeywordConfig("state", TokenType::State),
                                                      KeywordConfig("our", TokenType::Our),
                                                      KeywordConfig("break", TokenType::Break),
                                                      KeywordConfig("continue", TokenType::Continue),
                                                      KeywordConfig("given", TokenType::Given),
                                                      KeywordConfig("sub", TokenType::Sub),
                                                      KeywordConfig("package", TokenType::Package),
                                                      KeywordConfig("state", TokenType::State),
                                                      KeywordConfig("use", TokenType::Use),};
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
           c != ')' && c != '[' && c != ']' && c != ':';
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
    auto doubleStr = matchStringLiteral('"');
    if (!doubleStr.empty()) return doubleStr;
    auto singleStr = matchStringLiteral('\'');
    if (!singleStr.empty()) return singleStr;
    auto regexStr = matchStringLiteral('/');
    return regexStr;
}

std::string Tokeniser::matchStringLiteral(char ident, bool includeIdent) {
    std::string contents;
    if (this->peek() == ident || !includeIdent) {
        if (includeIdent) this->nextChar();
        while (this->peek() != ident || (this->peek() == ident && this->peekAhead(0) == '\\')) {
            contents += this->peek();
            this->nextChar();
        }

        if (includeIdent) {
            this->nextChar();
            contents = ident + contents + ident;
        }
    }

    return contents;
}

std::string Tokeniser::matchBracketedStringLiteral(char bracket) {
    std::string contents;
    char endBracket;
    if (bracket == '(') endBracket = ')';
    else if (bracket == '[') endBracket = ']';
    else if (bracket == '{') endBracket = '}';
    else if (bracket == '<') endBracket = '>';
    else return contents;

    int bracketCount = 1;
    while (bracketCount > 0 && peek() != EOF) {
        if (peek() == bracket && peekAhead(0) != '\\') {
            bracketCount++;
        } else if (peek() == endBracket && peekAhead(0) != '\\') {
            bracketCount--;
        }

        if (bracketCount == 0) return contents;
        contents += peek();
        nextChar();
    }

    return contents;
}

std::vector<Token> Tokeniser::matchQuoteLiteral() {
    std::vector<Token> tokens;
    auto start = currentPos();

    auto p1 = peek();
    auto p2 = peekAhead(2);
    std::string quoteChars;

    if ((p1 == 'q' && p2 == 'q') || (p1 == 'q' && p2 == 'x') || (p1 == 'q' && p2 == 'w') || (p1 == 'q' && p2 == 'r')) {
        quoteChars = std::string(1, p1) + p2;
        nextChar();
        nextChar();
    } else if (p1 == 'q' || p2 == 'm') {
        quoteChars = std::string(1, p1);
        nextChar();
    } else {
        return tokens;
    }

    auto whitespace = matchWhitespace();
    auto quoteChar = peek();
    nextChar();
    tokens.emplace_back(Token(TokenType::StringStart, start, quoteChars + whitespace + quoteChar));

    if (quoteChar == '{' || quoteChar == '(' || quoteChar == '<' || quoteChar == '[') {
        start = currentPos();
        auto literal = matchBracketedStringLiteral(quoteChar);
        tokens.emplace_back(Token(TokenType::String, start, literal));
        start = currentPos();
        auto endChar = peek();
        nextChar();
        tokens.emplace_back(Token(TokenType::StringEnd, start, std::string(1, endChar)));
    } else {
        start = currentPos();
        auto literal = matchStringLiteral(quoteChar, false);
        tokens.emplace_back(Token(TokenType::String, start, literal));
        start = currentPos();
        auto endChar = peek();
        nextChar();
        tokens.emplace_back(Token(TokenType::StringEnd, start, std::string(1, endChar)));
    }

    return tokens;
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

std::optional<Token>
Tokeniser::doMatchKeyword(FilePos startPos, const std::string &keywordCode, TokenType keywordType) {
    if (this->matchKeyword(keywordCode)) {
        return std::optional<Token>(Token(keywordType, startPos, startPos.col + (int) keywordCode.size() - 1));
    }

    return std::optional<Token>();
}

std::optional<Token> Tokeniser::tryMatchKeywords(FilePos startPos) {
    for (const auto &config: keywordConfigs) {
        auto attempt = doMatchKeyword(startPos, config.code, config.type);
        if (attempt.has_value()) return attempt;
    }

    return std::optional<Token>();
}

std::string Tokeniser::matchWhitespace() {
    return this->getWhile(this->isWhitespace);
}


std::vector<Token> Tokeniser::tokenise() {
    std::vector<Token> tokens;

    while (true) {
        // Position before anything is consumed
        auto startPos = currentPos();

        if (this->peek() == EOF) {
            tokens.emplace_back(Token(TokenType::EndOfInput, startPos, startPos));
            return tokens;
        }

        // Devour any whitespace
        std::string whitespace = matchWhitespace();
        if (whitespace.length() > 0) {
            tokens.emplace_back(Token(TokenType::Whitespace, startPos, whitespace));
            continue;
        }

        // Now for newlines
        if (this->peek() == '\n') {
            // Linux/mac newline
            this->nextChar();
            tokens.emplace_back(Token(TokenType::Newline, startPos, "\n"));
            continue;
        } else if (this->peek() == '\r' && this->peekAhead(2) == '\n') {
            // windows
            this->nextChar();
            this->nextChar();
            tokens.emplace_back(Token(TokenType::Newline, startPos, "\r\n"));
            continue;
        }

        auto var = this->matchVariable();
        if (!var.empty()) {
            auto type = TokenType::HashVariable;
            if (var[0] == '$') type = TokenType::ScalarVariable;
            if (var[0] == '@') type = TokenType::ArrayVariable;
            tokens.emplace_back(Token(type, startPos, var));
            continue;
        }

        // Perl has so many operators...
        // Thankfully we don't actually care what the do, just need to recognise them
        // TODO complete this list
        auto operators = std::vector<std::string>{
                "->", "+=", "++", "+", "--", "-=", "**=", "*=", "**", "*", "!=", "!~", "!", "~", "\\", "==", "=~",
                "/=", "//=", "=>", "//", "%=", "%", "x=", "x", ">>=", ">>", ">", ">=", "<=>", "<<=", "<<", "<",
                ">=",
                "~~", "&=", "&.=", "&&=", "&&", "&", "||=", "|.=", "|=", "||",
                "~", "^=", "^.=", "^", "...", "..", "?:", ":", ".=",
        };

        // These operators must be followed by a non alphanumeric
        auto wordOperators = std::vector<std::string>{
                "lt", "gt", "le", "ge", "eq", "ne", "cmp", "and", "or", "not", "xor"
        };


        std::string op = matchString(operators);
        if (!op.empty()) {
            tokens.emplace_back(Token(TokenType::Operator, startPos, op));
            continue;
        }

        std::string op2 = matchString(wordOperators, true);
        if (!op2.empty()) {
            tokens.emplace_back(Token(TokenType::Operator, startPos, op2));
            continue;
        }

        // Now consider the really easy single character tokens
        char peek = this->peek();

        if (peek == ';') {
            this->nextChar();
            tokens.emplace_back(Token(TokenType::Semicolon, startPos, startPos.col));
            continue;
        }
        if (peek == ',') {
            this->nextChar();
            tokens.emplace_back(Token(TokenType::Comma, startPos, startPos.col));
            continue;
        }
        if (peek == '{') {
            this->nextChar();
            tokens.emplace_back(Token(TokenType::LBracket, startPos, startPos.col));
            continue;
        }
        if (peek == '}') {
            this->nextChar();
            tokens.emplace_back(Token(TokenType::RBracket, startPos, startPos.col));
            continue;
        }
        if (peek == '(') {
            this->nextChar();
            tokens.emplace_back(Token(TokenType::LParen, startPos, startPos.col));
            continue;
        }
        if (peek == ')') {
            this->nextChar();
            tokens.emplace_back(Token(TokenType::RParen, startPos, startPos.col));
            continue;
        }
        if (peek == '[') {
            this->nextChar();
            tokens.emplace_back(Token(TokenType::LSquareBracket, startPos, startPos.col));
            continue;
        }
        if (peek == ']') {
            this->nextChar();
            tokens.emplace_back(Token(TokenType::RSquareBracket, startPos, startPos.col));
            continue;
        }
        if (peek == '.') {
            this->nextChar();
            tokens.emplace_back(Token(TokenType::Dot, startPos, startPos.col + 1));
            continue;
        }

        // Program keywords
        auto tryKeyword = tryMatchKeywords(startPos);
        if (tryKeyword.has_value()) {
            tokens.emplace_back(tryKeyword.value());
            continue;
        }

        auto numeric = this->matchNumeric();
        if (!numeric.empty()) {
            tokens.emplace_back(Token(TokenType::NumericLiteral, startPos, numeric));
            continue;
        }

        // Numeric first
        if (this->peek() == '-') {
            this->nextChar();
            tokens.emplace_back(Token(TokenType::Operator, startPos, "-"));
            continue;
        }

        auto pod = this->matchPod();
        if (!pod.empty()) {
            // One of the few tokens that can span multiple lines
            tokens.emplace_back(Token(TokenType::Pod, startPos, FilePos(this->currentLine, this->currentCol - 1), pod));
            continue;
        }

        // POD takes priority
        if (this->peek() == '=') {
            this->nextChar();
            tokens.emplace_back(Token(TokenType::Assignment, startPos, startPos.col));
            continue;
        }


        auto string = this->matchString();
        if (!string.empty()) {
            tokens.emplace_back(Token(TokenType::String, startPos, string));
            continue;
        }

        // String literals / quote literals / transliterations / ...
        auto quoteTokens = matchQuoteLiteral();
        if (!quoteTokens.empty()) {
            tokens.insert(tokens.end(), quoteTokens.begin(), quoteTokens.end());
            continue;
        }

        // Numeric first
        if (this->peek() == '/') {
            this->nextChar();
            tokens.emplace_back(Token(TokenType::Operator, startPos, "/"));
            continue;
        }


        auto comment = this->matchComment();
        if (!comment.empty()) {
            tokens.emplace_back(Token(TokenType::Comment, startPos, comment));
            continue;
        }

        auto name = this->matchName();
        if (!name.empty()) {
            tokens.emplace_back(Token(TokenType::Name, startPos, name));
            continue;
        }

        throw TokeniseException(std::string("Remaining code exists"));
    }

}

std::vector<Token> Tokeniser::matchAttribute() {
    // Read name of attribute
    std::vector<Token> tokens;
    if (this->peek() == ':') {
        tokens.emplace_back(Token(TokenType::AttributeColon, currentPos(), ":"));
        nextChar();
    }

    auto start = currentPos();
    auto attrName = matchName();
    if (attrName.empty()) return tokens;
    tokens.emplace_back(Token(TokenType::Attribute, start, attrName));

    start = currentPos();
    std::string arguments;

    if (this->peek() == '(') {
        // Attribute has arguments
        this->nextChar();
        while (this->peek() != ')') {
            arguments += this->nextChar();
        }

        this->nextChar();

        if (!arguments.empty()) {
            arguments = "(" + arguments + ")";
            tokens.emplace_back(Token(TokenType::AttributeArgs, start, arguments));
        }
    }

    return tokens;
}

std::vector<Token> Tokeniser::matchAttributes() {
    // Attributes must start with semicolon
    std::vector<Token> tokens;

    auto start = this->currentPos();
    auto whitespace = matchWhitespace();
    if (!whitespace.empty()) tokens.emplace_back(Token(TokenType::Whitespace, start, whitespace));

    if (peek() != ':') return tokens;
    tokens.emplace_back(Token(TokenType::AttributeColon, currentPos(), ":"));
    this->nextChar();

    // The newline check is just to prevent bad code from consuming the entire file
    while (peek() != '(' || peek() != '{' || peek() != EOF) {
        auto attrTokens = matchAttribute();
        if (attrTokens.empty()) return tokens;
        tokens.insert(tokens.end(), attrTokens.begin(), attrTokens.end());

        start = this->currentPos();
        whitespace = matchWhitespace();
        if (!whitespace.empty()) tokens.emplace_back(Token(TokenType::Whitespace, start, whitespace));
    }

    return tokens;
}

std::string Tokeniser::matchPrototype() {
    std::string proto;
    if (peek() != '(') return proto;
    proto += peek();
    nextChar();

    while (peek() != ')' && peek() != EOF && !isNewline(peek())) {
        proto += peek();
        nextChar();
    }

    if (peek() == ')') proto += ')';
    nextChar();
    return proto;
}

std::string Tokeniser::matchSignature() {
    std::string signature;
    if (peek() != '(') return signature;
    signature += peek();
    nextChar();

    while (peek() != '}' && peek() != EOF && !isNewline(peek())) {
        signature += peek();
        nextChar();
    }

    return signature;
}


std::vector<Token> Tokeniser::matchSignatureTokens() {
    auto start = currentPos();
    auto signature = matchSignature();
    std::vector<Token> tokens;
    if (signature.empty()) return tokens;

    // Handle trailing space
    int numWhitespace = 0;
    for (int j = (int) signature.size() - 1; j >= 0; j--) {
        if (isWhitespace(signature[j])) {
            numWhitespace++;
        } else {
            break;
        }
    }

    if (numWhitespace > 0) {
        auto whitespace = signature.substr(signature.size() - numWhitespace, numWhitespace);
        auto trimmedSignature = signature.substr(0, signature.size() - numWhitespace);
        tokens.emplace_back(Token(TokenType::Signature, start, trimmedSignature));
        FilePos whitespaceStart = start;
        whitespaceStart.col += (int) signature.size() - numWhitespace; // TODO newline?
        tokens.emplace_back(Token(TokenType::Whitespace, whitespaceStart, whitespace));
    } else {
        tokens.emplace_back(Token(TokenType::Signature, start, signature));
    }

    return tokens;
}


// Second pass to fix any tokenization errors with a little bit of context
// Note this is fixing errors, not doing any parsing
void Tokeniser::secondPass(std::vector<Token> &tokens) {
    for (int i = 0; i < (int) tokens.size() - 1; i++) {
        if (tokens[i].type != TokenType::Sub) continue;
        int subPos = i;

        // If function has a prototype/signature then needs to be fixed
        auto nextToken = tokens[i + 1];
        while (nextToken.isWhitespaceNewlineOrComment() && i < tokens.size() - 1) {
            i++;
            nextToken = tokens[i];
        }

        auto name = nextToken;
        if (nextToken.type == TokenType::Name) {
            // Named token, continue past this
            nextToken = tokens[i + 1];
            while (nextToken.isWhitespaceNewlineOrComment() && i < tokens.size() - 1) {
                i++;
                nextToken = tokens[i];
            }
        }

        // No prototype/signature/attributes
        if (nextToken.type == TokenType::LBracket) continue;

        // Next token is a '(' (for signature or proto) or ':' (for attributes)
        auto isProtoOrSig =
                nextToken.type == TokenType::LParen || (nextToken.type == TokenType::Operator && nextToken.data == ":");
        if (!isProtoOrSig) continue;

        FilePos start = nextToken.startPos;
        auto finalTokenPos = nextToken.endPos;

        int retokeniseStartIdx = i + 1;
        while (i < tokens.size() - 1 && nextToken.type != TokenType::LBracket) {
            nextToken = tokens[++i];
        }

        int retokeniseEndIdx = i;
        if (retokeniseEndIdx <= retokeniseStartIdx) continue;
        FilePos end = nextToken.endPos;
        std::string code = this->program.substr(start.position, (end.position - start.position));

        std::vector<Token> newSubTokens;

        // Create new tokeniser to tokenise the code
        // Use same logic to track location
        Tokeniser subCodeTokeniser(code);
        subCodeTokeniser.currentLine = finalTokenPos.line;
        subCodeTokeniser.currentCol = finalTokenPos.col;

        auto attrs = subCodeTokeniser.matchAttributes();
        newSubTokens.insert(newSubTokens.end(), attrs.begin(), attrs.end());
        auto startPos = subCodeTokeniser.currentPos();
        auto whitespace = matchWhitespace();
        if (!whitespace.empty()) newSubTokens.emplace_back(Token(TokenType::Whitespace, startPos, whitespace));

        if (!attrs.empty()) {
            // So we could next try to match a signature
            std::vector<Token> signatureTokens = subCodeTokeniser.matchSignatureTokens();
            if (!signatureTokens.empty())
                newSubTokens.insert(newSubTokens.end(), signatureTokens.begin(), signatureTokens.end());
        } else {
            // No attributes, try matching a prototype/signature
            startPos = subCodeTokeniser.currentPos();
            if (subCodeTokeniser.isPrototype()) {
                auto prototype = subCodeTokeniser.matchPrototype();
                if (!prototype.empty()) newSubTokens.emplace_back(Token(TokenType::Prototype, startPos, prototype));
            } else {
                auto signatureTokens = subCodeTokeniser.matchSignatureTokens();
                if (!signatureTokens.empty())
                    newSubTokens.insert(newSubTokens.end(), signatureTokens.begin(), signatureTokens.end());
            }

            // Finally try matching attributes
            std::vector<Token> moreAttributes = subCodeTokeniser.matchAttributes();
            if (!moreAttributes.empty()) {
                newSubTokens.insert(newSubTokens.end(), moreAttributes.begin(), moreAttributes.end());
            }
        }

        for (auto &token: newSubTokens) {
            token.startPos.position += finalTokenPos.position;
            token.endPos.position += finalTokenPos.position;
        }

        // Finally update tokens array
        tokens.erase(tokens.begin() + retokeniseStartIdx, tokens.begin() + retokeniseEndIdx);
        tokens.insert(tokens.begin() + retokeniseStartIdx, newSubTokens.begin(), newSubTokens.end());

        i = subPos + 1; // Take care as we are modifying tokens while looping over them
    }
}

std::string Tokeniser::tokenToStrWithCode(Token token, bool includeLocation) {
    std::string code;
    bool success = false;

    if (token.type == TokenType::EndOfInput) return "";

    if (token.startPos.position == -1) {
        code = "startPos position not set";
    } else if (token.endPos.position == -1) {
        code = "endPos position not set";
    } else if (token.endPos.position < token.startPos.position) {
        code = "token.endPos.position < token.startPos.position: Invalid positions";
    } else if (token.endPos.position >= this->program.size()) {
        code = "End position pos exceeds program size";
    } else {
        success = true;
        code = this->program.substr(token.startPos.position, (token.endPos.position - token.startPos.position) + 1);
    }

    if (!success) {
        throw "ERROR";
    }

    auto d1 = replace(code, "\n", "\\n");
    code = replace(d1, "\r", "\\r");

    return token.toStr(includeLocation) + " :: `" + code + "`";
}

// Uses heuristic to guess if bracketed expression is a prototype.
bool Tokeniser::isPrototype() {
    int numProtoChars = 0;
    int n = 0;
    int i = 1;

    while (true) {
        auto c = peekAhead(i++);
        if (c == ')' || c == '{' || isNewline(c) || c == EOF) break;
        if (c == '(') continue;
        if (c == '$' || c == '@' || c == '%' || c == '&' || c == '\\' || c == ';' || c == '*' || c == '[' ||
            c == ']')
            numProtoChars += 1;
        n++;
    }

    if (n == 0) return true;
    return (1.0 * numProtoChars / n) > 0.8;
}

FilePos Tokeniser::currentPos() {
    return FilePos(this->currentLine, this->currentCol, this->_position + 1);
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
    if (t == TokenType::ElseIf) return "ElseIf";
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
    return "TokenType toString NOT IMPLEMENTED";
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

int TokenIterator::getIndex() { return i; }

TokenIterator::TokenIterator(const std::vector<Token> &tokens, std::vector<TokenType> ignoreTokens, int offset)
        : tokens(tokens) {
    this->ignoreTokens = ignoreTokens;
    this->i = offset;
}

TokenIterator::TokenIterator(const std::vector<Token> &tokens, std::vector<TokenType> ignoreTokens) : tokens(tokens) {
    this->ignoreTokens = ignoreTokens;
}

KeywordConfig::KeywordConfig(const std::string &code, TokenType type) : code(code), type(type) {}

QuotedStringLiteral::QuotedStringLiteral(FilePos start, FilePos end, std::string literal) {
    this->literalStart = start;
    this->literalEnd = end;
    this->contents = literal;
}
