//
// Created by Ben Banerjee-Richards on 2019-08-19.
//

#include "Tokeniser.h"

static std::regex NUMERIC_REGEX(R"(^(\+|-)?((\d+|_)\.?(\d|_){0,}(e(\+|-)?(\d|_)+?)?|0x[\dabcdefABCDEF]+|0b[01]+|)$)");
static std::regex VERSION_REGEX(R"(v?\d([_|\.]?\d){0,})");

Tokeniser::Tokeniser(std::string perl, bool doSecondPass) {
    this->program = std::move(perl);
    this->doSecondPass = doSecondPass;

    this->keywordMap = {{"use",      TokenType::Use},
                        {"if",       TokenType::If},
                        {"else",     TokenType::Else},
                        {"elsif",    TokenType::ElsIf},
                        {"unless",   TokenType::Unless},
                        {"while",    TokenType::While},
                        {"until",    TokenType::Until},
                        {"for",      TokenType::For},
                        {"foreach",  TokenType::Foreach},
                        {"when",     TokenType::When},
                        {"do",       TokenType::Do},
                        {"next",     TokenType::Next},
                        {"redo",     TokenType::Redo},
                        {"last",     TokenType::Last},
                        {"my",       TokenType::My},
                        {"state",    TokenType::State},
                        {"local",    TokenType::Local},
                        {"our",      TokenType::Our},
                        {"break",    TokenType::Break},
                        {"continue", TokenType::Continue},
                        {"given",    TokenType::Given},
                        {"sub",      TokenType::Sub},
                        {"package",  TokenType::Package},
                        {"state",    TokenType::State},
                        {"use",      TokenType::Use},
                        {"require",  TokenType::Require}};

    // Built in perl functions, from https://perldoc.perl.org/5.30.0/index-functions.html
    this->builtinSubMap = {{"abs",              1},
                           {"accept",           1},
                           {"alarm",            1},
                           {"and",              1},
                           {"atan2",            1},
                           {"bind",             1},
                           {"binmode",          1},
                           {"bless",            1},
                           {"break",            1},
                           {"caller",           1},
                           {"chdir",            1},
                           {"chmod",            1},
                           {"chomp",            1},
                           {"chop",             1},
                           {"chown",            1},
                           {"chr",              1},
                           {"chroot",           1},
                           {"close",            1},
                           {"closedir",         1},
                           {"cmp",              1},
                           {"connect",          1},
                           {"continue",         1},
                           {"cos",              1},
                           {"crypt",            1},
                           {"__DATA__",         1},
                           {"dbmclose",         1},
                           {"dbmopen",          1},
                           {"default",          1},
                           {"defined",          1},
                           {"delete",           1},
                           {"die",              1},
                           {"do",               1},
                           {"dump",             1},
                           {"each",             1},
                           {"else",             1},
                           {"elseif",           1},
                           {"elsif",            1},
                           {"endgrent",         1},
                           {"endhostent",       1},
                           {"endnetent",        1},
                           {"endprotoent",      1},
                           {"endpwent",         1},
                           {"endservent",       1},
                           {"eof",              1},
                           {"eq",               1},
                           {"eval",             1},
                           {"evalbytes",        1},
                           {"exec",             1},
                           {"exists",           1},
                           {"exit",             1},
                           {"exp",              1},
                           {"fc",               1},
                           {"fcntl",            1},
                           {"fileno",           1},
                           {"flock",            1},
                           {"for",              1},
                           {"foreach",          1},
                           {"fork",             1},
                           {"format",           1},
                           {"formline",         1},
                           {"ge",               1},
                           {"getc",             1},
                           {"getgrent",         1},
                           {"getgrgid",         1},
                           {"getgrnam",         1},
                           {"gethostbyaddr",    1},
                           {"gethostbyname",    1},
                           {"gethostent",       1},
                           {"getlogin",         1},
                           {"getnetbyaddr",     1},
                           {"getnetbyname",     1},
                           {"getnetent",        1},
                           {"getpeername",      1},
                           {"getpgrp",          1},
                           {"getppid",          1},
                           {"getpriority",      1},
                           {"getprotobyname",   1},
                           {"getprotobynumber", 1},
                           {"getprotoent",      1},
                           {"getpwent",         1},
                           {"getpwnam",         1},
                           {"getpwuid",         1},
                           {"getservbyname",    1},
                           {"getservbyport",    1},
                           {"getservent",       1},
                           {"getsockname",      1},
                           {"getsockopt",       1},
                           {"given",            1},
                           {"glob",             1},
                           {"gmtime",           1},
                           {"goto",             1},
                           {"grep",             1},
                           {"gt",               1},
                           {"hex",              1},
                           {"INIT",             1},
                           {"if",               1},
                           {"import",           1},
                           {"index",            1},
                           {"int",              1},
                           {"ioctl",            1},
                           {"join",             1},
                           {"keys",             1},
                           {"kill",             1},
                           {"last",             1},
                           {"lc",               1},
                           {"lcfirst",          1},
                           {"le",               1},
                           {"length",           1},
                           {"link",             1},
                           {"listen",           1},
                           {"local",            1},
                           {"localtime",        1},
                           {"lock",             1},
                           {"log",              1},
                           {"lstat",            1},
                           {"lt",               1},
                           {"m",                1},
                           {"map",              1},
                           {"mkdir",            1},
                           {"msgctl",           1},
                           {"msgget",           1},
                           {"msgrcv",           1},
                           {"msgsnd",           1},
                           {"my",               1},
                           {"ne",               1},
                           {"next",             1},
                           {"no",               1},
                           {"not",              1},
                           {"oct",              1},
                           {"open",             1},
                           {"opendir",          1},
                           {"or",               1},
                           {"ord",              1},
                           {"our",              1},
                           {"pack",             1},
                           {"package",          1},
                           {"pipe",             1},
                           {"pop",              1},
                           {"pos",              1},
                           {"print",            1},
                           {"printf",           1},
                           {"prototype",        1},
                           {"push",             1},
                           {"q",                1},
                           {"qq",               1},
                           {"qr",               1},
                           {"quotemeta",        1},
                           {"qw",               1},
                           {"qx",               1},
                           {"rand",             1},
                           {"read",             1},
                           {"readdir",          1},
                           {"readline",         1},
                           {"readlink",         1},
                           {"readpipe",         1},
                           {"recv",             1},
                           {"redo",             1},
                           {"ref",              1},
                           {"rename",           1},
                           {"require",          1},
                           {"reset",            1},
                           {"return",           1},
                           {"reverse",          1},
                           {"rewinddir",        1},
                           {"rindex",           1},
                           {"rmdir",            1},
                           {"s",                1},
                           {"say",              1},
                           {"scalar",           1},
                           {"seek",             1},
                           {"seekdir",          1},
                           {"select",           1},
                           {"semctl",           1},
                           {"semget",           1},
                           {"semop",            1},
                           {"send",             1},
                           {"setgrent",         1},
                           {"sethostent",       1},
                           {"setnetent",        1},
                           {"setpgrp",          1},
                           {"setpriority",      1},
                           {"setprotoent",      1},
                           {"setpwent",         1},
                           {"setservent",       1},
                           {"setsockopt",       1},
                           {"shift",            1},
                           {"shmctl",           1},
                           {"shmget",           1},
                           {"shmread",          1},
                           {"shmwrite",         1},
                           {"shutdown",         1},
                           {"sin",              1},
                           {"sleep",            1},
                           {"socket",           1},
                           {"socketpair",       1},
                           {"sort",             1},
                           {"splice",           1},
                           {"split",            1},
                           {"sprintf",          1},
                           {"sqrt",             1},
                           {"srand",            1},
                           {"stat",             1},
                           {"state",            1},
                           {"study",            1},
                           {"sub",              1},
                           {"substr",           1},
                           {"symlink",          1},
                           {"syscall",          1},
                           {"sysopen",          1},
                           {"sysread",          1},
                           {"sysseek",          1},
                           {"system",           1},
                           {"syswrite",         1},
                           {"tell",             1},
                           {"telldir",          1},
                           {"tie",              1},
                           {"tied",             1},
                           {"time",             1},
                           {"times",            1},
                           {"tr",               1},
                           {"truncate",         1},
                           {"UNITCHECK",        1},
                           {"uc",               1},
                           {"ucfirst",          1},
                           {"umask",            1},
                           {"undef",            1},
                           {"unless",           1},
                           {"unlink",           1},
                           {"unpack",           1},
                           {"unshift",          1},
                           {"untie",            1},
                           {"until",            1},
                           {"use",              1},
                           {"utime",            1},
                           {"values",           1},
                           {"vec",              1},
                           {"wait",             1},
                           {"waitpid",          1},
                           {"wantarray",        1},
                           {"warn",             1},
                           {"when",             1},
                           {"while",            1},
                           {"write",            1},
                           {"-X",               1},
                           {"x",                1},
                           {"xor",              1}};

    // Remove any unicode Byte Order Mark (e.g. 0xEFBBBF)
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

/**
 * Gets and consumes next character from input
 * Also updates file position, line and column information
 * @return
 */
char Tokeniser::nextChar() {
    if (this->_position == this->program.length() - 1) {
        return EOF;
    }

    if ((this->peek() == '\r' && this->peekAhead(2) == '\n') ||
        (this->peek() == '\n' && this->peekAhead(0) != '\r')) {
        // Newline coming up, other code will handle token
        this->nextLine();
    } else {
        this->currentCol += 1;
    }

    this->_position += 1;
    return this->program[this->_position];
}

/**
 * Used for lookahead
 * @param i How far ahead to look. Note peek() = peekAhead(1). i = 0 is last char consumed
 * @return
 */
char Tokeniser::peekAhead(int i) {
    if (this->_position + i > this->program.length() - 1) {
        return EOF;
    }

    return this->program[this->_position + i];
}

void Tokeniser::backtrack(FilePos pos) {
    this->currentLine = pos.line;
    this->currentCol = pos.col;
    this->_position = pos.position - 1;
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
// TODO This is not comprehensive
bool Tokeniser::isNameBody(char c) {
    return c >= '!' && c != ';' && c != ',' && c != '>' && c != '<' && c != '-' && c != '.' && c != '{' &&
           c != '}' &&
           c != '(' &&
           c != ')' && c != '[' && c != ']' && c != ':' && c != '=' && c != '"' && c != '/';
}

/**
 * Given a list of possible strings, will try to match that string and consume the input
 * @param options
 * @param requireTrailingNonAN - Require a non alpha numeric character to follow the string
 * @return
 */
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

/**
 * Match any string, but it must contain only the given letters. Each letter given matched zero, one or many times
 * @param letters
 * @return
 */
std::string Tokeniser::matchStringContainingOnlyLetters(const std::string &letters) {
    std::string str;
    while (letters.find(peek()) != std::string::npos) {
        str += peek();
        nextChar();
    }

    return str;
}

/**
 * Match a normal string literal
 * e.g. "Hello", 'Hello', /Hello/ and `hello`
 * @return
 */
bool Tokeniser::matchSimpleString(std::vector<Token> &tokens) {
    if (peek() == '"' || peek() == '\'' || peek() == '/' || peek() == '`') {
        matchDelimString(tokens);
        return true;
    }

    return false;
}

/**
 * Match a string deliminated by any character. e.g. qq HWorld!H uses deliminator of 'H'
 * IMPORTANT does not support bracket deliminators with matching, use matchBracketedStringLiteral for that
 * Can support including deliminators in output
 * @param delim e.g. '"' or 'H'
 * @param includeDelim If true then current tokenizer state is at the deliminator (i.e. peek() == delim).
 *                     The output string will include both the start and end deliminator: HWorldW
 *                     If false, the tokenizer state is one char past the deliminator and will end at the
 *                     ending deliminator. The delims will not be included in the returned string: world
 * @return
 */
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

/**
 * Matches a bracked string literal, e.g. in qq {Hello {}} {Wor\{ld}
 * Supports matching of the brackets inside
 * IMPORTANT: The tokeniser state is one past the opening bracket. Yes this is confusing compared to the above functions.
 * No I don't want to risk changing it until more tests have been written.
 * @param bracket The starting bracket.
 * @return
 */
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

/**
 * Match any string that is deliminated, either by brackets or normal deliminators.
 * Higher level method over above deliminator matching ones that produces tokens
 * @param tokens
 */
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

std::string Tokeniser::matchQuoteOperator() {
    auto p1 = peek();
    auto p2 = peekAhead(2);

    if ((p1 == 'q' && p2 == 'q') || (p1 == 'q' && p2 == 'x') || (p1 == 'q' && p2 == 'w') ||
        (p1 == 'q' && p2 == 'r')) {
        this->nextChar();
        this->nextChar();
        return std::string(1, p1) + p2;
    } else if (p1 == 'q' || p1 == 'm') {
        this->nextChar();
        return std::string(1, p1);
    } else if (p1 == 's' || p1 == 'y') {
        this->nextChar();
        return std::string(1, p1);
    } else if (p1 == 't' && p2 == 'r') {
        this->nextChar();
        this->nextChar();
        return "tr";
    } else {
        return "";
    }
}


/**
 * Matches full quote literal, starting from qq, qw, y, tr, ...
 * Note that this DOES NOT include the normal quotes using //, "", '' or ``
 * @return
 */
bool Tokeniser::matchQuoteLiteral(std::vector<Token> &tokens) {
    int tokensSize = tokens.size(); // So we can backtrack to this
    auto startPos = currentPos();
    std::string quoteOperator = matchQuoteOperator();
    if (quoteOperator.empty()) return false;

    bool isMultipleLiteral = quoteOperator == "s" || quoteOperator == "y" || quoteOperator == "tr";
    tokens.emplace_back(Token(TokenType::QuoteIdent, startPos, quoteOperator));

    // Match whitespace, ignoring comments
    bool whitespaceEtcMatched = addNewlineWhitespaceCommentTokens(tokens, true);
    auto quoteChar = peek();

    if (isalnum(quoteChar)) {
        if (!whitespaceEtcMatched) {
            // Must have whitespace for alphanumeric quote char
            backtrack(startPos);
            tokens.erase(tokens.begin() + tokensSize, tokens.end());
            return false;
        }
    }

    auto start = currentPos();
    auto whitespace = this->matchWhitespace();
    if (!whitespace.empty()) tokens.emplace_back(Token(TokenType::Whitespace, start, whitespace));

    matchDelimString(tokens);
    if (isMultipleLiteral) {
        if (quoteChar == '{' || quoteChar == '(' || quoteChar == '<' || quoteChar == '[') {
            // If first block is brackets, then allow for whitespace and comments
            start = currentPos();
            whitespace = matchWhitespace();
            if (!whitespace.empty()) tokens.emplace_back(Token(TokenType::Whitespace, start, whitespace));
            addNewlineWhitespaceCommentTokens(tokens, true);

            // Now we can match something new
            matchDelimString(tokens);
        } else {
            // Match more string then followed by ending string
            start = currentPos();
            std::string contents = matchStringLiteral(quoteChar);
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
    if (quoteOperator == "s") {
        modifiers = matchStringContainingOnlyLetters("msixpodualngcer");
    }
    if (quoteOperator == "m") {
        modifiers = matchStringContainingOnlyLetters("msixpodualngc");
    }
    if (quoteOperator == "qr") {
        modifiers = matchStringContainingOnlyLetters("msixpodualn");
    }
    if (quoteOperator == "tr" || quoteOperator == "y") {
        modifiers = matchStringContainingOnlyLetters("cdsr");
    }

    if (!modifiers.empty()) tokens.emplace_back(Token(TokenType::StringModifiers, start, modifiers));
    return true;
}

/**
 * Simple numeric matching. Should be comprehensive for literals, uses regex after quick heuristic
 * @return
 */
std::string Tokeniser::matchNumeric() {
    std::string testString;
    int i = 0;
    while (isalnum(peekAhead(i + 1)) || peekAhead(i + 1) == '.' || peekAhead(i + 1) == '+' ||
           peekAhead(i + 1) == '-' || peekAhead(i + 1) == '_') {
        testString += peekAhead(i + 1);
        i += 1;
    }
    if (testString.empty()) return "";
    // Do quick check before we use expensive regex
    if (!isdigit(testString[0]) && testString[0] != '+' && testString[0] != '-') return "";

    std::smatch regexMatch;

    if (std::regex_match(testString, regexMatch, NUMERIC_REGEX)) {
        this->advancePositionSameLine(testString.size());
        return testString;
    }

    return "";
}

/**
 * Parse a version string e.g. v5.4, 5.23.4. Essentially simplier numeric literals but can start with 'v' and have
 * multiple dots.
 *
 * During parsing, it is likely that many version strings will actually parse as numeric literals (e.g. `5.4`). This
 * is fine as further analysis later on will note the context - version strings can only be used after `require` and
 * within `use`. Computing context for `use` is complex as `use Module List Version` is possible, so instead we just
 * do it later. There could be a bug where a non-numeric value is incorrectly identified as a version literal,
 * but I am yet to find it.
 * @return Version literal or empty string
 */
std::string Tokeniser::matchVersionString() {
    if (peek() != 'v' && !isdigit(peek())) return "";
    std::string versionString;
    int i = 0;
    if (peek() == 'v') {
        versionString += 'v';
        i++;
    }

    while (isdigit(peekAhead(i + 1)) || peekAhead(i + 1) == '.' || peekAhead(i + 1) == '_') {
        versionString += peekAhead(i + 1);
        i += 1;
    }

    std::smatch regexMatch;

    if (std::regex_match(versionString, regexMatch, VERSION_REGEX)) {
        this->advancePositionSameLine(versionString.size());
        return versionString;
    }

    return "";
}

std::string Tokeniser::matchComment() {
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

        // Strange edge case : $# usually used like $#array where array defined as @array
        // But this will also work with references
        // my @array = (1, 2, 3);
        // print $#array;   # Print 2
        // my $array_ref = \@array;
        // print $#$array_ref;   # Print 2 also!
        if (peekAhead(i + 1) == '$') i++;
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

        if (isdigit(this->peekAhead(i))) {
            // Numeric variable
            i += 1;
            while (isdigit(this->peekAhead(i))) i += 1;
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
    if (isdigit(this->peekAhead(i))) return "";
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

std::optional<Token> Tokeniser::tryMatchKeywords(FilePos startPos) {
    std::string possibleKeyword = "";
    int i = 1;
    while (isalpha(this->peekAhead(i))) {
        possibleKeyword += this->peekAhead(i);
        i++;
    }

    // Keyword must be followed by non a-zA-Z0-9 character
    if (possibleKeyword.empty() || isalnum(peekAhead(i))) {
        return std::optional<Token>();
    }

    if (this->keywordMap.count(possibleKeyword) == 0) {
        return std::optional<Token>();
    }

    this->advancePositionSameLine(possibleKeyword.size());
    return Token(this->keywordMap[possibleKeyword], startPos, startPos.col + (int) possibleKeyword.size() - 1);
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

/**
 * Match string in the format /<string contents>/<modifiers?>
 * @param tokens
 * @return true if slash string matched. If false then no input consumed
 */
bool Tokeniser::matchSlashString(std::vector<Token> &tokens) {
    if (peek() != '/') return false;
    if (this->matchSimpleString(tokens)) {
        auto start = currentPos();
        auto modifiers = this->matchStringContainingOnlyLetters("msixpodualngc");
        if (!modifiers.empty()) {
            tokens.emplace_back(Token(TokenType::StringModifiers, start, modifiers));
        }
        return true;
    }

    return false;
}

/**
 * Match new lines + whitespace + comments, then add them to the tokens list
 * @param tokens
 * @return
 */
bool Tokeniser::addNewlineWhitespaceCommentTokens(std::vector<Token> &tokens, bool ignoreComments) {
    bool matched = true;
    int size = tokens.size();
    while (matched) {
        matched = false;

        matched = matchNewline(tokens) || matched;
        auto start = currentPos();
        auto whitespace = matchWhitespace();
        if (!whitespace.empty()) {
            tokens.emplace_back(Token(TokenType::Whitespace, start, whitespace));
            matched = true;
        }

        matched = matchNewline(tokens) || matched;

        if (!ignoreComments) {
            start = currentPos();
            auto comment = matchComment();
            matched = !comment.empty() || matched;
            if (!comment.empty()) tokens.emplace_back(Token(TokenType::Comment, start, comment));
        }
    }

    return size != tokens.size();
}

/**
 * Newline matched that takes into account newlines on different systems
 * @param tokens
 * @return
 */
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

/**
 * Match dereference `$x->{...}{...}...`
 * Supports barewords inside the brackets (so keywords/string quote operators are not parsed as such)
 * @param tokens
 */
void Tokeniser::matchDereferenceBrackets(std::vector<Token> &tokens) {
    if (peek() != '{') return;
    auto start = currentPos();
    nextChar();
    tokens.emplace_back(Token(TokenType::HashDerefStart, start, "{"));
    int derefStartIdx = tokens.size() - 1;
    // Next consider whitespace
    start = currentPos();
    auto whitespace = matchWhitespace();
    if (!whitespace.empty()) tokens.emplace_back(Token(TokenType::Whitespace, start, whitespace));

    // Now see if we can match Name bareword until following }
    int offset = 1;
    while (isNameBody(peekAhead(offset))) offset++;

    // Consume any whitespace
    while (isWhitespace(peekAhead(offset))) offset++;
    auto firstChar = this->peek();
    if (peekAhead(offset) == '}' && isalpha(firstChar)) {
        // Is a name!
        start = currentPos();
        auto name = this->matchName();
        tokens.emplace_back(Token(TokenType::HashKey, start, name));

        start = currentPos();
        whitespace = matchWhitespace();
        if (!whitespace.empty()) tokens.emplace_back(Token(TokenType::Whitespace, start, whitespace));

        start = currentPos();
        nextChar();
        tokens.emplace_back(Token(TokenType::HashDerefEnd, start, "}"));
        return;
    }

    // Otherwise just consume tokens (now bareword tokens)
    while (
            tokens[tokens.size() - 1].type != TokenType::RBracket &&
            tokens[tokens.size() - 1].type != TokenType::EndOfInput) {
        this->nextTokens(tokens, true);
    }

    if (tokens[tokens.size() - 1].type == TokenType::RBracket) {
        tokens[tokens.size() - 1].type = TokenType::HashDerefEnd;
    }
}

/**
 * Next run of the tokeniser. Produces only as many new tokens as needed to progress input
 *
 * A large function that probably doesn't need to be inlined so much as the optmizer can do that, but should be reasonably
 * straightforward.
 * @param tokens
 * @param enableHereDoc Disable here-docs can be useful, though maybe not anymore
 */
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
                bool hasWhitespace = i + 1 < tokens.size() && tokens[i + 1].type == TokenType::Whitespace;
                if (hasWhitespace) i++;

                bool hasTilde =
                        i + 1 < tokens.size() && tokens[i + 1].type == TokenType::Operator &&
                        tokens[i + 1].data == "~";
                if (hasTilde) i++;

                // Now consider if we have ok identifier
                i++;
                if (i >= tokens.size() - 1) continue;
                // Here doc deliminator must be a string or a bareword
                if (tokens[i].type != TokenType::Name && tokens[i].type != TokenType::StringStart) {
                    continue;
                } else {
                    std::string delim;
                    if (tokens[i].type == TokenType::Name && hasWhitespace) {
                        continue;    // Whitesapce not allowed with barewords
                    }
                    // Now finally we can confirm a valid heredoc.
                    if (tokens[i].type == TokenType::Name) {
                        delim = tokens[i].data;
                    } else if (tokens[i].type == TokenType::StringStart) {
                        if (i + 1 >= tokens.size()) continue;
                        // Strings have the format StringStart(..) String(..) StringEnd(..)
                        // But empty strings don't include a String(..)
                        if (tokens[i + 1].type == TokenType::String) {
                            delim = tokens[i + 1].data;
                        } else if (tokens[i + 1].type != TokenType::StringEnd) {
                            // Something has gone wrong, don't parse as heredoc
                            continue;
                        }
                    }
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
        // Dereference can ONLY be of a scalar as references are always scalars
        if (this->peekAhead(i) == '$') {
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

    // Handle unary file operators such as -X 'filename'
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
            // Find previous token
            // If previous token is a -> operator, then this is a dereference
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

    auto versionString = this->matchVersionString();
    if (!versionString.empty()) {
        tokens.emplace_back(Token(TokenType::VersionLiteral, startPos, versionString));
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

            // Get next non-whitespace character
            int offset = 2;
            while (isWhitespace(peekAhead(offset))) offset++;
            char nextChar = peekAhead(offset);

            if (isVariable(prevTokenType) || prevTokenType == TokenType::RParen ||
                prevTokenType == TokenType::NumericLiteral || prevTokenType == TokenType::RBracket ||
                prevTokenType == TokenType::RSquareBracket || prevTokenType == TokenType::HashDerefEnd ||
                (secondType == TokenType::Operator && secondData == "->")) {
                this->nextChar();
                tokens.emplace_back(Token(TokenType::Operator, startPos, "/"));
                return;
            } else if (prevTokenType == TokenType::Name) {
                // this is an ambiguity that is impossible to parse statically
                // see  'Perl can not be pared: a formal proof' by Jeffrey Kegler at https://www.perlmonks.org/?node_id=663393
                // depends on number of arguments of the  functions. So `split /$a/;`(ok) and `time /$a/;`(syntax error)
                // are parsed very differently by perl
                //
                // To guess this we use observations from perl code observed in the wild
                //      1. Strings are far more common than division. So if we're not sure, prior information tells
                //         us to pick divide. But...
                //      2. ...Predicting an operator when actually a string is better than predicting a string when actually
                //         an operator. In the former, unless the string contains a left bracket, it is unlikely so
                //         cause issues. The latter will much more likely stringify many lines of perl which WILL LIKELY
                //         ruin the balance of the scopes
                //      3. Strings typically start and end on the same physical file line. If there is a matching string
                //         deliminator (/) on the same line, the effect of wrongly predicting an operator as a string is
                //         very low.
                // So simple heuristic: is there a matching / on the line? If yes then match string, otherwise match
                // operator
                int i = 2;

                while (peekAhead(i) != '\n' && peekAhead(i) != '\r' && peekAhead(i) != '/') i++;
                bool isSlashString = peekAhead(i) == '/';

                if (isSlashString) {
                    this->matchSlashString(tokens);
                } else {
                    this->nextChar();
                    tokens.emplace_back(Token(TokenType::Operator, startPos, "/"));
                }

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

    if (this->matchSimpleString(tokens)) return;

    // String literals / quote literals / transliterations / ...
    if (matchQuoteLiteral(tokens)) return;

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
        auto token = Token(TokenType::Name, startPos, ident);
        if (this->builtinSubMap.count(ident) != 0) token.type = TokenType::Builtin;
        tokens.emplace_back(token);
        return;

    }

    auto name = this->matchName();
    if (!name.empty()) {
        auto token = Token(TokenType::Name, startPos, name);
        if (this->builtinSubMap.count(name) != 0) token.type = TokenType::Builtin;
        tokens.emplace_back(token);
        return;
    }

    throw TokeniseException(std::string("Remaining code exists"));
}

/**
 * Public function that actually does the tokeniseration
 * @return
 */
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
        auto args = '(' + matchBracketedStringLiteral('(') + ')';
        nextChar();
        tokens.emplace_back(Token(TokenType::AttributeArgs, start, args));
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

/**
 * Match prototype. IMPORTANT: This will try to match a prototype regardless, check this makes sense first using
 * given heuristic (isPrototype).
 * @return
 */
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
    // Straight forward, just match until ending bracket
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

/**
 * Matches a subroutine definition `sub name ...`
 * In perl there is a large range of possibilities, but this method tries to capture them all
 * ... anonymous, normal, prototypes, signatures, ...
 * @param tokens
 */
void Tokeniser::matchSubroutine(std::vector<Token> &tokens) {
    // Assumes sub keyword has already been matched
    // Try first to match a name i.e. sub <NAME>
    addWhitespaceToken(tokens);
    matchNewline(tokens);

    auto start = currentPos();
    auto name = this->matchIdentifier();
    if (!name.empty()) {
        tokens.emplace_back(Token(TokenType::SubName, start, name));
    }
    addWhitespaceToken(tokens);

    // No prototype/signature/attributes.
    if (peek() == '{') {
        start = currentPos();
        this->nextChar();
        tokens.emplace_back(Token(TokenType::LBracket, start, "{"));
        return;
    }

    // Next token is a '(' (for signature or proto) or ':' (for attributes)
    // If not then we are done, return
    if (!(peek() == '(' || peek() == ':')) return;

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

/**
 * Second pass to replace *Bracket tokens with HashSub* tokens.
 * This is not too important but if missed then the parser will generate too many scopes (as $x->{...} will get a scope)
 *  This wastes resources, especially in analysis
 * @param tokens
 * @param i
 */
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

void Tokeniser::addWhitespaceToken(std::vector<Token> &tokens) {
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

        // Go back to previous non-whitespace token
        // If it is a Name, replace with HashKey
        // e.g. `key => "Hello"` -> HashKey[key] Op[=>] StringStart(") String(Hello) StringEnd(")
        if (tokens[i].type == TokenType::Operator && tokens[i].data == "=>") {
            int counter = i - 1;
            while (counter >= 0 && isWhitespaceNewlineComment(tokens[counter].type)) counter--;
            if (tokens[counter].type == TokenType::Name) {
                tokens[counter].type = TokenType::HashKey;
            }
        }
    }
}

std::string Tokeniser::tokenToStrWithCode(Token token) {
    return ::tokenToStrWithCode(token, this->program);
}

std::string tokenToStrWithCode(Token token, const std::string &program) {
    std::string code;
    bool success = false;

    if (token.type == TokenType::EndOfInput) return "";

    if (token.startPos.position == -1) {
        code = "startPos position not set";
    } else if (token.endPos.position == -1) {
        code = "endPos position not set";
    } else if (token.endPos.position < token.startPos.position) {
        code = "token.endPos.position < token.startPos.position: Invalid positions";
    } else if (token.endPos.position >= program.size()) {
        code = "End position pos exceeds program size";
    } else {
        success = true;
        code = program.substr(token.startPos.position, (token.endPos.position - token.startPos.position) + 1);
    }

    if (!success) {
        throw "ERROR";
    }

    auto d1 = replace(code, "\n", "\\n");
    code = replace(d1, "\r", "\\r");

    return token.toStr(true) + " :: `" + code + "`";
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


std::optional<Token> previousNonWhitespaceToken(const std::vector<Token> &tokens) {
    if (tokens.empty()) return std::optional<Token>();
    int i = tokens.size() - 1;
    while (i > 0 && isWhitespaceNewlineComment(tokens[i].type)) {
        i--;
    }

    if (isWhitespaceNewlineComment(tokens[i].type)) return std::optional<Token>();
    return std::optional<Token>(tokens[i]);
}
