//
// Created by Ben Banerjee-Richards on 2019-09-17.
//

#include "VarAnalysis.h"

bool ScopedVariable::isAccessibleAt(const FilePos &pos) {
    return insideRange(this->declaration, this->scopeEnd, pos);
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
                if (tokensNode->tokens[i].type == My || tokensNode->tokens[i].type == Our) {
                    bool isOur = tokensNode->tokens[i].type == Our;

                    auto nextToken = tokensNode->tokens[i + 1];
                    while (i < tokensNode->tokens.size() && nextToken.isWhitespaceNewlineOrComment()) {
                        i++;
                        nextToken = tokensNode->tokens[i];
                    }

                    if (nextToken.type == ScalarVariable || nextToken.type == HashVariable || nextToken.type == ArrayVariable) {
                        // We've got a definition!
                        if (isOur) {
                            auto package = findPackageAtPos(packages, nextToken.startPos);
                            variables.emplace_back(std::make_unique<OurVariable>(nextToken.data, nextToken.startPos, parentBlockNode->end, package));
                        }
                        else {
                            variables.emplace_back(std::make_unique<ScopedVariable>(nextToken.data, nextToken.startPos, parentBlockNode->end));
                        }
                    }
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