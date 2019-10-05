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


std::shared_ptr<Variable> makeVariable(TokenType type, std::string name, FilePos declaration, FilePos scopeEnd,
                                       const std::vector<PackageSpan> &packages) {
    if (type == TokenType::Our) {
        auto package = findPackageAtPos(packages, declaration);
        return std::make_shared<OurVariable>(name, declaration, scopeEnd, package);
    } else if (type == TokenType::My || type == TokenType::State) {
        return std::make_shared<ScopedVariable>(name, declaration, scopeEnd);
    } else if (type == TokenType::Local) {
        return std::make_shared<LocalVariable>(name, declaration, scopeEnd);
    }

    return nullptr;
}

std::vector<std::shared_ptr<Variable>>
handleVariableTokens(const std::shared_ptr<TokensNode> &tokensNode, const std::vector<PackageSpan> &packages, int &i,
                     FilePos parentEnd) {
    std::vector<std::shared_ptr<Variable>> variables;
    TokenIterator tokensIter(tokensNode->tokens,
                             std::vector<TokenType>{TokenType::Whitespace, TokenType::Comment, TokenType::Newline}, i);

    auto tokenType = tokensIter.next().type;
    auto nextToken = tokensIter.next();

    if (nextToken.type == TokenType::ScalarVariable || nextToken.type == TokenType::HashVariable ||
        nextToken.type == TokenType::ArrayVariable) {
        // We've got a definition!
        if (auto var = makeVariable(tokenType, nextToken.data, nextToken.startPos, parentEnd, packages)) {
            variables.emplace_back(var);
        }

        // Finally, if combined assignment then skip past Assignment token to prevent confusion with
        // package variables below
        nextToken = tokensNode->tokens[i + 1];
        while (i < tokensNode->tokens.size() && nextToken.isWhitespaceNewlineOrComment()) {
            i++;
            nextToken = tokensNode->tokens[i];
        }

        if (nextToken.type == TokenType::Assignment) i++;
    } else if (nextToken.type == TokenType::LParen) {
        // my ($x, $y) syntax - combined declaration. Consider every variable inside
        while (nextToken.type != TokenType::RParen && nextToken.type != TokenType::EndOfInput) {
            if (nextToken.type == TokenType::ScalarVariable || nextToken.type == TokenType::HashVariable ||
                nextToken.type == TokenType::ArrayVariable) {
                // Variable!
                if (auto var = makeVariable(tokenType, nextToken.data, nextToken.startPos, parentEnd, packages)) {
                    variables.emplace_back(var);
                }
            }
            nextToken = tokensIter.next();
        }

    }

    i = tokensIter.getIndex();
    return variables;
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

    if (prevToken.type == TokenType::ScalarVariable || prevToken.type == TokenType::HashVariable ||
        prevToken.type == TokenType::ArrayVariable) {
        // We have something in form $x = ... with no our/local/my/state...
        // Implies this is a global variable definition IF variable not previously declared
        if (std::find(variables.begin(), variables.end(), prevToken.data) == variables.end()) {
            auto package = findPackageAtPos(packages, prevToken.startPos);
            global = std::make_shared<GlobalVariable>(prevToken.data, prevToken.startPos, package);
        }
    }

    i = prevI + 1;

    return global;
}

std::string handleUseFeature(const std::shared_ptr<TokensNode> &tokensNode, int i) {
    auto tokenIter = TokenIterator(tokensNode->tokens, std::vector<TokenType>{TokenType::Whitespace, TokenType::Comment,
                                                                              TokenType::Newline}, i);
    if (tokenIter.next().type != TokenType::Use) return "";
    auto featureKeywordToken = tokenIter.next();
    if (featureKeywordToken.type != TokenType::Name || featureKeywordToken.data != "feature") return "";
    auto featureNameToken = tokenIter.next();
    // TODO support quoted strings (requires tokeniser support)
    if (featureKeywordToken.type != TokenType::Name) return "";
    return featureNameToken.data;
}

std::optional<Subroutine> handleSub(const std::shared_ptr<TokensNode> &tokensNode, int &i) {
    Subroutine subroutine;
    auto tokenIter = TokenIterator(tokensNode->tokens, std::vector<TokenType>{TokenType::Whitespace, TokenType::Comment,
                                                                              TokenType::Newline}, i);
    auto subToken = tokenIter.next();
    if (subToken.type != TokenType::Sub) return std::optional<Subroutine>();

    subroutine.pos = subToken.startPos;

    auto nextTok = tokenIter.next();
    if (nextTok.type == TokenType::Name) {
        // If this is not the case then function is unnamed
        subroutine.name = nextTok.data;
        subroutine.nameStart = nextTok.startPos;
        subroutine.nameEnd = nextTok.endPos;
        nextTok = tokenIter.next();
    }

    while (nextTok.type != TokenType::LBracket && nextTok.type != TokenType::EndOfInput) {
        if (nextTok.type == TokenType::Signature) subroutine.signature = nextTok.data;
        if (nextTok.type == TokenType::Prototype) subroutine.prototype = nextTok.data;
        nextTok = tokenIter.next();
    }

    return std::optional<Subroutine>(subroutine);

}

