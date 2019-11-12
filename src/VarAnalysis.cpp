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


std::shared_ptr<Variable> makeVariable(int id, TokenType type, std::string name, FilePos declaration, FilePos scopeEnd,
                                       const std::vector<PackageSpan> &packages) {
    if (type == TokenType::Our) {
        auto package = findPackageAtPos(packages, declaration);
        return std::make_shared<OurVariable>(id, name, declaration, scopeEnd, package);
    } else if (type == TokenType::My || type == TokenType::State) {
        return std::make_shared<ScopedVariable>(id, name, declaration, scopeEnd);
    } else if (type == TokenType::Local) {
        return std::make_shared<LocalVariable>(id, name, declaration, scopeEnd);
    }

    return nullptr;
}

std::vector<std::shared_ptr<Variable>>
handleVariableTokens(const std::shared_ptr<TokensNode> &tokensNode, const std::vector<PackageSpan> &packages, int &i,
                     FilePos parentEnd, int &id) {
    std::vector<std::shared_ptr<Variable>> variables;
    TokenIterator tokensIter(tokensNode->tokens,
                             std::vector<TokenType>{TokenType::Whitespace, TokenType::Comment, TokenType::Newline}, i);

    auto tokenType = tokensIter.next().type;
    auto nextToken = tokensIter.next();

    if (nextToken.type == TokenType::ScalarVariable || nextToken.type == TokenType::HashVariable ||
        nextToken.type == TokenType::ArrayVariable) {
        // We've got a definition!
        if (auto var = makeVariable(id, tokenType, nextToken.data, nextToken.startPos, parentEnd, packages)) {
            variables.emplace_back(var);
            id++;
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
                if (auto var = makeVariable(id, tokenType, nextToken.data, nextToken.startPos, parentEnd, packages)) {
                    variables.emplace_back(var);
                    id++;
                }
            }
            nextToken = tokensIter.next();
        }

    }

    i = tokensIter.getIndex();
    return variables;
}

