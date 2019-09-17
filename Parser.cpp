//
// Created by Ben Banerjee-Richards on 2019-09-16.
//

#include "Parser.h"

void doParse(std::shared_ptr<Node> node, const std::vector<Token> &tokens, int &tokenIdx) {
    std::vector<Token> tokensAcc;
    while (tokenIdx < tokens.size()) {
        Token token = tokens[tokenIdx];
        tokensAcc.emplace_back(token);
        tokenIdx += 1;

        if (token.type == LBracket) {
            node->children.emplace_back(std::make_shared<TokensNode>(tokensAcc));
            tokensAcc.clear();

            std::cout << "Node starting from " << token.startPos.toStr() << std::endl;
            // Now create child
            auto child = std::make_shared<BlockNode>();

            node->children.emplace_back(child);
            doParse(child, tokens, tokenIdx);
        } else if (token.type == RBracket) {
            std::cout << "Node ending at " << token.startPos.toStr() << std::endl;
            node->children.emplace_back(std::make_shared<TokensNode>(tokensAcc));
            tokensAcc.clear();
            return;
        }
    }
}

Token firstNonWhitespaceToken(const std::vector<Token>& tokens) {
    for (auto token : tokens) {
        if (token.type != Whitespace && token.type != Newline) return token;
    }

    return tokens[0];   // Nothing found
}

void doPrintParseTree(std::shared_ptr<Node> parent, int level) {
    for (int i = 0; i < (int)parent->children.size(); i++) {
        for (int j = 0; j < level; j++) std::cout << "  ";
        std::cout << parent->children[i]->toStr() << std::endl;
        doPrintParseTree(parent->children[i], level + 1);
    }
}


void printParseTree(std::shared_ptr<Node> root) {
    doPrintParseTree(root, 0);
}


std::shared_ptr<Node> parse(std::vector<Token> tokens) {
    auto node = std::make_shared<BlockNode>();
    int tokenIdx = 0;
    doParse(node, tokens, tokenIdx);
    return node;
}

