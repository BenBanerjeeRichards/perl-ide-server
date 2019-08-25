//
// Created by Ben Banerjee-Richards on 2019-08-19.
//

#ifndef PERLPARSER_TOKEN_H
#define PERLPARSER_TOKEN_H


#include <string>
#include <algorithm>

class Token {

public:
    virtual std::string toString() {
        if (this->data.empty()) {
            return this->type;
        }

        // Don't print out newlines to console
        std::string dataToShow = this->data;

        // TODO move this into until function
        while(dataToShow.find("\n") != std::string::npos) {
            dataToShow.replace(dataToShow.find("\n"), 1,"\\n");
        }

        while(dataToShow.find("\t") != std::string::npos) {
            dataToShow.replace(dataToShow.find("\t"), 1,"\\t");
        }


        return this->type + "(" + dataToShow + ")";
    }

    // When data is identical to code
    Token(const std::string &type, const std::string &data, unsigned int line, unsigned int startCol) {
        this->type = type;
        this->data = data;
        this->startLine = line;
        this->startCol = startCol;

        // Compute ending position from data
        this->endLine = line;
        this->endCol = startCol + data.size();
    }

    Token(const std::string &type, const std::string &data, unsigned int line, unsigned int startCol,
          unsigned int endCol) {
        this->type = type;
        this->data = data;
        this->startLine = line;
        this->startCol = startCol;

        // Compute ending position from data
        this->endLine = line;
        this->endCol = endCol;
    }

    Token(const std::string &type, const std::string &data, unsigned int startLine, unsigned int startCol,
          unsigned int endLine, unsigned int endCol) {
        this->type = type;
        this->data = data;
        this->startLine = startLine;
        this->startCol = startCol;

        // Compute ending position from data
        this->endLine = endLine;
        this->endCol = endCol;
    }


protected:
    // Readable name used in tostring
    std::string type;
    // Optional data used. e.g. $ident has 'ident' as it's data, but keyword my has no data
    std::string data;

    // Position of token in file
    unsigned int startLine;
    unsigned int startCol;
    unsigned int endLine;
    unsigned int endCol;
};

struct KeywordToken : public Token {
    explicit KeywordToken(const std::string &keyword, int startLine, int startCol)
            : Token("KEYWORD", keyword, startLine, startCol) {}
};

struct StringToken : public Token {
    explicit StringToken(const std::string &contents, int startLine, int startCol)
            : Token("STRING", contents, startLine, startCol) {}
};

// Dollar variable such as $thing
struct ScalarVariableToken : public Token {
    explicit ScalarVariableToken(const std::string &name, int startLine, int startCol)
            : Token("$SCALAR", name, startLine, startCol, startCol + 1) {}
};

struct ArrayVariableToken : public Token {
    explicit ArrayVariableToken(const std::string &name, int startLine, int startCol)
            : Token("@ARRAY", name, startLine, startCol, startCol + 1) {}
};

struct HashVariableToken : public Token {
    explicit HashVariableToken(const std::string &name, int startLine, int startCol)
            : Token("%HASH", name, startLine, startCol, startCol + 1) {}
};


// Function name, constant name etc..
// We need more context to figure out what it is
struct WordToken : public Token {
    explicit WordToken(const std::string &word, int startLine, int startCol)
            : Token("WORD", word, startLine, startCol) {}
};

// Operators
// For our use case I don't care much about the operators so we just use a single class
// with the data as the operator (e.g. we don't have AddOperator, SubtractOperator, ...)
// This may change in the future
struct OperatorToken : public Token {
    explicit OperatorToken(const std::string &op, int startLine, int startCol)
            : Token("OP", op, startLine, startCol) {}
};

// {
struct LBracketToken : public Token {
    explicit LBracketToken(int startLine, int startCol)
            : Token("LBRACKET", "", startLine, startCol, startCol + 1) {}
};

// }
struct RBracketToken : public Token {
    explicit RBracketToken(int startLine, int startCol)
            : Token("RBRACKET", "", startLine, startCol, startCol + 1) {}
};

// (
struct LParenToken : public Token {
    explicit LParenToken(int startLine, int startCol)
            : Token("LPAREN", "", startLine, startCol, startCol + 1) {}
};

