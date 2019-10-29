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
           c != ')' && c != '[' && c != ']' && c != ':' && c != '=' && c != '"';
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

std::string Tokeniser::matchStringContainingOnlyLetters(std::string letters) {
    std::string str;
    while (letters.find(peek()) != std::string::npos) {
        str += peek();
        nextChar();
    }

    return str;
}


std::string Tokeniser::matchString() {
    std::string contents;
    auto doubleStr = matchStringLiteral('"');
    if (!doubleStr.empty()) return doubleStr;
    auto singleStr = matchStringLiteral('\'');
    if (!singleStr.empty()) return singleStr;
    auto regexStr = matchStringLiteral('/');
    if (!regexStr.empty()) return regexStr;
    auto executeString = matchStringLiteral('`');
    return executeString;
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

        if (this->peek() == '\\') {
            if (this->peekAhead(2) == bracket) {
                contents += '\\';
                contents += bracket;
                this->nextChar();
                this->nextChar();
                continue;
            } else if (this->peekAhead(2) == endBracket) {
                // Escaped end bracket (e.g. `\}`)
                contents += '\\';
                contents += endBracket;
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
        } else if (this->peek() == endBracket) {
            bracketCount--;
        } else if (this->peek() == bracket) {
            bracketCount++;
        }

        if (bracketCount == 0) return contents;
        contents += peek();
        nextChar();
    }

    return contents;
}

