//
// Created by Ben Banerjee-Richards on 31/01/2020.
//

#ifndef PERLPARSE_SERIALIZE_H
#define PERLPARSE_SERIALIZE_H

#include <iostream>
#include "Token.h"
#include "FilePos.h"
#include "Symbols.h"
#include "../lib/json.hpp"

using json = nlohmann::json;
using std::cout;
using std::cerr;
using std::endl;

json toJson(SymbolNode &rootNode);

json toJson(std::shared_ptr<Variable> variable);

json toJson(FilePos &filePos);

std::shared_ptr<SymbolNode> symbolNodeFromJson(const json &j);

std::shared_ptr<Variable> variableFromJson(json j);

FilePos filePosFromJson(const json &j);

#endif //PERLPARSE_SERIALIZE_H
