//
// Created by Ben Banerjee-Richards on 2019-09-16.
//

#include "Parser.h"

void doParse(const std::shared_ptr<BlockNode> &node, const std::vector<Token> &tokens, int &tokenIdx) {
    std::vector<Token> tokensAcc;
    while (tokenIdx < tokens.size()) {
        Token token = tokens[tokenIdx];
        tokensAcc.emplace_back(token);
        tokenIdx += 1;

        if (token.type == LBracket) {
            node->children.emplace_back(std::make_shared<TokensNode>(tokensAcc));
            tokensAcc.clear();

            // Now create child
            auto child = std::make_shared<BlockNode>(token.startPos);

            node->children.emplace_back(child);
            doParse(child, tokens, tokenIdx);
        } else if (token.type == RBracket) {
            node->children.emplace_back(std::make_shared<TokensNode>(tokensAcc));
            node->end = token.endPos;
            tokensAcc.clear();
            return;
        }
    }

    // Add remaining tokens
    node->children.emplace_back(std::make_shared<TokensNode>(tokensAcc));
}

Token firstNonWhitespaceToken(const std::vector<Token> &tokens) {
    for (auto token : tokens) {
        if (token.type != Whitespace && token.type != Newline) return token;
    }

    return tokens[0];   // Nothing found
}

void doPrintParseTree(std::shared_ptr<Node> parent, int level) {
    for (int i = 0; i < (int) parent->children.size(); i++) {
        for (int j = 0; j < level; j++) std::cout << "  ";
        std::cout << parent->children[i]->toStr() << std::endl;
        doPrintParseTree(parent->children[i], level + 1);
    }
}


void printParseTree(std::shared_ptr<Node> root) {
    doPrintParseTree(root, 0);
}

void addPackageSpan(std::vector<PackageSpan>& packageSpans, PackageSpan packageSpan) {
    if (!packageSpans.empty()) {
        if (packageSpans[packageSpans.size() - 1].packageName == packageSpan.packageName) {
            // Same package name, just update end pos
            packageSpans[packageSpans.size() - 1].end = packageSpan.end;
            return;
        }
    }

    packageSpans.emplace_back(packageSpan);
}

std::vector<PackageSpan>
doParsePackages(const std::shared_ptr<BlockNode> &parent, std::stack<std::string> &packageStack,
                FilePos &currentPackageStart) {
    std::vector<PackageSpan> packageSpans;

    for (const auto &child : parent->children) {
        if (std::shared_ptr<BlockNode> blockNode = std::dynamic_pointer_cast<BlockNode>(child)) {
            // Going into new scope, push current package onto stack
            packageStack.push(packageStack.top());
            auto morePackages = doParsePackages(blockNode, packageStack, currentPackageStart);
            for (auto &morePackage : morePackages) packageSpans.emplace_back(std::move(morePackage));
        }

        if (std::shared_ptr<TokensNode> tokensNode = std::dynamic_pointer_cast<TokensNode>(child)) {
            for (int i = 0; i < (int) tokensNode->tokens.size() - 1; i++) {
                if (tokensNode->tokens[i].type == Package) {
                    auto nextToken = tokensNode->tokens[i + 1];
                    while (i < tokensNode->tokens.size() && nextToken.isWhitespaceNewlineOrComment()) {
                        i++;
                        nextToken = tokensNode->tokens[i];
                    }

                    if (nextToken.type == Name) {
                        // Found a new package definition
                        if (packageStack.empty()) {
                            // Package analysis failed
                            // FIXME Put a proper handling here
                            std::cerr << "Package analysis failed - package stack empty!";
                            return packageSpans;
                        }

                        std::string prevPackageName = packageStack.top();
                        packageStack.pop();
                        packageStack.push(nextToken.data);

                        auto packageStart = tokensNode->tokens[i].startPos;
                        addPackageSpan(packageSpans, PackageSpan(currentPackageStart, packageStart, prevPackageName));
                        currentPackageStart = packageStart;
                    }
                }
            }
        }
    }

    // End of BlockScope
    if (packageStack.empty()) {
        std::cerr << "Package analysis failed - package stack empty at end of BlockScope" << std::endl;
    } else {
        addPackageSpan(packageSpans, PackageSpan(currentPackageStart, parent->end, packageStack.top()));
        packageStack.pop();
        currentPackageStart = parent->end;
    }

    return packageSpans;
}

std::vector<PackageSpan> parsePackages(std::shared_ptr<BlockNode> parent) {
    // This is a bit of a pain to do
    // We can have multiple package statements in a single file. They are constrained to the scope they are in
    std::stack<std::string> packageStack;
    packageStack.push("main");  // Perl starts off any file in the main package
    auto start = FilePos(1, 1);
    auto packageSpans = doParsePackages(parent, packageStack, start);

    if (packageStack.empty()) {
        return packageSpans;
    }

    packageSpans.emplace_back(PackageSpan(start, parent->end, packageStack.top()));
    return packageSpans;
}

std::shared_ptr<BlockNode> parse(std::vector<Token> tokens) {
    auto node = std::make_shared<BlockNode>(FilePos(0, 0));
    node->end = tokens[tokens.size() - 1].endPos;
    int tokenIdx = 0;
    doParse(node, tokens, tokenIdx);
    return node;
}

