//
// Created by Ben Banerjee-Richards on 2019-09-16.
//

#include "Parser.h"

// These are modules that have a special syntaxic meaning in perl e.g. `use warnings` turns on warnings, it does
// not search for a module called 'warnings'
std::vector<std::string> PRAGMATIC_MODULES = std::vector<std::string>{"attributes", "autodie", "autodie::exception",
                                                                      "autodie::exception::system", "autodie::hints",
                                                                      "autodie::skip", "autouse", "base", "bigint",
                                                                      "bignum", "bigrat", "blib", "bytes", "charnames",
                                                                      "constant", "deprecate", "diagnostics",
                                                                      "encoding", "encoding::warnings", "experimental",
                                                                      "feature", "fields", "filetest", "if", "integer",
                                                                      "less", "lib", "locale", "mro", "ok", "open",
                                                                      "ops", "overload", "overloading", "parent", "re",
                                                                      "sigtrapsort", "strict", "subs", "threads",
                                                                      "threads::shared", "utf8", "vars", "version",
                                                                      "vmsish", "warnings", "warnings::register"};

// Returns -1 if there is no next token
int nextTokenIdx(const std::vector<Token> &tokens, int currentIdx) {
    while (currentIdx < tokens.size() - 1) {
        currentIdx += 1;

        auto nextType = tokens[currentIdx].type;
        if (nextType != TokenType::Whitespace && nextType != TokenType::Newline) return currentIdx;
    }

    return -1;
}

