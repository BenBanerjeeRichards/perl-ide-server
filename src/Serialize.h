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

json toJson(const FilePos &filePos);

json toJson(Range &range);

json toJson(PackageSpan &packageSpan);

json toJson(Subroutine &sub);

json toJson(SubroutineUsage &subUsage);

json toJson(Constant &subUsage);

json toJson(Import &import);

json toJson(const GlobalVariable &globalVar);

json toJson(FileSymbols &fileSymbols);

std::shared_ptr<SymbolNode> symbolNodeFromJson(const json &j);

std::shared_ptr<Variable> variableFromJson(json j);

FilePos filePosFromJson(const json &j);

PackageSpan packageSpanFromJson(const json &j);

Range rangeFromJson(const json &j);

Subroutine subFromJson(const json &j);

SubroutineUsage subUsageFromJson(const json &j);

Constant constantFromJson(const json &j);

Import importFromJson(const json &j);

GlobalVariable globalVarFromJson(const json &j);

#endif //PERLPARSE_SERIALIZE_H