void doFindVariableDeclarations(const std::shared_ptr<BlockNode> &tree, const std::shared_ptr<SymbolNode> &symbolNode,
                                FileSymbols &fileSymbols, std::vector<std::string> &variables,
                                std::unordered_map<std::string, std::vector<FilePos>>& variableUsages) {

    for (const auto &child : tree->children) {
        if (std::shared_ptr<BlockNode> blockNode = std::dynamic_pointer_cast<BlockNode>(child)) {
            // Create new child for symbol tree
            auto symbolChild = std::make_shared<SymbolNode>(blockNode->start, blockNode->end, symbolNode->features);
            symbolNode->children.emplace_back(symbolChild);
            doFindVariableDeclarations(blockNode, symbolChild, fileSymbols, variables, variableUsages);
        }

        if (std::shared_ptr<TokensNode> tokensNode = std::dynamic_pointer_cast<TokensNode>(child)) {
            for (int i = 0; i < (int) tokensNode->tokens.size() - 1; i++) {
                auto tokenType = tokensNode->tokens[i].type;
                if (tokenType == TokenType::My || tokenType == TokenType::Our || tokenType == TokenType::Local ||
                    tokenType == TokenType::State) {
                    auto newVariables = handleVariableTokens(tokensNode, fileSymbols.packages, i, tree->end);
                    for (auto &variable : newVariables) {
                        symbolNode->variables.emplace_back(variable);
                        variables.emplace_back(variable->name);

                        // Also add declarations to the usage lists
                        if (variableUsages.count(variable->name) == 0) {
                            variableUsages[variable->name] = std::vector<FilePos>();
                        }
                        variableUsages[variable->name].emplace_back(variable->declaration);
                    }

                    i++;
                } else if (tokenType == TokenType::ScalarVariable || tokenType == TokenType::ArrayVariable || tokenType == TokenType::HashVariable) {
                    auto varName = tokensNode->tokens[i].data;
                    if (variableUsages.count(varName) == 0) {
                        variableUsages[varName] = std::vector<FilePos>();
                    }
                    variableUsages[varName].emplace_back(tokensNode->tokens[i].startPos);

                } else if (tokenType == TokenType::Assignment && i > 0) {
                    // Could be a global (package) variable
                    if (auto global = handleGlobalVariables(tokensNode, variables, fileSymbols.packages, i)) {
                        fileSymbols.globals.emplace_back(global);
                        variables.emplace_back(global->name);
                    }
                } else if (tokenType == TokenType::Sub) {
                    auto sub = handleSub(tokensNode, i);
                    if (sub.has_value()) {
                        fileSymbols.subroutines.emplace_back(sub.value());
                        i++;
                    }
                } else if (tokenType == TokenType::Use) {
                    auto featureName = handleUseFeature(tokensNode, i);
                    if (!featureName.empty()) symbolNode->features.emplace_back(featureName);
                    i += 1;
                }
            }
        }
    }
}

std::shared_ptr<Variable> findDeclaration(const FileSymbols& fileSymbols, const std::string& variableName, FilePos pos) {
    auto map = getSymbolMap(fileSymbols, pos);
    if (map.count(variableName) > 0) return map[variableName];
    return nullptr;
}

std::shared_ptr<SymbolNode> buildVariableSymbolTree(const std::shared_ptr<BlockNode> &tree, FileSymbols &fileSymbols) {
    std::vector<std::string> variables;
    auto symbolNode = std::make_shared<SymbolNode>(tree->start, tree->end);
    std::unordered_map<std::string, std::vector<FilePos>> variableUsages;
    doFindVariableDeclarations(tree, symbolNode, fileSymbols, variables, variableUsages);
    fileSymbols.symbolTree = symbolNode;

    // Now process variable usages
    for (auto it = variableUsages.begin(); it != variableUsages.end(); it++) {
        for (auto pos: variableUsages[it->first]) {
            auto declaration = findDeclaration(fileSymbols, it->first, pos);
            if (declaration != nullptr) {
                if (fileSymbols.variableUsages.count(declaration) == 0) {
                    fileSymbols.variableUsages[declaration] = std::vector<FilePos>();
                }

                fileSymbols.variableUsages[declaration].emplace_back(pos);
            }
        }
    }

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
        std::cout << "SymbolNode " << child->startPos.toStr() << " - " << child->endPos.toStr() << " Features: ["
                  << join(child->features, ",") << "]" << std::endl;
        doPrintSymbolTree(child, level + 2);
    }
}

void printSymbolTree(const std::shared_ptr<SymbolNode> &node) {
    std::cout << "SymbolNode" << " Features: [" << join(node->features, ",") << "]" << std::endl;
    doPrintSymbolTree(node, 2);
}

void printFileSymbols(FileSymbols &fileSymbols) {
    std::cout << std::endl << "Packages" << std::endl;
    for (auto package : fileSymbols.packages) {
        std::cout << package.packageName << " " << package.start.toStr() << "-" << package.end.toStr() << std::endl;
    }

    std::cout << std::endl << "Variables" << std::endl;
    printSymbolTree(fileSymbols.symbolTree);

    std::cout << std::endl << "Subroutines" << std::endl;
    for (auto function : fileSymbols.subroutines) {
        std::cout << function.toStr() << std::endl;
    }
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

SymbolMap getSymbolMap(const FileSymbols &fileSymbols, const FilePos &pos) {
    SymbolMap symbolMap;
    doGetSymbolMap(fileSymbols.symbolTree, pos, symbolMap);

    // Now add globals
    for (const auto &var: fileSymbols.globals) {
        // Note that scoped variables to take presedence over globals
        if (!symbolMap.count(var->name)) symbolMap[var->name] = var;
    }

    return symbolMap;
}

std::string Subroutine::toStr() {
    std::string str;
    auto nameStr = name.empty() ? "<ANOM>" : name;
    str = pos.toStr() + " " + nameStr + "()";

    if (!signature.empty()) {
        str += " signature=" + signature;
    }

    if (!prototype.empty()) {
        str += " prototype=" + prototype;
    }

    return str;
}
