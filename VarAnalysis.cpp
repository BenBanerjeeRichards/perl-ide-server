//
// Created by Ben Banerjee-Richards on 2019-09-17.
//

#include "VarAnalysis.h"

bool ScopedVariable::isAccessibleAt(const FilePos &pos) {
    return insideRange(this->declaration, this->scopeEnd, pos);
}


bool GlobalVariable::isAccessibleAt(const FilePos &pos) {
    // Globals available everywhere in current file
    return true;
}


std::vector<std::unique_ptr<Variable>> findVariableDeclarations(const std::shared_ptr<Node> &tree, const std::vector<PackageSpan>& packages) {
    // Only supports my for now
    std::vector<std::unique_ptr<Variable>> variables;

    for (const auto &child : tree->children) {
        // TODO replace with visitor pattern
        if (std::shared_ptr<BlockNode> blockNode = std::dynamic_pointer_cast<BlockNode>(child)) {
            std::vector<std::unique_ptr<Variable>> recurseVars = findVariableDeclarations(child, packages);
            for (auto&& var : recurseVars) {
                variables.emplace_back(std::move(var));
            }
        }

        if (std::shared_ptr<TokensNode> tokensNode = std::dynamic_pointer_cast<TokensNode>(child)) {
            std::shared_ptr<BlockNode> parentBlockNode = std::dynamic_pointer_cast<BlockNode>(tree);

            for (int i = 0; i < (int)tokensNode->tokens.size() - 1; i++) {
                auto tokenType = tokensNode->tokens[i].type;
                if (tokenType == My || tokenType == Our || tokenType == Local || tokenType == State) {

                    auto nextToken = tokensNode->tokens[i + 1];
                    while (i < tokensNode->tokens.size() && nextToken.isWhitespaceNewlineOrComment()) {
                        i++;
                        nextToken = tokensNode->tokens[i];
                    }

                    if (nextToken.type == ScalarVariable || nextToken.type == HashVariable || nextToken.type == ArrayVariable) {
                        // We've got a definition!
                        if (tokenType == Our) {
                            auto package = findPackageAtPos(packages, nextToken.startPos);
                            variables.emplace_back(std::make_unique<OurVariable>(nextToken.data, nextToken.startPos, parentBlockNode->end, package));
                        }
                        else if (tokenType == My || tokenType == State){
                            variables.emplace_back(std::make_unique<ScopedVariable>(nextToken.data, nextToken.startPos, parentBlockNode->end));
                        } else if (tokenType == Local) {
                            variables.emplace_back(std::make_unique<LocalVariable>(nextToken.data, nextToken.startPos, parentBlockNode->end));
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

                    if (prevToken.type == ScalarVariable || prevToken.type == HashVariable || prevToken.type == ArrayVariable) {
                        // We have something in form $x = ... with no our/local/my/state...
                        // Implies this is a global variable definition IF variable not previously declared
                        bool isDeclared = false;
                        for (const std::unique_ptr<Variable>& var : variables) {
                            if (var->name == prevToken.data) {
                                isDeclared = true;
                                break;
                            }
                        }

                        if (!isDeclared) {
                            auto package = findPackageAtPos(packages, prevToken.startPos);
                            variables.emplace_back(std::make_unique<GlobalVariable>(prevToken.data, prevToken.startPos, package));
                        }
                    }

                    i = prevI + 1;
                }
            }
        }
    }

    return variables;
}

std::string findPackageAtPos(const std::vector<PackageSpan>& packages, FilePos pos) {
    // TODO replace with binary search
    for (const auto& package : packages){
        if (insideRange(package.start, package.end, pos)) return package.packageName;
    }

    std::cerr << "Failed to find package span at pos " << pos.toStr() << std::endl;
    return "main";
}
