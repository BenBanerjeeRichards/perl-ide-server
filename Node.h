//
// Created by Ben Banerjee-Richards on 2019-10-15.
//

#ifndef PERLPARSER_NODE_H
#define PERLPARSER_NODE_H

#include "FilePos.h"
#include "Token.h"
#include "Util.h"
#include <vector>

struct Node {
    Node() {}

    std::vector<std::shared_ptr<Node>> children;

    virtual std::string toStr() {
        return std::string();
    };
};

struct TokensNode : Node {
    std::vector<Token> tokens;

    explicit TokensNode(std::vector<Token> tokens) {
        this->tokens = tokens;
    }

    std::string toStr() override {

        if (tokens.size() > 1) {
            return "TokensNode " + firstNonWhitespaceToken(tokens).toStr(true) + " - " +
                   tokens[tokens.size() - 1].startPos.toStr() + " " + tokens[tokens.size() - 1].endPos.toStr();
        }

        return "TokensNode";
    }
};

struct BlockNode : Node {

    explicit BlockNode(FilePos start) {
        this->start = start;
    }

    FilePos start;
    FilePos end;

    std::string toStr() override {
        return "BlockNode " + start.toStr() + " - " + end.toStr();
    }
};

struct PackageSpan {

    PackageSpan(FilePos start, FilePos end, const std::string &name) {
        this->start = start;
        this->end = end;
        this->packageName = name;
    }

    FilePos start;
    FilePos end;
    std::string packageName;
};



#endif //PERLPARSER_NODE_H
