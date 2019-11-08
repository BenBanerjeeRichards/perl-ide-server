//
// Created by Ben Banerjee-Richards on 2019-09-16.
//

#ifndef PERLPARSER_PARSER_H
#define PERLPARSER_PARSER_H

#include <utility>
#include <stack>
#include "Node.h"


std::shared_ptr<BlockNode> parse(std::vector<Token> tokens, int&);

std::vector<PackageSpan> parsePackages(std::shared_ptr<BlockNode> parent);

void printParseTree(std::shared_ptr<Node> root);

#endif //PERLPARSER_PARSER_H
