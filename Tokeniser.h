//
// Created by Ben Banerjee-Richards on 2019-08-19.
//

#ifndef PERLPARSER_TOKENISER_H
#define PERLPARSER_TOKENISER_H

#include <string>
#include <functional>
#include <memory>
#include <vector>
#include <regex>
#include "TokeniseException.h"
#include <utility>
#include <iostream>
#include "FilePos.h"
#include "Util.h"
#include <unordered_map>
#include <optional>

enum class TokenType {
    String,
    StringStart,
    StringEnd,
    ScalarVariable,
    ArrayVariable,
    HashVariable,
    Operator,
    LBracket,
    RBracket,
    LParen,
    RParen,
    LSquareBracket,
    RSquareBracket,
    Comment,
    Newline,
    Whitespace,
    Dot,
    Assignment,
    Semicolon,
    EndOfInput,
    If,
    Else,
    ElseIf,
    Unless,
    While,
    Until,
    For,
    Foreach,
    When,
    Do,
    Package,
    Next,
    Redo,
    Last,
    My,
    State,
    Our,
    Break,
    Continue,
    Given,
    Use,
    Sub,
    Name,
    NumericLiteral,
    Pod,
    Comma,
    Local,
    Prototype,
    Signature,
    SubName,
    Attribute,
    AttributeArgs,
    AttributeColon
};

struct KeywordConfig {
    KeywordConfig(const std::string &code, TokenType type);

    std::string code;
    TokenType type;
};

struct QuotedStringLiteral {
    QuotedStringLiteral(FilePos start, FilePos end, std::string literal);

    std::string contents;
    FilePos literalStart;
    FilePos literalEnd;
};

std::string tokenTypeToString(const TokenType &t);


class Token {

public:
    // When data is identical to code
    Token(const TokenType &type, FilePos start, const std::string &data = "");

    Token(const TokenType &type, FilePos start, int endCol, const std::string &data = "");

    Token(const TokenType &type, FilePos start, FilePos end, const std::string &data = "");

    std::string toStr(bool includeLocation = false);

    bool isWhitespaceNewlineOrComment();

    TokenType type;
    // Position of token in file
    FilePos startPos;
    FilePos endPos;
    // Readable name used in tostring
    // Optional data used. e.g. $ident has 'ident' as it's data, but keyword my has no data
    std::string data;


private:

};

class Tokeniser {
public:
    explicit Tokeniser(std::string programStream);

    std::vector<Token> tokenise();

    std::string tokenToStrWithCode(Token token, bool includeLocation = false);

private:
    char nextChar();

    char peek();

    char peekAhead(int i);

    bool isEof();

    std::string substring(int fromIdx, int length);

    char prevChar(int i);

    // Whitespace such as tabs, empty spaces. Does NOT include new lines
    static bool isWhitespace(char c);

    static bool isNewline(char c);

    static bool isLowercase(char c);

    static bool isUppercase(char c);

    static bool isNumber(char c);

    static bool isAlphaNumeric(char c);

    static bool isPunctuation(char c);

    bool matchKeyword(const std::string &keyword);

    static bool isNameBody(char c);

    std::string getWhile(const std::function<bool(char)> &nextCharTest);

    // options should be sorted longest to shortest and in preference of match
    std::string matchString(const std::vector<std::string> &options, bool requireTrailingNonAN = false);

    // Match some perl 'name' - could be a function name, function call, etc... We just don't know yet
    std::string matchName();

    std::string matchStringLiteral(char ident, bool includeIdent = true);

    std::string matchBracketedStringLiteral(char bracket);

    std::vector<Token> matchQuoteLiteral();

    std::string matchString();

    std::string matchNumeric();

    std::string matchComment();

    std::string matchPod();

    std::string matchVariable();

    std::string matchWhitespace();

    int peekPackageTokens(int i);

    int nextLine();

    void advancePositionSameLine(int i);

    void secondPass(std::vector<Token> &tokens);

    std::optional<Token> tryMatchKeywords(FilePos startPos);

    std::optional<Token> doMatchKeyword(FilePos startPos, const std::string &keywordCode, TokenType keywordType);

    FilePos currentPos();

    std::vector<Token> matchAttribute();

    std::vector<Token> matchAttributes();

    std::string matchPrototype();

    std::string matchSignature();

    std::vector<Token> matchSignatureTokens();


    bool isPrototype();

    int _position = -1;
    int currentLine = 1;
    int currentCol = 1;
    std::string program;
    std::vector<KeywordConfig> keywordConfigs;

    std::vector<Token> matchLiteralBody(const std::string& quoteChars, FilePos start, char matchedQuoteChar = EOF);
};

// TODO make this an actual iterator
class TokenIterator {
public:
    TokenIterator(const std::vector<Token> &tokens, std::vector<TokenType> ignoreTokens);

    TokenIterator(const std::vector<Token> &tokens, std::vector<TokenType> ignoreTokens, int offset);

    Token next();

    int getIndex();

private:
    const std::vector<Token> &tokens;
    std::vector<TokenType> ignoreTokens;
    int i;
};

#endif //PERLPARSER_TOKENISER_H
