//
// Created by Ben Banerjee-Richards on 2019-08-19.
//

#include "Tokeniser.h"

static std::regex NUMERIC_REGEX(R"(^(\+|-)?(\d+\.?\d{0,}(e(\+|-)?\d+?)?|0x[\dabcdefABCDEF]+|0b[01]+|)$)");

Tokeniser::Tokeniser(std::string perl, bool doSecondPass) {
    this->program = std::move(perl);
    this->doSecondPass = doSecondPass;

    this->keywordConfigs = std::vector<KeywordConfig>{KeywordConfig("use", TokenType::Use),
                                                      KeywordConfig("if", TokenType::If),
                                                      KeywordConfig("else", TokenType::Else),
                                                      KeywordConfig("elsif", TokenType::ElsIf),
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

    // Remove unicode Byte Order Mark (0xEFBBBF) if it exists
    if (this->program.size() >= 3 && this->program[0] == '\xEF' && this->program[1] == '\xBB' &&
        this->program[2] == '\xBF') {
        this->program = this->program.substr(3, this->program.size() - 3);
    } else if (this->program.size() >= 2 && this->program[0] == '\xFF' && this->program[1] == '\xFE') {
        this->program = this->program.substr(2, this->program.size() - 3);
    } else if (this->program.size() >= 2 && this->program[0] == '\xFE' && this->program[1] == '\xFF') {
        this->program = this->program.substr(2, this->program.size() - 3);
    } else if (this->program.size() >= 4 && this->program[0] == '\xFF' && this->program[1] == '\xFE' &&
               this->program[2] == '\x00' && this->program[3] == '\x00') {
        this->program = this->program.substr(4, this->program.size() - 3);
    } else if (this->program.size() >= 4 && this->program[0] == '\x00' && this->program[1] == '\x00' &&
               this->program[2] == '\xFE' && this->program[3] == '\xFF') {
        this->program = this->program.substr(4, this->program.size() - 3);
    }

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

    if ((this->peek() == '\r' && this->peekAhead(2) == '\n') || (this->peek() == '\n' && this->peekAhead(0) != '\r')) {
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
    return this->_position > (int) this->program.length() - 1;
}

std::string Tokeniser::getWhile(const std::function<bool(char)> &nextCharTest) {
    std::string acc;
    while (nextCharTest(this->peek()) && !this->isEof()) {
        acc += this->nextChar();
    }

    return acc;
}

bool Tokeniser::isWhitespace(char c) {
    return c == ' ' || c == '\t' || c == '\x0c';
}

bool Tokeniser::isNewline(char c) {
    return c == '\n' || c == '\r';
}

bool Tokeniser::isPunctuation(char c) {
    return (c >= '!' && c <= '/') || (c >= ':' && c <= '@') || (c >= '[' && c <= '`') || (c >= '{' && c <= '~');
}

// Variable body i.e. variable name after any Sigil and then first char
bool Tokeniser::isNameBody(char c) {
    return c >= '!' && c != ';' && c != ',' && c != '>' && c != '<' && c != '-' && c != '.' && c != '{' && c != '}' &&
           c != '(' &&
           c != ')' && c != '[' && c != ']' && c != ':';
}

std::string Tokeniser::matchStringOption(const std::vector<std::string> &options, bool requireTrailingNonAN) {
    for (const std::string &option : options) {
        bool match = true;  // Assume match until proven otherwise
        for (int i = 0; i < (int) option.length(); i++) {
            match = match && (this->peekAhead(i + 1) == option[i]);
        }

        // If requireTrailingNonAN check next char is not alphanumeric
        // This fixes issues with `sub length() {...}` being translated to NAME(SUB) OP(LE) NAME(GTH) ...
        if (match && (!requireTrailingNonAN || !isalnum(this->peekAhead((int) option.length() + 1)))) {
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
    if (isalnum(nextChar)) return false;

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

std::string Tokeniser::matchStringLiteral(char delim, bool includeDelim) {
    std::string contents;
    if (this->peek() == delim || !includeDelim) {
        if (includeDelim) this->nextChar();
        while (this->peek() != EOF) {
            if (this->peek() == '\\') {
                if (this->peekAhead(2) == delim) {
                    // Escaped deliminator (e.g. \")
                    contents += '\\';
                    contents += delim;
                    this->nextChar();
                    this->nextChar();
                    continue;
                } else if (this->peekAhead(2) == '\\') {
                    // Escaped backslash (i.e. \\)
                    contents += '\\';
                    this->nextChar();
                    this->nextChar();
                    continue;
                }
            } else if (this->peek() == delim) {
                // Deliminator without escape => Done
                break;
            }
            contents += this->peek();
            this->nextChar();
            if (this->peek() == EOF) break;
        }

        if (includeDelim) {
            this->nextChar();
            contents = delim + contents + delim;
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

std::vector<Token> Tokeniser::matchLiteralBody(const std::string &quoteChars, FilePos start, char matchedQuoteChar) {
    std::vector<Token> tokens;
    char quoteChar;
    if (matchedQuoteChar == EOF) {
        auto whitespace = matchWhitespace();
        quoteChar = peek();
        nextChar();
        tokens.emplace_back(Token(TokenType::StringStart, start, quoteChars + whitespace + quoteChar));
    } else {
        quoteChar = matchedQuoteChar;
    }

    start = currentPos();
    std::string literal = (quoteChar == '{' || quoteChar == '(' || quoteChar == '<' || quoteChar == '[')
                          ? matchBracketedStringLiteral(quoteChar) : matchStringLiteral(quoteChar, false);
    tokens.emplace_back(Token(TokenType::String, start, literal));
    start = currentPos();
    auto endChar = peek();
    nextChar();
    tokens.emplace_back(Token(TokenType::StringEnd, start, std::string(1, endChar)));
    return tokens;
}

std::vector<Token> Tokeniser::matchQuoteLiteral() {
    auto start = currentPos();

    auto p1 = peek();
    auto p2 = peekAhead(2);
    std::string quoteChars;

    bool isMultipleLiteral = false;
    int offset = 1;

    if ((p1 == 'q' && p2 == 'q') || (p1 == 'q' && p2 == 'x') || (p1 == 'q' && p2 == 'w') || (p1 == 'q' && p2 == 'r')) {
        quoteChars = std::string(1, p1) + p2;
        offset += 2;
    } else if (p1 == 'q' || p1 == 'm') {
        quoteChars = std::string(1, p1);
        offset++;
    } else if (p1 == 's' || p1 == 'y') {
        isMultipleLiteral = true;
        offset++;
        quoteChars = std::string(1, p1);
    } else if (p1 == 't' && p2 == 'r') {
        quoteChars = "tr";
        isMultipleLiteral = true;
        offset += 2;
    } else {
        return std::vector<Token>();
    }

    std::vector<Token> tokens;
    std::string whitespace;
    while (isWhitespace(peekAhead(offset))) {
        whitespace += peekAhead(offset);
        offset++;
    }

    auto quoteChar = peekAhead(offset);

    if (isalnum(quoteChar)) {
        if (whitespace.empty()) {
            // Must have whitespace for alphanumeric quote char
            return std::vector<Token>();
        }
    }

    for (int i = 0; i < offset - 1; i++) this->nextChar();

    nextChar();
    tokens.emplace_back(Token(TokenType::StringStart, start, quoteChars + whitespace + quoteChar));

    start = currentPos();
    std::string literal = (quoteChar == '{' || quoteChar == '(' || quoteChar == '<' || quoteChar == '[')
                          ? matchBracketedStringLiteral(quoteChar) : matchStringLiteral(quoteChar, false);
    tokens.emplace_back(Token(TokenType::String, start, literal));
    start = currentPos();
    auto endChar = peek();
    nextChar();
    tokens.emplace_back(Token(TokenType::StringEnd, start, std::string(1, endChar)));

    if (isMultipleLiteral) {
        start = currentPos();
        literal = (quoteChar == '{' || quoteChar == '(' || quoteChar == '<' || quoteChar == '[')
                  ? matchBracketedStringLiteral(quoteChar) : matchStringLiteral(quoteChar, false);
        tokens.emplace_back(Token(TokenType::String, start, literal));
        start = currentPos();
        endChar = peek();
        nextChar();
        tokens.emplace_back(Token(TokenType::StringEnd, start, std::string(1, endChar)));
    }

    return tokens;

}


std::string Tokeniser::matchNumeric() {
    std::string testString;
    int i = 0;
    while (isalnum(peekAhead(i + 1)) || peekAhead(i + 1) == '.' || peekAhead(i + 1) == '+' ||
           peekAhead(i + 1) == '-') {
        testString += peekAhead(i + 1);
        i += 1;
    }
    if (testString.empty()) return "";
    // Do quick check before we use expensive regex
    if (!isnumber(testString[0]) && testString[0] != '+' && testString[0] != '-') return "";

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
            while (this->peek() != EOF) {
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
    if (isupper(this->peekAhead(i)) || islower(this->peekAhead(i)) || this->peekAhead(i) == '_') {
        // Valid starting
        i += 1;
        while (true) {
            int start = i;
            i += peekPackageTokens(i);
            while (isalnum(this->peekAhead(i)) || this->peekAhead(i) == '_') i += 1;

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
            if (isupper(second) || second == '[' || second == ']' || second == '^' || second == '_' ||
                second == '?' || second == '\\') {
                i += 2;
                goto done;
            }
        }

        if (isnumber(this->peekAhead(i))) {
            // Numeric variable
            i += 1;
            while (isnumber(this->peekAhead(i))) i += 1;
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
            while (isalnum(this->peekAhead(i))) i += 1;
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

    std::string var = this->program.substr(this->_position + 1, i - 1);
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

void Tokeniser::matchHeredDoc(std::vector<Token> &tokens) {
    if (this->peek() != '<' || this->peekAhead(2) != '<') return;

    std::string hereDocStart;
    // Possible heredoc.
    // Check if next token is a String(...) or Name(...)
    auto start = this->currentPos();
    this->nextChar();
    this->nextChar();
    tokens.emplace_back(Token(TokenType::Operator, start, "<<"));

    start = this->currentPos();
    auto preNameWhitespace = this->matchWhitespace();
    if (!preNameWhitespace.empty()) tokens.emplace_back(Token(TokenType::Whitespace, start, preNameWhitespace));
    // Now try to match tilde
    // tilde is a indented here doc
    // e.g. <<~OUT or << ~"Hello World"
    // Important as it affects the way we close the
    start = this->currentPos();
    auto hasTilde = this->peek() == '~';
    if (hasTilde) {
        this->nextChar();
        tokens.emplace_back(Token(TokenType::Operator, start, "~"));
    }

    auto stringLiteral = this->matchStringLiteral('"', true);
    if (stringLiteral.empty()) stringLiteral = this->matchStringLiteral('\'', true);
    if (!stringLiteral.empty()) {
        // Remove quotations from string body
        stringLiteral = stringLiteral.substr(1, stringLiteral.size() - 2);
    }
    // Note that a bareword Name(..) is only allowed in heredoc if not preceeded by whitespace:
    // <<OUT        OK
    // << OUT       NOT OK
    // <<"HellO"    OK
    // << "Hello"   OK
    if (stringLiteral.empty() && preNameWhitespace.empty()) {
        if ((isalnum(peek()) || peek() == '_')) {
            hereDocStart = this->matchName();
        } else {
            // Failed to match
            return;
        }
    } else {
        hereDocStart = stringLiteral;
    }

    if (hereDocStart.empty()) return;

    // Next continue to read tokens until end of current line
    std::vector<Token> lineRemainingTokens;
    while (lineRemainingTokens.empty() ||
           (lineRemainingTokens[lineRemainingTokens.size() - 1].type != TokenType::Newline &&
            lineRemainingTokens[lineRemainingTokens.size() - 1].type != TokenType::EndOfInput)) {
        nextTokens(lineRemainingTokens);
    }

    tokens.insert(tokens.end(), lineRemainingTokens.begin(), lineRemainingTokens.end());

    // Now read newlines until we reach the ending terminator on a separate line
    start = currentPos();
    FilePos bodyEnd;
    FilePos lineStart;
    std::string hereDocContents;
    std::string line;
    while (this->peek() != EOF) {
        if (this->peek() == '\n') {
            if (line == hereDocStart) {
                break;
            } else if (hasTilde) {
                // Supports any number of whitespace before string then a newline
                for (int i = 0; i < (int) line.size(); i++) {
                    if (isWhitespace(line[i])) continue;
                    auto nonWhitespacePart = line.substr(i, line.size() - i);
                    if (nonWhitespacePart == hereDocStart) goto done;
                    else break;
                }
            } else {
                bodyEnd = currentPos();
            }

            line += '\n';
            bodyEnd = currentPos();
            this->nextChar();
            hereDocContents += line;
            line = "";
            lineStart = currentPos();
        } else {
            line += this->peek();
            this->nextChar();
        }
    }

    done:
    // Finally add our heredoc token
    tokens.emplace_back(Token(TokenType::HereDoc, start, bodyEnd, hereDocContents));
    tokens.emplace_back(Token(TokenType::HereDocEnd, lineStart, line));
    return;
}

void Tokeniser::nextTokens(std::vector<Token> &tokens, bool enableHereDoc) {
    // Position before anything is consumed
    auto startPos = currentPos();

    // End program at EOF, 0x04 (^D) or 0x1A (^Z)
    if (this->peek() == EOF || this->peek() == '\x04' || this->peek() == '\x1a') {
        tokens.emplace_back(Token(TokenType::EndOfInput, startPos, startPos));
        return;
    }

    // Search for __DATA__ and end program if reached
    if (!this->matchStringOption(std::vector<std::string>{"__DATA__", "__END__"}, true).empty()) {
        tokens.emplace_back(Token(TokenType::EndOfInput, startPos, startPos));
        return;
    }

    // Devour any whitespace
    std::string whitespace = matchWhitespace();
    if (whitespace.length() > 0) {
        tokens.emplace_back(Token(TokenType::Whitespace, startPos, whitespace));
        return;
    }

    // Now for newlines
    if (this->peek() == '\n') {
        // Linux/mac newline
        this->nextChar();
        tokens.emplace_back(Token(TokenType::Newline, startPos, "\n"));
        return;
    } else if (this->peek() == '\r' && this->peekAhead(2) == '\n') {
        // windows
        this->nextChar();
        this->nextChar();
        tokens.emplace_back(Token(TokenType::Newline, startPos, "\r\n"));
        return;
    } else if (this->peek() == '\r') {
        this->nextChar();
        tokens.emplace_back(Token(TokenType::Newline, startPos, "\r"));
        return;
    }

    auto var = this->matchVariable();
    if (!var.empty()) {
        auto type = TokenType::HashVariable;
        if (var[0] == '$') type = TokenType::ScalarVariable;
        if (var[0] == '@') type = TokenType::ArrayVariable;
        tokens.emplace_back(Token(type, startPos, var));
        return;
    }


    if (enableHereDoc && peek() == '<' && peekAhead(2) == '<') {
        matchHeredDoc(tokens);
        return;
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


    if (!isalnum(this->peek())) {
        std::string op = matchStringOption(operators);
        if (!op.empty()) {
            tokens.emplace_back(Token(TokenType::Operator, startPos, op));
            return;
        }
    }


    std::string op2 = matchStringOption(wordOperators, true);
    if (!op2.empty()) {
        tokens.emplace_back(Token(TokenType::Operator, startPos, op2));
        return;
    }

    // Now consider the really easy single character tokens
    char peek = this->peek();

    if (peek == ';') {
        this->nextChar();
        tokens.emplace_back(Token(TokenType::Semicolon, startPos, startPos.col));
        return;
    }
    if (peek == ',') {
        this->nextChar();
        tokens.emplace_back(Token(TokenType::Comma, startPos, startPos.col));
        return;
    }
    if (peek == '{') {
        this->nextChar();
        tokens.emplace_back(Token(TokenType::LBracket, startPos, startPos.col));
        return;
    }
    if (peek == '}') {
        this->nextChar();
        tokens.emplace_back(Token(TokenType::RBracket, startPos, startPos.col));
        return;
    }
    if (peek == '(') {
        this->nextChar();
        tokens.emplace_back(Token(TokenType::LParen, startPos, startPos.col));
        return;
    }
    if (peek == ')') {
        this->nextChar();
        tokens.emplace_back(Token(TokenType::RParen, startPos, startPos.col));
        return;
    }
    if (peek == '[') {
        this->nextChar();
        tokens.emplace_back(Token(TokenType::LSquareBracket, startPos, startPos.col));
        return;
    }
    if (peek == ']') {
        this->nextChar();
        tokens.emplace_back(Token(TokenType::RSquareBracket, startPos, startPos.col));
        return;
    }
    if (peek == '.') {
        this->nextChar();
        tokens.emplace_back(Token(TokenType::Dot, startPos, startPos.col + 1));
        return;
    }

    // Program keywords
    auto tryKeyword = tryMatchKeywords(startPos);
    if (tryKeyword.has_value()) {
        tokens.emplace_back(tryKeyword.value());
        return;
    }

    auto numeric = this->matchNumeric();
    if (!numeric.empty()) {
        tokens.emplace_back(Token(TokenType::NumericLiteral, startPos, numeric));
        return;
    }

    // Numeric first
    if (this->peek() == '-') {
        this->nextChar();
        tokens.emplace_back(Token(TokenType::Operator, startPos, "-"));
        return;
    }

    auto pod = this->matchPod();
    if (!pod.empty()) {
        // One of the few tokens that can span multiple lines
        tokens.emplace_back(Token(TokenType::Pod, startPos, FilePos(this->currentLine, this->currentCol - 1), pod));
        return;
    }

    // POD takes priority
    if (this->peek() == '=') {
        this->nextChar();
        tokens.emplace_back(Token(TokenType::Assignment, startPos, startPos.col));
        return;
    }

    // This one is tricky
    // Could be a division (34 / 5) OR a bare regex literal (/354/)/
    // Guess based on previous non-whitespace token
    // TODO improve this heuristic
    if (this->peek() == '/') {
        if (tokens.empty()) {
            auto string = this->matchString();
            if (!string.empty()) {
                tokens.emplace_back(Token(TokenType::String, startPos, string));
                return;
            }
        }
        int i = (int) tokens.size() - 1;
        Token prevToken = tokens[i];
        while (i > 0 && prevToken.isWhitespaceNewlineOrComment()) {
            prevToken = tokens[--i];
        }

        auto type = prevToken.type;
        if (type == TokenType::RParen || type == TokenType::NumericLiteral || type == TokenType::ScalarVariable ||
            type == TokenType::HashVariable || type == TokenType::ArrayVariable || type == TokenType::RBracket ||
            type == TokenType::RSquareBracket) {
            this->nextChar();
            tokens.emplace_back(Token(TokenType::Operator, startPos, "/"));
            return;
        } else {
            auto string = this->matchString();
            if (!string.empty()) {
                tokens.emplace_back(Token(TokenType::String, startPos, string));
                return;
            }
        }
    }

    auto string = this->matchString();
    if (!string.empty()) {
        tokens.emplace_back(Token(TokenType::String, startPos, string));
        return;
    }

    // String literals / quote literals / transliterations / ...
    auto quoteTokens = matchQuoteLiteral();
    if (!quoteTokens.empty()) {
        tokens.insert(tokens.end(), quoteTokens.begin(), quoteTokens.end());
        return;
    }

    // Numeric first
    if (this->peek() == '/') {
        this->nextChar();
        tokens.emplace_back(Token(TokenType::Operator, startPos, "/"));
        return;
    }


    auto comment = this->matchComment();
    if (!comment.empty()) {
        tokens.emplace_back(Token(TokenType::Comment, startPos, comment));
        return;
    }

    auto name = this->matchName();
    if (!name.empty()) {
        tokens.emplace_back(Token(TokenType::Name, startPos, name));
        return;
    }

    throw TokeniseException(std::string("Remaining code exists"));
}

std::vector<Token> Tokeniser::tokenise() {
    std::vector<Token> tokens;

    while (tokens.empty() || tokens[tokens.size() - 1].type != TokenType::EndOfInput) {
        nextTokens(tokens);
    }

    secondPass(tokens);
    return tokens;
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


void Tokeniser::secondPassSub(std::vector<Token> &tokens, int &i) {
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
    if (nextToken.type == TokenType::LBracket) return;

    // Next token is a '(' (for signature or proto) or ':' (for attributes)
    auto isProtoOrSig =
            nextToken.type == TokenType::LParen || (nextToken.type == TokenType::Operator && nextToken.data == ":");
    if (!isProtoOrSig) return;

    FilePos start = nextToken.startPos;
    auto finalTokenPos = nextToken.endPos;

    int retokeniseStartIdx = i + 1;
    while (i < tokens.size() - 1 && nextToken.type != TokenType::LBracket) {
        nextToken = tokens[++i];
    }

    int retokeniseEndIdx = i;
    if (retokeniseEndIdx <= retokeniseStartIdx) return;
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

void Tokeniser::secondPassHash(std::vector<Token> &tokens, int &i) {
    TokenIterator tokenIterator(tokens, std::vector<TokenType>{TokenType::Whitespace, TokenType::Comment, TokenType::Whitespace}, i);
    Token token = tokenIterator.next();
    if (!isVariable(token.type)) return;
    token = tokenIterator.next();
    if (token.type != TokenType::LBracket) return;
    int lBracketOffset = tokenIterator.getIndex() - 1;

    // Expect comma separated list of variable|string|name
    while (token.type != TokenType::RBracket && token.type != TokenType::EndOfInput) {
        // Read variable, name or string
        token = tokenIterator.next();
        if (token.type != TokenType::Name && token.type != TokenType::String && !isVariable(token.type)) return;
        token = tokenIterator.next();

        // Now a possible comma
        if (token.type != TokenType::Comma && token.type != TokenType::RBracket) return;
    }

    if (token.type != TokenType::RBracket) return;

    // Now just replace brackets
    tokens[lBracketOffset].type = TokenType::HashSubStart;
    tokens[tokenIterator.getIndex() -1 ].type = TokenType::HashSubEnd;
    i++;
}

void Tokeniser::secondPassHashReref(std::vector<Token> &tokens, int &i)  {
    TokenIterator tokenIterator(tokens, std::vector<TokenType>{TokenType::Whitespace, TokenType::Comment, TokenType::Whitespace}, i);
    Token token = tokenIterator.next();
    if (!isVariable(token.type)) return;
    token = tokenIterator.next();
    if (token.type != TokenType::Operator || token.data != "->") return;

    token = tokenIterator.next();
    if (token.type != TokenType::LBracket) return;
    int lBracketOffset = tokenIterator.getIndex() - 1;

    // Keep count of brackets to match
    int bracketCount = 1;
    token = tokenIterator.next();
    int rBracketOffset = tokenIterator.getIndex();

    while (bracketCount > 0 && token.type != TokenType::EndOfInput) {
        if (token.type == TokenType::LBracket) bracketCount++;
        if (token.type == TokenType::RBracket) {
            rBracketOffset = tokenIterator.getIndex() - 1;
            bracketCount--;
        }
        token = tokenIterator.next();
    }

    if (bracketCount > 0) return;
    tokens[lBracketOffset].type = TokenType::HashDerefStart;
    tokens[rBracketOffset].type = TokenType::HashDerefEnd;
    i++;
}


// Second pass to fix any tokenization errors with a little bit of context
// Note this is fixing errors, not doing any parsing
void Tokeniser::secondPass(std::vector<Token> &tokens) {
    if (!doSecondPass) return;
    for (int i = 0; i < (int) tokens.size() - 1; i++) {
        if (tokens[i].type == TokenType::Sub) {
            secondPassSub(tokens, i);
        } else if (isVariable(tokens[i].type)) {
            int prevI = i;
            secondPassHash(tokens, i);
            i = prevI;
            secondPassHashReref(tokens, i);
        }
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
    return FilePos(this->currentLine, this->currentCol, this->_position + 1 + this->positionOffset);
}


KeywordConfig::KeywordConfig(const std::string &code, TokenType type) : code(code), type(type) {}

QuotedStringLiteral::QuotedStringLiteral(FilePos start, FilePos end, std::string literal) {
    this->literalStart = start;
    this->literalEnd = end;
    this->contents = literal;
}
