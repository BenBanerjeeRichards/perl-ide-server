//
// Created by Ben Banerjee-Richards on 2019-09-17.
//

#include "VarAnalysis.h"


std::optional<GlobalVariable> handleGlobalVariables(const Token &varToken, const std::vector<PackageSpan> &packages) {
    if (!(varToken.type == TokenType::ScalarVariable || varToken.type == TokenType::HashVariable ||
          varToken.type == TokenType::ArrayVariable)) {
        return {};
    }

    // First check if it is a special variable
    // if it is, ignore it
    if (varToken.type == TokenType::ScalarVariable &&
        std::find(constant::SPECIAL_SCALARS.begin(), constant::SPECIAL_SCALARS.end(), varToken.data) !=
        constant::SPECIAL_SCALARS.end()) {
        return std::optional<GlobalVariable>();
    }
    if (varToken.type == TokenType::ArrayVariable &&
        std::find(constant::SPECIAL_ARRAYS.begin(), constant::SPECIAL_ARRAYS.end(), varToken.data) !=
        constant::SPECIAL_ARRAYS.end()) {
        return std::optional<GlobalVariable>();
    }
    if (varToken.type == TokenType::HashVariable &&
        std::find(constant::SPECIAL_HASHES.begin(), constant::SPECIAL_HASHES.end(), varToken.data) !=
        constant::SPECIAL_HASHES.end()) {
        return std::optional<GlobalVariable>();
    }

    if (varToken.type == TokenType::ScalarVariable && varToken.data.size() >= 2) {
        // Finally check for $<digit> variables
        auto digitStr = varToken.data.substr(1, varToken.data.size() - 1);
        bool isInt = true;

        for (auto c : digitStr) {
            if (!isdigit(c)) {
                isInt = false;
                break;
            }
        }

        if (isInt) return {};
    }

    auto package = findPackageAtPos(packages, varToken.startPos);
    auto globalVariable = getFullyQualifiedVariableName(varToken.data, package);
    globalVariable.setLocation(Range(varToken.startPos, varToken.endPos));
    return std::optional<GlobalVariable>(globalVariable);
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
                     std::unordered_map<std::shared_ptr<Variable>, std::vector<Range>> &usages) {
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
                // Need to consider context of variable to determine what it's declaration looks like
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
                        // IMPORTANT: hash for GlobalVariable DOES NOT take into account the global position
                        if (fileSymbols.globals.count(global) == 0) {
                            fileSymbols.globals[global] = std::vector<GlobalVariable>{};
                        }

                        fileSymbols.globals[global].emplace_back(global);
                    }
                } else {
                    if (usages.count(declaration) == 0) usages[declaration] = std::vector<Range>();
                    usages[declaration].emplace_back(Range(token.startPos, token.endPos));
                }

            } else if (token.type == TokenType::Name) {
                Token nameToken = token;
                auto peek = tokenIterator.peek();
                if (peek.type == TokenType::Operator && peek.data == "->") {
                    tokenIterator.next();
                    auto peekName = tokenIterator.peek();
                    if (peekName.type == TokenType::Name) {
                        tokenIterator.next();
                        // We have Name -> Name
                        // Combine into single token
                        nameToken.endPos = peekName.endPos;
                        nameToken.data = nameToken.data + "::" + peekName.data;
                    }
                }
                // Try to resolve to subroutine declaration
                auto currPackage = findPackageAtPos(fileSymbols.packages, nameToken.startPos);
                auto canonicalSubName = getCanonicalPackageName(nameToken.data);
                PackagedSymbol subSymbol = splitOnPackage(canonicalSubName, currPackage);

                // Now resolve
                auto key = subSymbol.package + "::" + subSymbol.symbol;
                if (fileSymbols.subroutineDeclarations.count(key) > 0) {
                    auto decl = fileSymbols.subroutineDeclarations[key];
                    if (fileSymbols.fileSubroutineUsages.count(*decl) == 0) {
                        fileSymbols.fileSubroutineUsages[*decl] = std::vector<SubroutineCode>();
                    }
                    fileSymbols.fileSubroutineUsages[*decl].emplace_back(
                            SubroutineCode(Range(nameToken.startPos, nameToken.endPos), nameToken.data));

                } else {
                    // For further processing later on
                    auto subUsage = SubroutineUsage(subSymbol.package, subSymbol.symbol, nameToken.data,
                                                    Range(nameToken.startPos, nameToken.endPos));
                    fileSymbols.possibleSubroutineUsages.emplace_back(subUsage);
                }
            }
            token = tokenIterator.next();
        }
    }


    for (auto &symbolChild : symbolNode->children) {
        doFindVariableUsages(fileSymbols, symbolChild, usages);
    }

}

