//
// Created by Ben Banerjee-Richards on 2019-09-17.
//

#include "VarAnalysis.h"

bool ScopedVariable::isAccessibleAt(const FilePos &pos) {
    return insideRange(this->declaration, this->scopeEnd, pos);
}


bool GlobalVariable::isAccessibleAt(const FilePos &pos) {
    // Globals available everywhere in current file
    // Even before declaration for now (in future we need to consider packages in other files)
    return true;
}



void doFindVariableDeclarations(const std::shared_ptr<Node> &tree, const std::shared_ptr<SymbolNode> &symbolNode,
                         const std::vector<PackageSpan> &packages, std::vector<std::string>& variables) {

    for (const auto &child : tree->children) {
        if (std::shared_ptr<BlockNode> blockNode = std::dynamic_pointer_cast<BlockNode>(child)) {
            // Create new child for symbol tree
            auto symbolChild = std::make_shared<SymbolNode>(blockNode->start, blockNode->end);
            symbolNode->children.emplace_back(symbolChild);
            doFindVariableDeclarations(child, symbolChild, packages, variables);
        }

        if (std::shared_ptr<TokensNode> tokensNode = std::dynamic_pointer_cast<TokensNode>(child)) {
            std::shared_ptr<BlockNode> parentBlockNode = std::dynamic_pointer_cast<BlockNode>(tree);

            for (int i = 0; i < (int) tokensNode->tokens.size() - 1; i++) {
                auto tokenType = tokensNode->tokens[i].type;
                if (tokenType == My || tokenType == Our || tokenType == Local || tokenType == State) {

                    auto nextToken = tokensNode->tokens[i + 1];
                    while (i < tokensNode->tokens.size() && nextToken.isWhitespaceNewlineOrComment()) {
                        i++;
                        nextToken = tokensNode->tokens[i];
                    }

                    if (nextToken.type == ScalarVariable || nextToken.type == HashVariable ||
                        nextToken.type == ArrayVariable) {
                        // We've got a definition!
                        if (tokenType == Our) {
                            auto package = findPackageAtPos(packages, nextToken.startPos);
                            symbolNode->variables.emplace_back(
                                    std::make_shared<OurVariable>(nextToken.data, nextToken.startPos,
                                                                  parentBlockNode->end, package));
                            variables.emplace_back(nextToken.data);
                        } else if (tokenType == My || tokenType == State) {
                            symbolNode->variables.emplace_back(
                                    std::make_shared<ScopedVariable>(nextToken.data, nextToken.startPos,
                                                                     parentBlockNode->end));
                            variables.emplace_back(nextToken.data);
                        } else if (tokenType == Local) {
                            symbolNode->variables.emplace_back(
                                    std::make_shared<LocalVariable>(nextToken.data, nextToken.startPos,
                                                                    parentBlockNode->end));
                            variables.emplace_back(nextToken.data);
                        }

                        // Finally, if combined assignment then skip past Assignment token to prevent confusion with
                        // package variables below
                        nextToken = tokensNode->tokens[i + 1];
                        while (i < tokensNode->tokens.size() && nextToken.isWhitespaceNewlineOrComment()) {
                            i++;
                            nextToken = tokensNode->tokens[i];
                        }

                        if (nextToken.type == Assignment) i++;
                    }
                } else if (tokenType == Assignment && i > 0) {
                    // Could be a global (package) variable
                    // Go back to previous token
                    int prevI = i;
                    auto prevToken = tokensNode->tokens[i - 1];
                    while (i > 0 && prevToken.isWhitespaceNewlineOrComment()) {
                        i--;
                        prevToken = tokensNode->tokens[i];
                    }

                    if (prevToken.type == ScalarVariable || prevToken.type == HashVariable ||
                        prevToken.type == ArrayVariable) {
                        // We have something in form $x = ... with no our/local/my/state...
                        // Implies this is a global variable definition IF variable not previously declared
                        bool isDeclared = false;
                        for (const std::string &var : variables) {
                            if (var == prevToken.data) {
                                isDeclared = true;
                                break;
                            }
                        }

                        if (!isDeclared) {
                            auto package = findPackageAtPos(packages, prevToken.startPos);
                            symbolNode->variables.emplace_back(
                                    std::make_shared<GlobalVariable>(prevToken.data, prevToken.startPos, package));
                            variables.emplace_back(prevToken.data);
                        }
                    }

                    i = prevI + 1;
                }
            }
        }
    }
}
void findVariableDeclarations(const std::shared_ptr<Node> &tree, const std::shared_ptr<SymbolNode> &symbolNode,
                              const std::vector<PackageSpan> &packages) {
    std::vector<std::string> variables;
    doFindVariableDeclarations(tree, symbolNode, packages, variables);
}



std::string findPackageAtPos(const std::vector<PackageSpan> &packages, FilePos pos) {
    // TODO replace with binary search
    for (const auto &package : packages) {
        if (insideRange(package.start, package.end, pos)) return package.packageName;
    }

    std::cerr << "Failed to find package span at pos " << pos.toStr() << std::endl;
    return "main";
}

SymbolNode::SymbolNode(const FilePos &startPos, const FilePos &endPos) : startPos(startPos), endPos(endPos) {}

void doPrintSymbolTree(const std::shared_ptr<SymbolNode>& node, int level) {
    for (const auto& variable : node->variables) {
        for (int i = 0; i < level; i++) std::cout << " ";
        std::cout << variable->toStr() << std::endl;
    }

    for (const auto& child : node->children) {
        for (int i = 0; i < level; i++) std::cout << " ";
        std::cout << "SymbolNode " << node->startPos.toStr() << " - " << node->endPos.toStr() << std::endl;
        doPrintSymbolTree(child, level + 2);
    }
}

void printSymbolTree(const std::shared_ptr<SymbolNode>& node) {
    std::cout << "SymbolNode" << std::endl;
    doPrintSymbolTree(node, 2);
}

