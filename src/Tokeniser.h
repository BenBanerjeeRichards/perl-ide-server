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
#include <utility>
#include <iostream>
#include <unordered_map>
#include <optional>

#include "Token.h"
#include "Util.h"
#include "TokeniseException.h"

class Tokeniser {
public:
    Tokeniser(std::string program, bool doSecondPass = true);

    std::vector<Token> tokenise();

    std::string tokenToStrWithCode(Token token);

    std::string matchIdentifier();

private:
    char nextChar();

    char peek();

    char peekAhead(int i);

    bool isEof();

    char prevChar(int i);

    // Whitespace such as tabs, empty spaces. Does NOT include new lines
    static bool isWhitespace(char c);

    static bool isNewline(char c);

    static bool isPunctuation(char c);

    bool matchKeyword(const std::string &keyword);

    static bool isNameBody(char c);

    std::string getWhile(const std::function<bool(char)> &nextCharTest);

    // options should be sorted longest to shortest and in preference of match
    std::string matchStringOption(const std::vector<std::string> &options, bool requireTrailingNonAN = false);

    // Match some perl 'name' - could be a function name, function call, etc... We just don't know yet
    std::string matchName();

    std::string matchStringLiteral(char delim, bool includeDelim = true);

    std::string matchBracketedStringLiteral(char bracket);

    bool matchQuoteLiteral(std::vector<Token> &tokens);

    bool matchSimpleString(std::vector<Token> &tokens);

    std::string matchNumeric();

    std::string matchComment();

    std::string matchPod();

    std::string matchVariable();

    std::string matchWhitespace();

    int peekPackageTokens(int i);

    int nextLine();

    void advancePositionSameLine(int i);

    void secondPass(std::vector<Token> &tokens);

    void matchSubroutine(std::vector<Token> &tokens);

    void secondPassHash(std::vector<Token> &tokens, int &i);

    std::optional<Token> tryMatchKeywords(FilePos startPos);

    FilePos currentPos();

    bool matchAttribute(std::vector<Token> &tokens);

    bool matchAttributes(std::vector<Token> &tokens);

    std::string matchPrototype();

    std::string matchSignature();

    bool matchSignatureTokens(std::vector<Token> &tokens);

    std::string matchStringContainingOnlyLetters(const std::string &letters);


    bool isPrototype();

    int _position = -1;
    int currentLine = 1;
    int currentCol = 1;
    std::string program;
    std::unordered_map<std::string, TokenType> keywordMap;
    std::unordered_map<std::string, int> builtinSubMap;
    int positionOffset = 0;
    bool doSecondPass = true;

    void matchDelimString(std::vector<Token> &tokens);

    void nextTokens(std::vector<Token> &tokens, bool enableHereDoc = true);

    void matchHereDocBody(std::vector<Token> &tokens, const std::string &hereDocDelim, bool hasTilde);

    void secondPassHashReref(std::vector<Token> &tokens, int &i);

    bool matchSlashString(std::vector<Token> &tokens);

    bool matchNewline(std::vector<Token> &tokens);

    std::string matchBasicIdentifier(int &i);

    std::string doMatchNormalIdentifier(int &i);

    void matchDereferenceBrackets(std::vector<Token> &tokens);

    bool addWhitespaceToken(std::vector<Token> &tokens);

    bool addNewlineWhitespaceCommentTokens(std::vector<Token> &tokens, bool ignoreComments = false);

    std::string matchQuoteOperator();

    void backtrack(FilePos pos);

    std::string matchVersionString();
};

std::optional<Token> previousNonWhitespaceToken(const std::vector<Token> &tokens);

std::string tokenToStrWithCode(Token token, const std::string& program);


#endif //PERLPARSER_TOKENISER_H
