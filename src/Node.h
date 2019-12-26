//
// Created by Ben Banerjee-Richards on 2019-10-15.
//

#ifndef PERLPARSER_NODE_H
#define PERLPARSER_NODE_H

#include "FilePos.h"
#include "Token.h"
#include "Util.h"
#include <memory>
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
            int i = 0;
            auto token = tokens[i];
            while (i < tokens.size() && tokens[i].isWhitespaceNewlineOrComment()) i++;
            return "TokensNode " + tokens[i].toStr(true) + " - " +
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


#endif //PERLPARSER_NODE_H