// )
struct RParenToken : public Token {
    explicit RParenToken(int startLine, int startCol)
            : Token("RPAREN", "", startLine, startCol, startCol + 1) {}
};

struct LSquareBracketToken : public Token {
    explicit LSquareBracketToken(int startLine, int startCol)
            : Token("RPAREN", "", startLine, startCol, startCol + 1) {}
};

struct RSquareBracketToken : public Token {
    explicit RSquareBracketToken(int startLine, int startCol)
            : Token("RPAREN", "", startLine, startCol, startCol + 1) {}
};

struct CommentToken : public Token {
    CommentToken(const std::string &comment, int startLine, int startCol, int endLine, int endCol)
            : Token("COMMENT", comment, startLine, startCol, endLine, endCol) {}
};

struct NewlineToken : public Token {
    NewlineToken(const std::string &specifiers, int line, int col)
            : Token("NEWLINE", specifiers, line, col) {}

};

struct WhitespaceToken : public Token {
     WhitespaceToken(const std::string &whitespace, int startLine, int startCol, int endLine, int endCol)
            : Token("WHITESPACE", whitespace, startLine, startCol, endLine, endCol) {}
};


struct DotToken : public Token {
    DotToken(int line, int col) : Token("DOT", "", line, col) {};
};

struct AssignmentToken : public Token {
    AssignmentToken(int line, int col) : Token("ASSIGN", "", line, col) {};
};

struct SemicolonToken : public Token {
    SemicolonToken(int line, int col) : Token("SEMICOLON", "", line, col) {};
};

struct EndOfInputToken : public Token {
    EndOfInputToken(int line, int col) : Token("EOF", "", line, col) {};
};

struct IfToken : public Token  {
    IfToken(int line, int col) : Token("IF", "", line, col, col + 2) {};
};

struct ElseToken : public Token  {
    ElseToken(int line, int col) : Token("ELSE", "", line, col, col + 4) {};
};

struct ElseIfToken : public Token  {
    ElseIfToken(int line, int col) : Token("ELSEIF", "", line, col, col + 6) {};
};

struct UnlessToken : public Token  {
    UnlessToken(int line, int col) : Token("UNLESS", "", line, col, col + 6) {};
};

struct WhileToken : public Token  {
    WhileToken(int line, int col) : Token("WHILE", "", line, col, col + 5) {};
};

struct UntilToken : public Token  {
    UntilToken(int line, int col) : Token("UNTIL", "", line, col, col + 5) {};
};

struct ForToken : public Token  {
    ForToken(int line, int col) : Token("FOR", "", line, col, col + 3) {};
};

struct ForeachToken : public Token  {
    ForeachToken(int line, int col) : Token("FOREACH", "", line, col, col + 7) {};
};

struct WhenToken : public Token  {
    WhenToken(int line, int col) : Token("WHEN", "", line, col, col + 4) {};
};

struct DoToken : public Token  {
    DoToken(int line, int col) : Token("DO", "", line, col, col + 2) {};
};

struct NextToken : public Token  {
    NextToken(int line, int col) : Token("NEXT", "", line, col, col + 4) {};
};

struct RedoToken : public Token  {
    RedoToken(int line, int col) : Token("REDO", "", line, col, col + 4) {};
};

struct LastToken : public Token  {
    LastToken(int line, int col) : Token("LAST", "", line, col, col + 4) {};
};

struct MyToken : public Token  {
    MyToken(int line, int col) : Token("MY", "", line, col, col + 2) {};
};

struct StateToken : public Token  {
    StateToken(int line, int col) : Token("STATE", "", line, col, col + 5) {};
};

struct OurToken : public Token  {
    OurToken(int line, int col) : Token("OUR", "", line, col, col + 3) {};
};

struct BreakToken : public Token  {
    BreakToken(int line, int col) : Token("BREAK", "", line, col, col + 5) {};
};

struct ContinueToken : public Token  {
    ContinueToken(int line, int col) : Token("BREAK", "", line, col, col + 8) {};
};

struct GivenToken : public Token  {
    GivenToken(int line, int col) : Token("GIVEN", "", line, col, col + 5) {};
};

struct UseToken : public Token  {
    UseToken(int line, int col) : Token("USE", "", line, col, col + 3) {};
};



#endif //PERLPARSER_TOKEN_H
