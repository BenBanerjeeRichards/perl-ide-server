//
// Created by Ben Banerjee-Richards on 2019-09-16.
//

#ifndef PERLPARSER_PARSER_H
#define PERLPARSER_PARSER_H

#include <utility>
#include <stack>
#include <optional>
#include "Node.h"
#include "Symbols.h"
#include "Variable.h"
#include "Package.h"
#include "Constants.h"

std::shared_ptr<BlockNode> buildParseTree(std::vector<Token> tokens, int &);

std::vector<PackageSpan> parsePackages(std::shared_ptr<BlockNode> parent);

void printParseTree(std::shared_ptr<Node> root);


void parseFirstPass(std::shared_ptr<BlockNode> tree, FileSymbols &fileSymbols, bool enableLocals);

#endif //PERLPARSER_PARSER_H
