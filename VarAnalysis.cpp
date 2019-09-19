//
// Created by Ben Banerjee-Richards on 2019-09-17.
//

#include "VarAnalysis.h"

bool ScopedVariable::isAccessibleAt(const FilePos &pos) {
    return insideRange(this->declaration, this->scopeEnd, pos);
}


std::vector<ScopedVariable> findVariableDeclarations(const std::shared_ptr<Node> &tree) {
    // Only supports my for now
    std::vector<ScopedVariable> variables;

    for (const auto &child : tree->children) {
        // TODO replace with visitor pattern
        if (std::shared_ptr<BlockNode> blockNode = std::dynamic_pointer_cast<BlockNode>(child)) {
            auto recurseVars = findVariableDeclarations(child);
            variables.insert(variables.end(), recurseVars.begin(), recurseVars.end());
        }

        if (std::shared_ptr<TokensNode> tokensNode = std::dynamic_pointer_cast<TokensNode>(child)) {
            for (int i = 0; i < (int)tokensNode->tokens.size() - 1; i++) {
                if (tokensNode->tokens[i].type == My) {
                    auto nextToken = tokensNode->tokens[i + 1];
                    while (i < tokensNode->tokens.size() && nextToken.isWhitespaceNewlineOrComment()) {
                        i++;
                        nextToken = tokensNode->tokens[i];
                    }

                    if (nextToken.type == ScalarVariable || nextToken.type == HashVariable || nextToken.type == ArrayVariable) {
                        // We've got a definition!
                        variables.emplace_back(ScopedVariable(nextToken.data, nextToken.startPos, FilePos(0, 0)));
                    }
                }
            }
        }
    }

    return variables;
}