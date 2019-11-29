//
// Created by Ben Banerjee-Richards on 2019-09-17.
//

#include "VarAnalysis.h"

bool ScopedVariable::isAccessibleAt(const FilePos &pos) {
    return insideRange(this->declaration, this->scopeEnd, pos);
}


std::shared_ptr<Variable>
makeVariable(int id, TokenType type, std::string name, FilePos declaration, FilePos symbolEnd, FilePos scopeEnd,
             const std::vector<PackageSpan> &packages) {
    if (type == TokenType::Our) {
        auto package = findPackageAtPos(packages, declaration);
        return std::make_shared<OurVariable>(id, name, declaration, symbolEnd, scopeEnd, package);
    } else if (type == TokenType::My || type == TokenType::State) {
        return std::make_shared<ScopedVariable>(id, name, declaration, symbolEnd, scopeEnd);
    } else if (type == TokenType::Local) {
        return std::make_shared<LocalVariable>(id, name, declaration, symbolEnd, scopeEnd);
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
        if (auto var = makeVariable(id, tokenType, nextToken.data, nextToken.startPos, nextToken.endPos, parentEnd,
                                    packages)) {
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
                if (auto var = makeVariable(id, tokenType, nextToken.data, nextToken.startPos, nextToken.endPos,
                                            parentEnd, packages)) {
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


std::optional<GlobalVariable> handleGlobalVariables(const Token &token, const std::vector<PackageSpan> &packages) {
    if (token.type == TokenType::ScalarVariable || token.type == TokenType::HashVariable ||
        token.type == TokenType::ArrayVariable) {
        auto package = findPackageAtPos(packages, token.startPos);
        auto globalVariable = getFullyQualifiedVariableName(token.data, package);
        globalVariable.setFilePos(token.startPos);
        return std::optional<GlobalVariable>(globalVariable);
    }

    return std::optional<GlobalVariable>();
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

std::optional<Subroutine>
handleSub(const std::shared_ptr<TokensNode> &tokensNode, std::vector<PackageSpan> &packages, int &i) {
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

    subroutine.package = findPackageAtPos(packages, subroutine.pos);

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
                } else if (tokenType == TokenType::Sub) {
                    auto sub = handleSub(tokensNode, fileSymbols.packages, i);
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
                    auto globalOptional = handleGlobalVariables(token, fileSymbols.packages);
                    if (globalOptional.has_value()) {
                        GlobalVariable global = globalOptional.value();
                        if (fileSymbols.globals.count(global) == 0) {
                            fileSymbols.globals[global] = std::vector<FilePos>{global.getFilePos()};
                        } else {
                            fileSymbols.globals[global].emplace_back(global.getFilePos());
                        }
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
    for (const auto &package : packages) {
        if (insideRange(package.start, package.end, pos)) return package.packageName;
    }

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
        std::cout << global.first.getFullName() << " ";
        for (auto decl : global.second) {
            std::cout << decl.toStr() << " ";
        }
        std::cout << std::endl;
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
    return symbolMap;
}

std::string variableForCompletion(std::string variable, char sigilContext) {
    if (variable.empty()) return "";
    if (variable[0] == sigilContext) return variable;
    std::string variableWithoutSigil = variable.substr(1, variable.size() - 1);
    if (variable[0] == '@' && sigilContext == '$') return "$" + variableWithoutSigil;
    if (variable[0] == '%' && sigilContext == '$') return "$" + variableWithoutSigil;
    if (variable[0] == '%' && sigilContext == '@') return "@" + variableWithoutSigil;
    return "";
}


std::vector<AutocompleteItem>
variableNamesAtPos(const FileSymbols &fileSymbols, const FilePos &filePos, char sigilContext) {
    SymbolMap symbolMap = getSymbolMap(fileSymbols, filePos);
    std::vector<AutocompleteItem> variables;

    // Now add the globals
    std::string currentPackage = findPackageAtPos(fileSymbols.packages, filePos);
    // For globals in the current package, they won't require a fully qualified identifier and so could clash
    // If there is a clash, then insert the fully qualified identifier, otherwise just insert the name
    for (const auto &global : fileSymbols.globals) {
        if (global.first.getPackage() == currentPackage) {
            if (symbolMap.count(global.first.getName()) == 0) {
                auto variableName = variableForCompletion(global.first.getSigil() + global.first.getName(),
                                                          sigilContext);
                if (!variableName.empty()) {
                    variables.emplace_back(AutocompleteItem(variableName, global.first.getFullName()));
                }
            } else {
                auto variableName = variableForCompletion(global.first.getFullName(), sigilContext);
                if (!variableName.empty()) {
                    variables.emplace_back(AutocompleteItem(variableName, ""));
                }
            }
        } else {
            auto variableName = variableForCompletion(global.first.getFullName(), sigilContext);
            if (!variableName.empty()) {
                variables.emplace_back(AutocompleteItem(variableName, ""));
            }
        }
    }

    // Now add the symbol map
    for (const auto &symbolVal : symbolMap) {
        auto variableName = variableForCompletion(symbolVal.first, sigilContext);
        if (!variableName.empty()) {
            variables.emplace_back(AutocompleteItem(variableName, ""));
        }
    }

    return variables;
}

std::string Subroutine::toStr() {
    std::string str;
    auto nameStr = name.empty() ? "<ANOM>" : name;
    str = pos.toStr() + " " + package + "::" + nameStr + "()";

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
GlobalVariable getFullyQualifiedVariableName(std::string packageVariableName, std::string packageContext) {
    auto canonicalName = getCanonicalVariableName(packageVariableName);
    if (packageVariableName.empty()) {
        return GlobalVariable("", "", "");
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
        return GlobalVariable(sigil, packageContext, variableName);
    } else {
        // package name provided in variable name, use that instead
        return GlobalVariable(sigil, packageNameFromVariable, variableName);
    }
}

/**
 * Find usages for the variable located at the given location
 *
 * @param fileSymbols
 * @param location
 * @return list of usages in the current file
 */
std::vector<FilePos> variable::findVariableUsages(FileSymbols &fileSymbols, FilePos location) {
    for (auto variable : fileSymbols.variableUsages) {
        if (insideRange(variable.first->declaration, variable.first->symbolEnd, location)) {
            // Found variable
            return variable.second;
        }

        // Now check every usage as well, as find-usages can be used on the usage as well as the declaration
        for (auto usage : variable.second) {
            FilePos end = usage;
            end.col += variable.first->name.size();
            end.position += variable.first->name.size();
            if (insideRange(usage, end, location)) {
                return variable.second;
            }
        }
    }

    // None found
    std::cout << "FindUsages - No symbol found at location " << location.toStr() << std::endl;
    return std::vector<FilePos>();
}

GlobalVariable::GlobalVariable(std::string sigil, std::string package, std::string name) {
    this->sigil = sigil;
    this->package = package;
    this->name = name;
}

const std::string GlobalVariable::getFullName() const {
    return sigil + package + "::" + name;
}

const std::string &GlobalVariable::getPackage() const {
    return package;
}

const std::string &GlobalVariable::getName() const {
    return name;
}

const std::string &GlobalVariable::getSigil() const {
    return sigil;
}

std::string GlobalVariable::toStr() {
    return this->sigil + "," + this->package + "," + this->name;
}

const FilePos &GlobalVariable::getFilePos() const {
    return filePos;
}

void GlobalVariable::setFilePos(const FilePos &filePos) {
    GlobalVariable::filePos = filePos;
}