void Tokeniser::matchDelimString(std::vector<Token> &tokens) {
    char quoteChar = this->peek();
    auto start = currentPos();
    this->nextChar();
    std::string contents;
    if (quoteChar == '{' || quoteChar == '(' || quoteChar == '<' || quoteChar == '[') {
        tokens.emplace_back(Token(TokenType::StringStart, start, std::string(1, quoteChar)));
        start = currentPos();
        contents = matchBracketedStringLiteral(quoteChar);
        if (!contents.empty()) {
            auto endPos = currentPos();
            endPos.position -= 1;
            endPos.col = endPos.col == 0 ? 0 : endPos.col - 1;   // FIXME
            tokens.emplace_back(Token(TokenType::String, start, endPos, contents));
        }
        start = currentPos();
        auto endChar = peek();
        nextChar();
        tokens.emplace_back(Token(TokenType::StringEnd, start, std::string(1, endChar)));
    } else {
        tokens.emplace_back(Token(TokenType::StringStart, start, std::string(1, quoteChar)));
        start = currentPos();
        contents = matchStringLiteral(quoteChar, false);
        if (!contents.empty()) {
            auto endPos = currentPos();
            endPos.position -= 1;
            endPos.col = endPos.col == 0 ? 0 : endPos.col - 1;   // FIXME
            tokens.emplace_back(Token(TokenType::String, start, endPos, contents));
        }
        start = currentPos();
        auto endChar = peek();
        nextChar();
        tokens.emplace_back(Token(TokenType::StringEnd, start, std::string(1, endChar)));
    }
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
    } else if (quoteChar == '_') {
        // Banned quote characters
        return std::vector<Token>();
    }

    start = currentPos();
    auto quoteIdent = this->matchStringOption(std::vector<std::string>{quoteChars});
    tokens.emplace_back(Token(TokenType::QuoteIdent, start, quoteIdent));

    start = currentPos();
    whitespace = this->matchWhitespace();
    if (!whitespace.empty()) tokens.emplace_back(Token(TokenType::Whitespace, start, whitespace));

    matchDelimString(tokens);
    if (isMultipleLiteral) {

        if (quoteChar == '{' || quoteChar == '(' || quoteChar == '<' || quoteChar == '[') {
            // If first block is brackets, then allow for whitespace
            start = currentPos();
            whitespace = matchWhitespace();
            if (!whitespace.empty()) tokens.emplace_back(Token(TokenType::Whitespace, start, whitespace));

            // Now we can match something completly new
            matchDelimString(tokens);
        } else {
            // Match more string then followed by ending string
            start = currentPos();
            std::string contents = matchStringLiteral(quoteChar, false);
            auto pos = currentPos();
            pos.position -= 1;
            pos.col = pos.col == 0 ? 0 : pos.col - 1;   // FIXME

            if (!contents.empty()) {
                auto endPos = currentPos();
                endPos.position -= 1;
                endPos.col = endPos.col == 0 ? 0 : endPos.col - 1;   // FIXME
                tokens.emplace_back(Token(TokenType::String, start, endPos, contents));
            }
            tokens.emplace_back(Token(TokenType::StringEnd, currentPos(), std::string(1, quoteChar)));
            nextChar();
        }
    }
    // Finally to match ending part of string for certain types
    std::string modifiers;
    start = currentPos();
    if (p1 == 's') {
        modifiers = matchStringContainingOnlyLetters("msixpodualngcer");
    }
    if (p1 == 'm') {
        modifiers = matchStringContainingOnlyLetters("msixpodualngc");
    }
    if (p1 == 'q' && p2 == 'r') {
        modifiers = matchStringContainingOnlyLetters("msixpodualn");
    }
    if (p1 == 't' && p2 == 'r') {
        modifiers = matchStringContainingOnlyLetters("cdsr");
    }
    if (p1 == 'y') {
        modifiers = matchStringContainingOnlyLetters("cdsr");
    }

    if (!modifiers.empty()) tokens.emplace_back(Token(TokenType::StringModifiers, start, modifiers));
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
        if (this->peek() == '=' && !isWhitespace(this->peekAhead(2))) {
            // Yes
            pod += this->nextChar();

            // Consume until end of line (we can't start and end POD on same line)
            pod += this->getWhile(isNewline);

            // Now consume until ending
            while (this->peek() != EOF) {
                char c0 = this->peek();
                char c1 = this->peekAhead(2);
                char c2 = this->peekAhead(3);
                char c3 = this->peekAhead(4);
                char c4 = this->peekAhead(5);
                pod += this->nextChar();
                if (c0 == '\n' && c1 == '=' && c2 == 'c' && c3 == 'u' && c4 == 't') {
                    this->nextChar();
                    this->nextChar();
                    this->nextChar();
                    this->nextChar();
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
    if (peekAhead(i) == '$' && peekAhead(i + 1) == '#') {
        // Get first element of the array
        i++;
    }
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

// This is defined https://perldoc.perl.org/perldata.html#Identifier-parsing
// TODO can normal regex be faster?
std::string Tokeniser::matchBasicIdentifier(int &i) {
    // First char can't be a number
    if (isnumber(this->peekAhead(i))) return "";
    // Now we can just match alpha numerics
    std::string contents;
    while (isNameBody(this->peekAhead(i))) contents += this->peekAhead(i++);
    return contents;
}

// https://perldoc.perl.org/perldata.html#Identifier-parsing
// Matches identifiers with packages
// No sigil though (so not a variable/glob)
std::string Tokeniser::doMatchNormalIdentifier(int &i) {
    std::string contents;
    // (?: :: )* '?
    while (peekAhead(i) == ':') {
        // Only match colons in groups of 2
        if (peekAhead(i + 1) != ':') return contents;
        contents += "::";
        i += 2;
    }

    if (peekAhead(i) == '\'') {
        contents += '\'';
        i++;
    }

    // (?&basic_identifier)
    auto basic = matchBasicIdentifier(i);
    if (basic.empty()) {
        return contents;
    }

    contents += basic;

    while (peekAhead(i) == ':') {
        if (peekAhead(i + 1) != ':') {
            return contents;
        }
        contents += "::";
        i += 2;
    }

    if (peekAhead(i) == '\'') {
        contents += '\'';
        i++;
    }

    auto normalRecurse = doMatchNormalIdentifier(i);
    contents += normalRecurse;
    while (peekAhead(i) == ':') {
        if (peekAhead(i + 1) != ':') {
            return contents;
        }
        contents += "::";
        i += 2;
    }

    return contents;
}

std::string Tokeniser::matchIdentifier() {
    int i = 1;
    auto ident = doMatchNormalIdentifier(i);
    this->advancePositionSameLine(i - 1);
    return ident;
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

void Tokeniser::matchHereDocBody(std::vector<Token> &tokens, const std::string &hereDocDelim, bool hasTilde) {
    auto start = currentPos();
    FilePos bodyEnd = currentPos();
    FilePos lineStart;
    std::string hereDocContents;
    std::string line;
    while (this->peek() != EOF) {
        if (this->peek() == '\n' || this->peek() == '\r') {
            if (line == hereDocDelim) {
                break;
            } else if (hasTilde) {
                // Supports any number of whitespace before string then a newline
                for (int i = 0; i < (int) line.size(); i++) {
                    if (isWhitespace(line[i])) continue;
                    auto nonWhitespacePart = line.substr(i, line.size() - i);
                    if (nonWhitespacePart == hereDocDelim) goto done;
                    else break;
                }
            }

            bodyEnd = currentPos();
            if (this->peek() == '\n') {
                line += '\n';
                this->nextChar();
            } else if (this->peek() == '\r' && this->peek() == '\n') {
                line += "\r\n";
                this->nextChar();
                this->nextChar();
            } else if (this->peek() == '\r') {
                line += '\r';
                this->nextChar();
            }
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
    if (!hereDocContents.empty()) tokens.emplace_back(Token(TokenType::HereDoc, start, bodyEnd, hereDocContents));
    tokens.emplace_back(Token(TokenType::HereDocEnd, lineStart, line));
}

bool Tokeniser::matchSlashString(std::vector<Token> &tokens) {
    auto start = this->currentPos();
    auto string = this->matchString();
    if (!string.empty()) {
        tokens.emplace_back(Token(TokenType::String, start, string));
        auto start = currentPos();
        auto modifiers = this->matchStringContainingOnlyLetters("msixpodualngc");
        if (!modifiers.empty()) {
            tokens.emplace_back(Token(TokenType::StringModifiers, start, modifiers));
        }
        return true;
    }

    return false;
}

bool Tokeniser::matchNewline(std::vector<Token> &tokens) {
    auto start = this->currentPos();
    if (this->peek() == '\n') {
        // Linux/mac newline
        this->nextChar();
        tokens.emplace_back(Token(TokenType::Newline, start, "\n"));
        return true;
    } else if (this->peek() == '\r' && this->peekAhead(2) == '\n') {
        // windows
        this->nextChar();
        this->nextChar();
        tokens.emplace_back(Token(TokenType::Newline, start, "\r\n"));
        return true;
    } else if (this->peek() == '\r') {
        this->nextChar();
        tokens.emplace_back(Token(TokenType::Newline, start, "\r"));
        return true;
    }

    return false;
}

// Match {...}, but the middle can be a bare word
void Tokeniser::matchDereferenceBrackets(std::vector<Token> &tokens) {
    if (peek() != '{') return;
    auto start = currentPos();
    nextChar();
    tokens.emplace_back(Token(TokenType::HashDerefStart, start, "{"));

    // Next consider whitespace
    start = currentPos();
    auto whitespace = matchWhitespace();
    if (!whitespace.empty()) tokens.emplace_back(Token(TokenType::Whitespace, start, whitespace));

    // Now see if we can match Name bareword until following }
    int offset = 1;
    while (isNameBody(peekAhead(offset))) offset++;

    // Consume any whitespace
    while (isWhitespace(peekAhead(offset))) offset++;
    if (peekAhead(offset) == '}') {
        // Is a name!
        start = currentPos();
        auto name = this->matchName();
        tokens.emplace_back(Token(TokenType::Name, start, name));

        start = currentPos();
        whitespace = matchWhitespace();
        if (!whitespace.empty()) tokens.emplace_back(Token(TokenType::Whitespace, start, whitespace));

        start = currentPos();
        nextChar();
        tokens.emplace_back(Token(TokenType::HashDerefEnd, start, "}"));
        return;
    }

    // Otherwise just consume tokens (now bareword tokens)
    while (tokens[tokens.size() - 1].type != TokenType::HashDerefEnd &&
           tokens[tokens.size() - 1].type != TokenType::RBracket &&
           tokens[tokens.size() - 1].type != TokenType::EndOfInput) {
        this->nextTokens(tokens, true);
    }

    if (tokens[tokens.size() - 1].type == TokenType::RBracket) {
        tokens[tokens.size() - 1].type = TokenType::HashDerefEnd;
    }
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
    if (matchNewline(tokens)) {
        // Check this line for a possible heredoc starter
        for (int idx = tokens.size() - 2; idx >= 0; idx--) {
            int i = idx;
            if (tokens[i].type == TokenType::Newline) break;
            if (tokens[i].type == TokenType::Operator && tokens[i].data == "<<") {
                // got a <<, see if it could be a heredoc
                int heredocStartIdx = i;
                bool hasWhitespace = i + 1 < tokens.size() && tokens[i + 1].type == TokenType::Whitespace;
                if (hasWhitespace) i++;

                bool hasTilde =
                        i + 1 < tokens.size() && tokens[i + 1].type == TokenType::Operator && tokens[i + 1].data == "~";
                if (hasTilde) i++;

                // Now consider if we have ok identifier
                i++;
                if (i >= tokens.size() - 1) continue;
                if (tokens[i].type != TokenType::Name && tokens[i].type != TokenType::String) {
                    continue;
                } else {
                    std::string delim;
                    if (tokens[i].type == TokenType::Name && hasWhitespace) continue;    // Now allowed with barewords
                    // Now finally we can confirm a valid heredoc.
                    if (tokens[i].type == TokenType::Name) delim = tokens[i].data;
                    else if (tokens[i].type == TokenType::String)
                        delim = tokens[i].data.substr(1,
                                                      tokens[i].data.size() -
                                                      2);
                    matchHereDocBody(tokens, delim, hasTilde);
                    break;
                }
            }
        }

        return;
    }

    // Consider special variables
    // TODO fill this our more
    // $$ (returns PID) does mess with parser if not handled properly now
    if (this->peek() == '$' && this->peekAhead(2) == '$' && !isNameBody(this->peekAhead(3))) {
        auto start = this->currentPos();
        this->nextChar();
        this->nextChar();
        tokens.emplace_back(Token(TokenType::ScalarVariable, startPos, "$$"));
        return;
    }
    // Consider dereference before variable
    if (this->peek() == '$' || this->peek() == '@' || this->peek() == '%') {
        int i = 2;
        while (isWhitespace(this->peekAhead(i))) i++;
        if (this->peekAhead(i) == '$' || this->peekAhead(i) == '@' || this->peekAhead(i) == '%') {
            // We have double sigil => deref
            // TODO check that this could not be valid variable
            auto pos = this->currentPos();
            auto sigil = this->peek();
            this->nextChar();
            tokens.emplace_back(Token(TokenType::Deref, pos, std::string(1, sigil)));
            return;
        }
    }

    auto var = this->matchVariable();
    if (!var.empty()) {
        auto type = TokenType::HashVariable;
        if (var[0] == '$') type = TokenType::ScalarVariable;
        if (var[0] == '@') type = TokenType::ArrayVariable;
        tokens.emplace_back(Token(type, startPos, var));
        return;
    }

    if (this->peek() == '-' && !isalnum(peekAhead(3))) {
        auto testChar = this->peekAhead(2);

        if (testChar == 'r' || testChar == 'x' || testChar == 'w' || testChar == 'o' || testChar == 'R' ||
            testChar == 'W' || testChar == 'X' || testChar == 'O' || testChar == 'e' || testChar == 'z' ||
            testChar == 's' || testChar == 'f' || testChar == 'd' || testChar == 'l' || testChar == 'p' ||
            testChar == 'S' || testChar == 'b' || testChar == 'c' || testChar == 't' || testChar == 'u' ||
            testChar == 'g' || testChar == 'k' || testChar == 'T' || testChar == 'B' || testChar == 'M' ||
            testChar == 'A' || testChar == 'C') {
            this->nextChar();
            this->nextChar();
            tokens.emplace_back(Token(TokenType::FileTest, startPos, "-" + std::string(1, testChar)));
            return;
        }
    }

    // Perl has so many operators...
    // Thankfully we don't actually care what the do, just need to recognise them
    // TODO complete this list
    auto operators = std::vector<std::string>{
            "+=", "++", "+", "--", "-=", "**=", "*=", "**", "*", "!=", "!~", "!", "~", "\\", "==", "=~",
            "//=", "=>", "//", "%=", "%", "x=", "x", ">>=", ">>", ">", ">=", "<=>", "<<=", "<<", "<",
            ">=",
            "~~", "&=", "&.=", "&&=", "&&", "&", "||=", "|.=", "|=", "||",
            "~", "^=", "^.=", "^", "...", "..", "?:", ":", ".=", "?"
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

    // Dereference, check for possible Name on other wise
    if (peek == '-' && this->peekAhead(2) == '>') {
        this->nextChar();
        this->nextChar();
        tokens.emplace_back(Token(TokenType::Operator, startPos, "->"));

        // Add whitespace tokem
        auto start = this->currentPos();
        whitespace = this->matchWhitespace();
        if (!whitespace.empty()) tokens.emplace_back(Token(TokenType::Whitespace, start, whitespace));

        start = this->currentPos();
        auto name = this->matchName();
        if (!name.empty()) tokens.emplace_back(Token(TokenType::Name, start, name));
        return;
    }


    if (peek == '{') {
        if (!tokens.empty()) {
            int i = (int) tokens.size() - 1;
            auto prevType = tokens[i].type;
            while (i > 0 && (prevType == TokenType::Whitespace || prevType == TokenType::Newline ||
                             prevType == TokenType::Comment)) {
                i--;
                prevType = tokens[i].type;
            }

            if (isVariable(prevType) || (prevType == TokenType::Operator && tokens[i].data == "->")) {
                // So we have something like %x{...} or $x->{...} Contents of brackets can contain unquoted barewords
                // Note we also want to support multiple bareword seconds
                while (this->peek() == '{') {
                    // Match first set
                    matchDereferenceBrackets(tokens);

                    // Match any following whitespace
                    auto start = currentPos();
                    whitespace = this->matchWhitespace();
                    if (!whitespace.empty()) tokens.emplace_back(Token(TokenType::Whitespace, start, whitespace));
                }

                return;
            }
        }

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
        tokens.emplace_back(Token(TokenType::Dot, startPos, startPos.col));
        return;
    }

    // Consider => operator. If we have some Name them this will be quoted
    // Must be done here to prevent confusion with keywords/quoted operators
    int offset = 1;
    if (this->peek() == '_' || islower(this->peek())) {
        offset++;
        while (isalnum(peekAhead(offset)) || peekAhead(offset) == '_') offset++;
        while (isWhitespace(peekAhead(offset))) offset++;
        if (peekAhead(offset) == '=' && peekAhead(offset + 1) == '>') {
            auto start = currentPos();
            auto name = this->matchName();
            tokens.emplace_back(Token(TokenType::Name, start, name));

            start = currentPos();
            auto preArrowWhitespace = this->matchWhitespace();
            if (!preArrowWhitespace.empty())
                tokens.emplace_back(Token(TokenType::Whitespace, start, preArrowWhitespace));

            start = currentPos();
            this->nextChar();
            this->nextChar();
            tokens.emplace_back(Token(TokenType::Operator, start, "=>"));
            return;
        }
    }

    // Program keywords
    auto tryKeyword = tryMatchKeywords(startPos);
    if (tryKeyword.has_value()) {
        tokens.emplace_back(tryKeyword.value());

        // If we have matched a subroutine then we want to match it's tokens
        if (tryKeyword.value().type == TokenType::Sub) {
            matchSubroutine(tokens);
        }

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
        auto prevTokenTypeOption = previousNonWhitespaceToken(tokens);

        if (!prevTokenTypeOption.has_value()) {
            if (this->matchSlashString(tokens)) return;
        } else {
            auto prevTokenType = prevTokenTypeOption.value().type;
            TokenType secondType = TokenType::EndOfInput;
            std::string secondData = "";

            if (prevTokenType == TokenType::Name) {
                // See if it is a direct dereference (e.g ->name)
                int i = (int) tokens.size() - 1;
                while (i > 0) {
                    if (tokens[i].type == TokenType::Whitespace || tokens[i].type == TokenType::Name) {
                        i--;
                        continue;
                    }

                    secondType = tokens[i].type;
                    secondData = tokens[i].data;
                    break;
                }
            }

            if (isVariable(prevTokenType) || prevTokenType == TokenType::RParen ||
                prevTokenType == TokenType::NumericLiteral || prevTokenType == TokenType::RBracket ||
                prevTokenType == TokenType::RSquareBracket || prevTokenType == TokenType::HashDerefEnd ||
                (secondType == TokenType::Operator && secondData == "->")) {
                this->nextChar();
                tokens.emplace_back(Token(TokenType::Operator, startPos, "/"));
                return;
            } else {
                if (this->matchSlashString(tokens)) return;
            }
        }
    }

    if (peek == '/' && peekAhead(2) == '=') {
        this->nextChar();
        tokens.emplace_back(Token(TokenType::Operator, startPos, startPos.col, "/="));
        return;
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

    auto ident = matchIdentifier();
    if (!ident.empty()) {
        // Also use a name here
        tokens.emplace_back(Token(TokenType::Name, startPos, ident));
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

bool Tokeniser::matchAttribute(std::vector<Token> &tokens) {
    // Read name of attribute
    if (this->peek() == ':') {
        tokens.emplace_back(Token(TokenType::AttributeColon, currentPos(), ":"));
        nextChar();
    }

    addWhitespaceToken(tokens);

    auto start = currentPos();
    auto attrName = matchName();
    if (attrName.empty()) return false;
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

    return true;
}

bool Tokeniser::matchAttributes(std::vector<Token> &tokens) {
    // Attributes must start with semicolon
    auto start = this->currentPos();
    auto whitespace = matchWhitespace();
    if (!whitespace.empty()) tokens.emplace_back(Token(TokenType::Whitespace, start, whitespace));

    if (peek() != ':') return false;
    tokens.emplace_back(Token(TokenType::AttributeColon, currentPos(), ":"));
    this->nextChar();

    // The newline check is just to prevent bad code from consuming the entire file
    while (peek() != '(' || peek() != '{' || peek() != EOF) {
        auto attrTokens = matchAttribute(tokens);
        if (!attrTokens) return true;   // We know there is an attribute as we have matched the colon

        start = this->currentPos();
        whitespace = matchWhitespace();
        if (!whitespace.empty()) tokens.emplace_back(Token(TokenType::Whitespace, start, whitespace));
    }

    return true;
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

    while (peek() != '}' && peek() != EOF && peek() != ')') {
        signature += peek();
        nextChar();
    }

    if (peek() == ')') {
        signature += ')';
        nextChar();
    }

    return signature;
}


bool Tokeniser::matchSignatureTokens(std::vector<Token> &tokens) {
    auto start = currentPos();
    auto signature = matchSignature();
    if (signature.empty()) return false;

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

    return true;
}


void Tokeniser::matchSubroutine(std::vector<Token> &tokens) {
    // Assumes sub keyword has already been matched
    // Try first to match a name i.e. sub <NAME>
    addWhitespaceToken(tokens);

    auto start = currentPos();
    auto name = this->matchIdentifier();
    if (!name.empty()) {
        tokens.emplace_back(Token(TokenType::SubName, start, name));
    }
    addWhitespaceToken(tokens);

    // No prototype/signature/attributes.
    if (peek() == '{') {
        start = currentPos();
        start = currentPos();
        this->nextChar();
        tokens.emplace_back(Token(TokenType::LBracket, start, "{"));
        return;
    }

    // Next token is a '(' (for signature or proto) or ':' (for attributes)
    // If not then we are done, return
    if (!(peek() == '(' || peek() ==  ':')) return;

    // Try to match some attributes
    auto attributesMatched = matchAttributes(tokens);
    addWhitespaceToken(tokens);

    if (attributesMatched) {
        // So we could next try to match a signature
        matchSignatureTokens(tokens);
    } else {
        // No attributes, try matching a prototype/signature
        auto startPos = currentPos();
        if (isPrototype()) {
            auto prototype = matchPrototype();
            if (!prototype.empty()) tokens.emplace_back(Token(TokenType::Prototype, startPos, prototype));
        } else {
            matchSignatureTokens(tokens);
        }

        // Finally try matching attributes
        matchAttributes(tokens);
    }
}

void Tokeniser::secondPassHash(std::vector<Token> &tokens, int &i) {
    TokenIterator tokenIterator(tokens, std::vector<TokenType>{TokenType::Whitespace, TokenType::Comment,
                                                               TokenType::Whitespace}, i);
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
    tokens[tokenIterator.getIndex() - 1].type = TokenType::HashSubEnd;
    i++;
}

void Tokeniser::addWhitespaceToken(std::vector<Token> tokens) {
    auto start = currentPos();
    auto whitespace = matchWhitespace();
    if (!whitespace.empty()) tokens.emplace_back(Token(TokenType::Whitespace, start, whitespace));
}

void Tokeniser::secondPassHashReref(std::vector<Token> &tokens, int &i) {
    TokenIterator tokenIterator(tokens, std::vector<TokenType>{TokenType::Whitespace, TokenType::Comment,
                                                               TokenType::Whitespace}, i);
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
         if (isVariable(tokens[i].type)) {
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


KeywordConfig::KeywordConfig(
        const std::string &code, TokenType
type) : code(code), type(type) {}


std::optional<Token> previousNonWhitespaceToken(const std::vector<Token> &tokens) {
    if (tokens.empty()) return std::optional<Token>();
    int i = tokens.size() - 1;
    while (i > 0 && isWhitespaceNewlineComment(tokens[i].type)) {
        i--;
    }

    if (isWhitespaceNewlineComment(tokens[i].type)) return std::optional<Token>();
    return std::optional<Token>(tokens[i]);
}