void buildVariableSymbolTree(const std::shared_ptr<BlockNode> &tree, FileSymbols &fileSymbols) {
    std::unordered_map<std::shared_ptr<Variable>, std::vector<Range>> usages;
    doFindVariableUsages(fileSymbols, fileSymbols.symbolTree, usages);
    fileSymbols.variableUsages = usages;
}

void doPrintSymbolTree(const std::shared_ptr<SymbolNode> &node, int level) {
    for (const auto &variable : node->variables) {
        for (int i = 0; i < level; i++) std::cout << " ";
        std::cout << variable->toStr() << std::endl;
    }

    for (const auto &child : node->children) {
        for (int i = 0; i < level; i++) std::cout << " ";
        std::cout << "SymbolNode " << child->startPos.toStr() << " - " << child->endPos.toStr() << std::endl;
        doPrintSymbolTree(child, level + 2);
    }
}

void printSymbolTree(const std::shared_ptr<SymbolNode> &node) {
    std::cout << "SymbolNode" << " Features: [" << std::endl;
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
    for (auto function : fileSymbols.subroutineDeclarations) {
        std::cout << function.second->toStr() << std::endl;
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


/**
 * Get's the canonical name for a variale
 *
 * If a package is involved, there are many possible ways of writing the same variable name:
 *  $main::x = $main::'x = $main::::x = $main'x = ${main'x} = ${main::x} = ...
 * This funtion converts each one of the above into $main::x
 *
 * @param variableName The variable name, as it appears in the code
 * @return The canonical form of the variable
 */
std::string getCanonicalVariableName(std::string variableName) {
    // First remove brackets if they exist
    std::string canonical = variableName;
    if (variableName.size() <= 1 || (variableName[0] != '@' && variableName[0] != '$' && variableName[0] != '%')) {
        return variableName;
    }

    if (variableName[1] == '{' && variableName[variableName.size() - 1] == '}') {
        canonical = variableName[0] + variableName.substr(2, variableName.size() - 3);
    }

    auto package = canonical.substr(1, canonical.size() - 1);
    return canonical[0] + getCanonicalPackageName(package);
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
GlobalVariable getFullyQualifiedVariableName(const std::string &packageVariableName, std::string packageContext) {
    if (packageVariableName.empty()) {
        return GlobalVariable("", "", "", "");
    }

    std::string canonicalName = getCanonicalVariableName(packageVariableName);
    std::string sigil = std::string(1, canonicalName[0]);
    std::string withoutSigil = canonicalName.substr(1, canonicalName.size() - 1);
    PackagedSymbol packagedSymbol = splitOnPackage(withoutSigil, packageContext);
    return GlobalVariable(packageVariableName, sigil, packagedSymbol.package, packagedSymbol.symbol);
}


std::optional<VariableDeclarationWithUsages> findVariableAtLocation(FileSymbols &fileSymbols, FilePos location) {
    for (const auto &variable : fileSymbols.variableUsages) {
        if (insideRange(variable.first->declaration, variable.first->symbolEnd, location)) {
            // Found variable at declaration
            return std::optional<VariableDeclarationWithUsages>(
                    VariableDeclarationWithUsages(variable.first, variable.second));
        }

        // Now check every usage as well, as find-usages can be used on the usage as well as the declaration
        for (auto usage : variable.second) {
            if (insideRange(usage, location)) {
                return std::optional<VariableDeclarationWithUsages>(
                        VariableDeclarationWithUsages(variable.first, variable.second));
            }
        }
    }

    return std::optional<VariableDeclarationWithUsages>();
}

/**
 * Find usages for the variable located at the given location
 *
 * @param fileSymbols
 * @param location
 * @return list of usages in the current file
 */
std::vector<Range> findLocalVariableUsages(FileSymbols &fileSymbols, FilePos location) {
    auto maybeVariable = findVariableAtLocation(fileSymbols, location);
    if (!maybeVariable.has_value()) {
        // None found
        std::cout << "FindUsages - No symbol found at location " << location.toStr() << std::endl;
        return std::vector<Range>();
    }

    return maybeVariable.value().usages;
}

std::optional<FilePos> findVariableDeclaration(FileSymbols &fileSymbols, FilePos location) {
    auto maybeVariable = findVariableAtLocation(fileSymbols, location);
    if (!maybeVariable.has_value()) {
        // None found
        std::cout << "FindUsages - No symbol found at location " << location.toStr() << std::endl;
        return std::optional<FilePos>();
    }

    return std::optional<FilePos>(maybeVariable.value().declaration->declaration);

}