void doBuildParseTree(const std::shared_ptr<BlockNode> &node, const std::vector<Token> &tokens, int &tokenIdx) {
    std::vector<Token> tokensAcc;
    while (tokenIdx < tokens.size()) {
        Token token = tokens[tokenIdx];
        tokensAcc.emplace_back(token);
        tokenIdx += 1;

        if (token.type == TokenType::LBracket) {
            node->children.emplace_back(std::make_shared<TokensNode>(tokensAcc));
            tokensAcc.clear();

            // Now create child
            auto child = std::make_shared<BlockNode>(token.startPos);

            node->children.emplace_back(child);
            doBuildParseTree(child, tokens, tokenIdx);
        } else if (token.type == TokenType::RBracket) {
            node->children.emplace_back(std::make_shared<TokensNode>(tokensAcc));
            node->end = token.endPos;
            tokensAcc.clear();
            return;
        }
    }

    // Add remaining tokens
    node->children.emplace_back(std::make_shared<TokensNode>(tokensAcc));
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

void addPackageSpan(std::vector<PackageSpan> &packageSpans, PackageSpan packageSpan) {
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

    bool isBlockPackage = false;

    for (int childIdx = 0; childIdx < (int) parent->children.size(); childIdx++) {
        std::shared_ptr<Node> child = parent->children[childIdx];
        if (std::shared_ptr<BlockNode> blockNode = std::dynamic_pointer_cast<BlockNode>(child)) {
            // Going into new scope, push current package onto stack
            if (!isBlockPackage) packageStack.push(packageStack.top());
            isBlockPackage = false;
            auto morePackages = doParsePackages(blockNode, packageStack, currentPackageStart);
            if (!morePackages.empty()) {
                int start = 0;
                if (!packageSpans.empty() &&
                    packageSpans[packageSpans.size() - 1].packageName == morePackages[0].packageName) {
                    // Combine two packages together
                    start = 1;
                    packageSpans[packageSpans.size() - 1].end = morePackages[0].end;
                }
                for (int i = start; i < (int) morePackages.size(); i++) {
                    packageSpans.emplace_back(std::move(morePackages[i]));
                }
            }
        }

        if (std::shared_ptr<TokensNode> tokensNode = std::dynamic_pointer_cast<TokensNode>(child)) {
            for (int i = 0; i < (int) tokensNode->tokens.size() - 1; i++) {
                if (tokensNode->tokens[i].type == TokenType::Package) {
                    auto nextToken = tokensNode->tokens[i + 1];
                    while (i < tokensNode->tokens.size() && nextToken.isWhitespaceNewlineOrComment()) {
                        i++;
                        nextToken = tokensNode->tokens[i];
                    }

                    if (nextToken.type == TokenType::Name) {
                        // Found a new package definition

                        // Check for a package block (i.e. package NAME {...}
                        // This applies only to the next scope
                        int nextTok = nextTokenIdx(tokensNode->tokens, i);
                        if (nextTok > -1 && tokensNode->tokens[nextTok].type == TokenType::LBracket &&
                            childIdx <= parent->children.size() - 2) {
                            if (std::dynamic_pointer_cast<BlockNode>(parent->children[childIdx + 1]) != nullptr) {
                                // Indeed we have the block syntax
                                isBlockPackage = true;
                            }
                        }

                        if (packageStack.empty()) {
                            // Package analysis failed
                            // FIXME Put a proper handling here
                            std::cerr << "Package analysis failed - package stack empty!";
                            return packageSpans;
                        }

                        std::string prevPackageName = packageStack.top();
                        if (!isBlockPackage) packageStack.pop();
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

std::shared_ptr<BlockNode> buildParseTree(std::vector<Token> tokens, int &incorrectNestingStart) {
    incorrectNestingStart = -1;
    auto node = std::make_shared<BlockNode>(FilePos(0, 0));
    node->end = tokens[tokens.size() - 1].endPos;
    int tokenIdx = 0;
    doBuildParseTree(node, tokens, tokenIdx);
    incorrectNestingStart = tokenIdx < tokens.size() ? tokens[tokens.size() - 1].endPos.line : -1;

    while (tokenIdx < tokens.size()) {
        doBuildParseTree(node, tokens, tokenIdx);
    }
    return node;
}

Subroutine handleSub(TokenIterator &tokenIter, FilePos subStart, std::vector<PackageSpan> &packages) {
    Subroutine subroutine;

    subroutine.pos = subStart;

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
    return subroutine;

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
handleVariableTokens(TokenIterator &tokensIter, TokenType varKwdTokenType, const std::vector<PackageSpan> &packages,
                     FilePos parentEnd, int &id) {
    std::vector<std::shared_ptr<Variable>> variables;

    auto nextToken = tokensIter.next();

    if (nextToken.type == TokenType::ScalarVariable || nextToken.type == TokenType::HashVariable ||
        nextToken.type == TokenType::ArrayVariable) {
        // We've got a definition!
        if (auto var = makeVariable(id, varKwdTokenType, nextToken.data, nextToken.startPos, nextToken.endPos,
                                    parentEnd,
                                    packages)) {
            variables.emplace_back(var);
            id++;
        }

        // Finally, if combined assignment then skip past Assignment token to prevent confusion with
        // package variables below
        nextToken = tokensIter.next();
        if (nextToken.type == TokenType::Assignment) {
            nextToken = tokensIter.next();
        }
    } else if (nextToken.type == TokenType::LParen) {
        // my ($x, $y) syntax - combined declaration. Consider every variable inside
        while (nextToken.type != TokenType::RParen && nextToken.type != TokenType::EndOfInput) {
            if (nextToken.type == TokenType::ScalarVariable || nextToken.type == TokenType::HashVariable ||
                nextToken.type == TokenType::ArrayVariable) {
                // Variable!
                if (auto var = makeVariable(id, varKwdTokenType, nextToken.data, nextToken.startPos, nextToken.endPos,
                                            parentEnd, packages)) {
                    variables.emplace_back(var);
                    id++;
                }
            }
            nextToken = tokensIter.next();
        }

    }

    return variables;
}

std::optional<Import> handleRequire(TokenIterator &tokenIter, FilePos location) {
    auto maybeString = tokenIter.tryGetString();
    if (maybeString.has_value()) {
        // require 'Math/Calc.pm'
        return Import(location, ImportType::Path, ImportMechanism::Require, maybeString.value(),
                      std::vector<std::string>());
    }

    Token next = tokenIter.next();
    if (next.type == TokenType::Name) {
        // require Math::Calc;
        return Import(location, ImportType::Module, ImportMechanism::Require, next.data, std::vector<std::string>());
    }

    return std::optional<Import>();
}

std::optional<Import> handleUse(TokenIterator &tokenIter, FilePos location) {
    Token token = tokenIter.next();
    std::string moduleName;
    std::vector<std::string> exportList;
    moduleName = token.data;
    if (token.type == TokenType::Name) {
        // Check if module name is pragmatic
        for (const auto &pragmatic : PRAGMATIC_MODULES) {
            if (moduleName == pragmatic) return std::optional<Import>();
        }
    } else {
        return std::optional<Import>();
    }

    token = tokenIter.next();
    if (token.type == TokenType::NumericLiteral || token.type == TokenType::VersionLiteral) {
        token = tokenIter.next();
    }

    // At this point we have `use Module Version?`
    // Could have list at the end
    if (token.type == TokenType::QuoteIdent) token = tokenIter.next();
    if (token.type == TokenType::StringStart) {
        token = tokenIter.next();
        exportList = split(token.data, " ");
    }

    return Import(location, ImportType::Module, ImportMechanism::Use, moduleName, exportList);
}


void doParseFirstPass(const std::shared_ptr<BlockNode> &tree, const std::shared_ptr<SymbolNode> &symbolNode,
                      FileSymbols &fileSymbols, std::vector<std::string> &variables, int lastId) {

    for (const auto &child : tree->children) {
        if (std::shared_ptr<BlockNode> blockNode = std::dynamic_pointer_cast<BlockNode>(child)) {
            // Create new child for symbol tree
            auto symbolChild = std::make_shared<SymbolNode>(blockNode->start, blockNode->end, blockNode,
                                                            symbolNode->features);
            symbolNode->children.emplace_back(symbolChild);
            doParseFirstPass(blockNode, symbolChild, fileSymbols, variables, lastId);
        }

        if (std::shared_ptr<TokensNode> tokensNode = std::dynamic_pointer_cast<TokensNode>(child)) {
            TokenIterator tokenIter(tokensNode->tokens,
                                    std::vector<TokenType>{TokenType::Whitespace, TokenType::Comment,
                                                           TokenType::Newline});
            Token token = tokenIter.next();

            while (token.type != TokenType::EndOfInput) {
                auto tokenType = token.type;
                if (tokenType == TokenType::My || tokenType == TokenType::Our || tokenType == TokenType::Local ||
                    tokenType == TokenType::State) {
                    auto newVariables = handleVariableTokens(tokenIter, tokenType, fileSymbols.packages, tree->end,
                                                             lastId);
                    for (auto &variable : newVariables) {
                        symbolNode->variables.emplace_back(variable);
                        variables.emplace_back(variable->name);
                    }

                } else if (tokenType == TokenType::Sub) {
                    auto sub = handleSub(tokenIter, token.startPos, fileSymbols.packages);
                    fileSymbols.subroutines.emplace_back(sub);
                } else if (tokenType == TokenType::Require) {
                    auto import = handleRequire(tokenIter, token.startPos);
                    if (import.has_value()) {
                        fileSymbols.imports.emplace_back(import.value());
                    }
                } else if (tokenType == TokenType::Use) {
                    auto import = handleUse(tokenIter, token.startPos);
                    if (import.has_value()) {
                        fileSymbols.imports.emplace_back(import.value());
                    }
                }

                token = tokenIter.next();
            }
        }
    }
}

void parseFirstPass(std::shared_ptr<BlockNode> tree, FileSymbols &fileSymbols) {
    std::vector<std::string> variables;
    auto symbolNode = std::make_shared<SymbolNode>(tree->start, tree->end, tree);
    doParseFirstPass(tree, symbolNode, fileSymbols, variables, 0);
    fileSymbols.symbolTree = symbolNode;
}