std::shared_ptr<GlobalVariable>
handleGlobalVariables(const Token &token, const std::vector<PackageSpan> &packages) {
    std::shared_ptr<GlobalVariable> global = nullptr;

    if (token.type == TokenType::ScalarVariable || token.type == TokenType::HashVariable ||
        token.type == TokenType::ArrayVariable) {
        // We have something in form $x = ... with no our/local/my/state...
        // Implies this is a global variable definition IF variable not previously declared
        auto package = findPackageAtPos(packages, token.startPos);
        global = std::make_shared<GlobalVariable>(1, token.data, token.startPos, package);
    }

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
    if (nextTok.type == TokenType::SubName) {
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
                                FileSymbols &fileSymbols, std::vector<std::string> &variables, int lastId) {

    for (const auto &child : tree->children) {
        if (std::shared_ptr<BlockNode> blockNode = std::dynamic_pointer_cast<BlockNode>(child)) {
            // Create new child for symbol tree
            auto symbolChild = std::make_shared<SymbolNode>(blockNode->start, blockNode->end, blockNode,
                                                            symbolNode->features);
            symbolNode->children.emplace_back(symbolChild);
            doFindVariableDeclarations(blockNode, symbolChild, fileSymbols, variables, lastId);
        }

        if (std::shared_ptr<TokensNode> tokensNode = std::dynamic_pointer_cast<TokensNode>(child)) {
            for (int i = 0; i < (int) tokensNode->tokens.size() - 1; i++) {
                auto tokenType = tokensNode->tokens[i].type;
                if (tokenType == TokenType::My || tokenType == TokenType::Our || tokenType == TokenType::Local ||
                    tokenType == TokenType::State) {
                    auto newVariables = handleVariableTokens(tokensNode, fileSymbols.packages, i, tree->end, lastId);
                    for (auto &variable : newVariables) {
                        symbolNode->variables.emplace_back(variable);
                        variables.emplace_back(variable->name);
                    }

                    i++;
                } else if (tokenType == TokenType::Assignment && i > 0) {
                    // Could be a global (package) variable
//                    if (auto global = handleGlobalVariables(tokensNode, variables, fileSymbols.packages, i, lastId)) {
//                        fileSymbols.globals.emplace_back(global);
//                        variables.emplace_back(global->fullyQualifiedName);
//                    }
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

void doFindDeclaration(const std::shared_ptr<SymbolNode> &symbolNode, const std::string &varName, const FilePos &varPos,
                       const std::shared_ptr<SymbolNode> &searchUntilInc, std::shared_ptr<Variable> &currentDecl) {
    for (const auto &variable : symbolNode->variables) {
        if (variable->name == varName && variable->declaration <= varPos) {
            currentDecl = variable;
        }
    }

    for (const auto &child : symbolNode->children) {
        if (insideRange(child->startPos, child->endPos, varPos)) {
            doFindDeclaration(child, varName, varPos, searchUntilInc, currentDecl);
        }
        if (child == searchUntilInc) return;
    }
}


std::shared_ptr<Variable>
findDeclaration(const std::shared_ptr<SymbolNode> &symbolTree, const std::shared_ptr<SymbolNode> &searchUntilInc,
                const std::string &varName, const FilePos &varPos) {
    std::shared_ptr<Variable> currentDecl = nullptr;
    doFindDeclaration(symbolTree, varName, varPos, searchUntilInc, currentDecl);
    return currentDecl;
}


/**
 * Scan tree to find variable declarations
 *
 * @param symbolNode - The tree containing the variable declarations for this scope
 * @param usages - The output map containing the usage of each variable. Maps each variable to list of positions where
 * is it used
 */
void
doFindVariableUsages(FileSymbols &fileSymbols, const std::shared_ptr<SymbolNode> &symbolNode,
                     std::unordered_map<std::shared_ptr<Variable>, std::vector<FilePos>> &usages) {
    for (auto &child : symbolNode->blockNode->children) {
        // BlockNode, don't bother will be considered when we consider children of current child node
        std::shared_ptr<TokensNode> tokensNode = std::dynamic_pointer_cast<TokensNode>(child);
        if (tokensNode == nullptr) continue;

        TokenIterator tokenIterator(tokensNode->tokens,
                                    std::vector<TokenType>{TokenType::Newline, TokenType::Whitespace,
                                                           TokenType::Comment});
        Token token = tokenIterator.next();
        while (token.type != TokenType::EndOfInput) {
            if (token.type == TokenType::ScalarVariable || token.type == TokenType::HashVariable ||
                token.type == TokenType::ArrayVariable) {
                // First find declaration
                // Need to consider context of variale to determine what it's declaration looks lile
                // e.g. $test refers to a scalar defined like my $test = ..., where as $test[0] refers to my @test = ...
                std::string canonicalName = token.data;
                Token accessor = tokenIterator.next();
                if (token.type == TokenType::ScalarVariable && accessor.type == TokenType::LSquareBracket) {
                    // Array access
                    canonicalName = "@" + token.data.substr(1, token.data.size() - 1);
                } else if (token.type == TokenType::ScalarVariable && accessor.type == TokenType::HashDerefStart) {
                    canonicalName = "%" + token.data.substr(1, token.data.size() - 1);
                }

                std::shared_ptr<Variable> declaration = findDeclaration(fileSymbols.symbolTree, symbolNode,
                                                                        canonicalName,
                                                                        token.startPos);
                if (declaration == nullptr) {
                    // If there is no previous declaration then treat the variable as a global
                    if (auto global = handleGlobalVariables(token, fileSymbols.packages)) {
                        fileSymbols.globals.emplace_back(global);
                    }
                } else {
                    if (usages.count(declaration) == 0) usages[declaration] = std::vector<FilePos>();
                    usages[declaration].emplace_back(token.startPos);
                }

            }
            token = tokenIterator.next();
        }
    }


    for (auto &symbolChild : symbolNode->children) {
        doFindVariableUsages(fileSymbols, symbolChild, usages);
    }

}

std::shared_ptr<SymbolNode> buildVariableSymbolTree(const std::shared_ptr<BlockNode> &tree, FileSymbols &fileSymbols) {
    std::vector<std::string> variables;
    auto symbolNode = std::make_shared<SymbolNode>(tree->start, tree->end, tree);
    doFindVariableDeclarations(tree, symbolNode, fileSymbols, variables, 0);
    fileSymbols.symbolTree = symbolNode;
    std::unordered_map<std::shared_ptr<Variable>, std::vector<FilePos>> usages;
    doFindVariableUsages(fileSymbols, symbolNode, usages);
    fileSymbols.variableUsages = usages;
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

SymbolNode::SymbolNode(const FilePos &startPos, const FilePos &endPos, std::shared_ptr<BlockNode> blockNode) :
        startPos(startPos), endPos(endPos), blockNode(blockNode) {}

SymbolNode::SymbolNode(const FilePos &startPos, const FilePos &endPos, std::shared_ptr<BlockNode> blockNode,
                       std::vector<std::string> parentFeatures)
        : startPos(startPos), endPos(endPos), blockNode(blockNode), features(parentFeatures) {
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
    std::cout << std::endl << "\e[1mPackages\e[0m" << std::endl;
    for (auto package : fileSymbols.packages) {
        std::cout << package.packageName << " " << package.start.toStr() << "-" << package.end.toStr() << std::endl;
    }

    std::cout << std::endl << "\e[1mVariables\e[0m" << std::endl;
    printSymbolTree(fileSymbols.symbolTree);

    std::cout << std::endl << console::bold << "Package variables" << console::clear << std::endl;
    for (auto global : fileSymbols.globals) {
        std::cout << global->toStr() << std::endl;
    }

    std::cout << std::endl << "\e[1mSubroutines\e[0m" << std::endl;
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


// Get canonical name for a variable
// Normalises equivalent variables e.g. $main::x = $main::'x = $main::::x = $main'x = ${main'x} = ${main::x} = ...
std::string getCanonicalVariableName(std::string variableName) {
    // First remove brackets if they exist
    std::string canonical = variableName;
    if (variableName.size() <= 1 || (variableName[0] != '@' && variableName[0] != '$' && variableName[0] != '%'))
        return variableName;

    if (variableName[1] == '{' && variableName[variableName.size() - 1] == '}') {
        canonical = variableName[0] + variableName.substr(2, variableName.size() - 3);
    }

    // Now replace ' with ::
    for (int i = 1; i < canonical.size(); i++) {
        if (canonical[i] == '\'') {
            canonical = canonical.substr(0, i) + "::" + canonical.substr(i + 1, canonical.size() - i);
        }
    }

    // Now replace :::: with ::
    for (int i = 1; i < (int) canonical.size(); i++) {
        if (canonical[i] == ':') {
            int j = 1;
            while (j < (int) canonical.size()) {
                if (canonical[i + j] != ':') break;
                j++;
            }

            int numDoubleColons = (int) j / 2;
            if (numDoubleColons > 1) {
                int deleteFrom = i + 2;
                int deleteTo = i + numDoubleColons * 2;
                canonical = canonical.substr(0, deleteFrom) + canonical.substr(deleteTo, canonical.size() - deleteTo);
            }

            i += 2;
        }
    }

    return canonical;
}

/**
 * Given a package variable name and current package, returns the fully qualified canonical package reference along
 * with the package the variable is in. e.g. - examples are (var in code, current package) -> (qualified name, package)
 *  ($hello, main)           -> ($main::hello, main)
 *  (${hello}, test)         -> ($test::hello, test)
 *  ($test::'hello, main)    -> ($test::hello, main)
 * @param packageVariableName Variable as it appears in the code
 * @param packageContext Package that the variable was found in
 */
PackageVariableName getFullyQualifiedVariableName(std::string packageVariableName, std::string packageContext) {
    auto canonicalName = getCanonicalVariableName(packageVariableName);
    if (packageVariableName.empty()) {
        return PackageVariableName("","","");
    }

    std::string sigil = std::string(1, canonicalName[0]);
    std::string withoutSigil = canonicalName.substr(1, canonicalName.size() - 1);
    std::vector<std::string> parts = split(withoutSigil, "::");

    // Remove actual name
    std::string variableName = parts[parts.size() - 1];
    parts.pop_back();

    // Join again for package name
    std::string packageNameFromVariable = parts.empty() ? "" : join(parts, "::");

    if (packageNameFromVariable.empty()) {
        // variable has no encoded package name - assume current package at location
        return PackageVariableName(sigil, packageContext, variableName);
    } else {
        // package name provided in variable name, use that instead
        return PackageVariableName(sigil, packageNameFromVariable, variableName);
    }
}

PackageVariableName::PackageVariableName(std::string sigil, std::string package, std::string name) {
    this->sigil = sigil;
    this->package = package;
    this->name = name;
}

std::string PackageVariableName::getFullName() {
    return sigil + package + "::" + name;
}

const std::string &PackageVariableName::getPackage() const {
    return package;
}

const std::string &PackageVariableName::getName() const {
    return name;
}

const std::string &PackageVariableName::getSigil() const {
    return sigil;
}

std::string PackageVariableName::toStr() {
    return this->sigil + "," + this->package + "," + this->name;
}

