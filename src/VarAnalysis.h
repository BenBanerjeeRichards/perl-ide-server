//
// Created by Ben Banerjee-Richards on 2019-09-17.
//

#ifndef PERLPARSER_VARANALYSIS_H
#define PERLPARSER_VARANALYSIS_H

#include "AutocompleteItem.h"
#include "Token.h"
#include "Util.h"
#include "Node.h"
#include "Variable.h"
#include "Subroutine.h"
#include <algorithm>
#include <utility>
#include <unordered_map>
#include "Symbols.h"


std::shared_ptr<SymbolNode> buildVariableSymbolTree(const std::shared_ptr<BlockNode> &tree, FileSymbols &fileSymbols);

std::string findPackageAtPos(const std::vector<PackageSpan> &packages, FilePos pos);

void printSymbolTree(const std::shared_ptr<SymbolNode> &node);

void printFileSymbols(FileSymbols &fileSymbols);

typedef std::unordered_map<std::string, std::shared_ptr<Variable>> SymbolMap;

SymbolMap getSymbolMap(const FileSymbols &fileSymbols, const FilePos &pos);

std::string getCanonicalVariableName(std::string variableName);

std::vector<AutocompleteItem>
variableNamesAtPos(const FileSymbols &fileSymbols, const FilePos &filePos, char sigilContext);


GlobalVariable getFullyQualifiedVariableName(std::string packageVariableName, std::string packageContext);

std::vector<FilePos> findVariableUsages(FileSymbols &fileSymbols, FilePos location);

std::optional<FilePos> findVariableDeclaration(FileSymbols &fileSymbols, FilePos location);


#endif //PERLPARSER_VARANALYSIS_H
