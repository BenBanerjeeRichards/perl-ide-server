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


std::shared_ptr<Variable>
handleVariableTokens(const std::shared_ptr<TokensNode> &tokensNode, const std::vector<PackageSpan> &packages, int &i,
                     FilePos parentEnd) {
    auto tokenType = tokensNode->tokens[i].type;
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
            return std::make_shared<OurVariable>(nextToken.data, nextToken.startPos, parentEnd, package);
        } else if (tokenType == My || tokenType == State) {
            return std::make_shared<ScopedVariable>(nextToken.data, nextToken.startPos, parentEnd);
        } else if (tokenType == Local) {
            return std::make_shared<LocalVariable>(nextToken.data, nextToken.startPos, parentEnd);
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

    return nullptr;
}

std::shared_ptr<GlobalVariable>
handleGlobalVariables(const std::shared_ptr<TokensNode> &tokensNode, const std::vector<std::string> &variables,
                      const std::vector<PackageSpan> &packages, int &i) {
    std::shared_ptr<GlobalVariable> global = nullptr;

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
            global = std::make_shared<GlobalVariable>(prevToken.data, prevToken.startPos, package);
        }
    }

    i = prevI + 1;

    return global;
}

std::string handleUseFeature(const std::shared_ptr<TokensNode> &tokensNode, int i) {
    auto tokenIter = TokenIterator(tokensNode->tokens, std::vector<TokenType>{Whitespace, Comment, Newline}, i);
    if (tokenIter.next().type != Use) return "";
    auto featureKeywordToken = tokenIter.next();
    if (featureKeywordToken.type != Name || featureKeywordToken.data != "feature") return "";
    auto featureNameToken = tokenIter.next();
    // TODO support quoted strings (requires tokeniser support)
    if (featureKeywordToken.type != Name) return "";
    return featureNameToken.data;
}

void doFindVariableDeclarations(const std::shared_ptr<BlockNode> &tree, const std::shared_ptr<SymbolNode> &symbolNode,
                                FileSymbols &fileSymbols, std::vector<std::string> &variables) {

    for (const auto &child : tree->children) {
        if (std::shared_ptr<BlockNode> blockNode = std::dynamic_pointer_cast<BlockNode>(child)) {
            // Create new child for symbol tree
            auto symbolChild = std::make_shared<SymbolNode>(blockNode->start, blockNode->end, symbolNode->features);
            symbolNode->children.emplace_back(symbolChild);
            doFindVariableDeclarations(blockNode, symbolChild, fileSymbols, variables);
        }

        if (std::shared_ptr<TokensNode> tokensNode = std::dynamic_pointer_cast<TokensNode>(child)) {
            for (int i = 0; i < (int) tokensNode->tokens.size() - 1; i++) {
                auto tokenType = tokensNode->tokens[i].type;
                if (tokenType == My || tokenType == Our || tokenType == Local || tokenType == State) {
                    if (auto variable = handleVariableTokens(tokensNode, fileSymbols.packages, i, tree->end)) {
                        symbolNode->variables.emplace_back(variable);
                        variables.emplace_back(variable->name);
                    }
                } else if (tokenType == Assignment && i > 0) {
                    // Could be a global (package) variable
                    if (auto global = handleGlobalVariables(tokensNode, variables, fileSymbols.packages, i)) {
                        symbolNode->variables.emplace_back(global);
                        variables.emplace_back(global->name);
                    }
                } else if (tokenType == Sub) {
                    // TODO subs
                } else if (tokenType == Use) {
                    auto featureName = handleUseFeature(tokensNode, i);
                    if (!featureName.empty()) symbolNode->features.emplace_back(featureName);
                    i += 1;
                }
            }
        }
    }
}

std::shared_ptr<SymbolNode> buildVariableSymbolTree(const std::shared_ptr<BlockNode> &tree, FileSymbols &fileSymbols) {
    std::vector<std::string> variables;
    auto symbolNode = std::make_shared<SymbolNode>(tree->start, tree->end);
    doFindVariableDeclarations(tree, symbolNode, fileSymbols, variables);
    fileSymbols.symbolTree = symbolNode;
    return symbolNode;
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

SymbolNode::SymbolNode(const FilePos &startPos, const FilePos &endPos, std::vector<std::string> parentFeatures)
        : startPos(startPos), endPos(endPos), features(parentFeatures) {

}

void doPrintSymbolTree(const std::shared_ptr<SymbolNode> &node, int level) {
    for (const auto &variable : node->variables) {
        for (int i = 0; i < level; i++) std::cout << " ";
        std::cout << variable->toStr() << std::endl;
    }

    for (const auto &child : node->children) {
        for (int i = 0; i < level; i++) std::cout << " ";
        std::cout << "SymbolNode " << child->startPos.toStr() << " - " << child->endPos.toStr() << " Features: [" << join(child->features, ",") << "]" <<  std::endl;
        doPrintSymbolTree(child, level + 2);
    }
}

void printSymbolTree(const std::shared_ptr<SymbolNode> &node) {
    std::cout << "SymbolNode" << " Features: [" << join(node->features, ",") << "]" << std::endl;
    doPrintSymbolTree(node, 2);
}

void doGetSymbolMap(const std::shared_ptr<SymbolNode> &symbolTree, const FilePos &pos, SymbolMap &symbolMap) {
    for (const auto &variable : symbolTree->variables) {
        symbolMap[variable->name] = variable;
    }

    for (const auto &child : symbolTree->children) {
        if (insideRange(child->startPos, child->endPos, pos)) {
            doGetSymbolMap(child, pos, symbolMap);
        }
    }
}

SymbolMap getSymbolMap(const std::shared_ptr<SymbolNode> &symbolTree, const FilePos &pos) {
    SymbolMap symbolMap;
    doGetSymbolMap(symbolTree, pos, symbolMap);
    return symbolMap;
}
