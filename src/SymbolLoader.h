//
// Created by Ben Banerjee-Richards on 27/12/2019.
//

#ifndef PERLPARSE_SYMBOLLOADER_H
#define PERLPARSE_SYMBOLLOADER_H

#include <string>
#include <unordered_map>
#include "Util.h"
#include "FileAnalysis.h"
#include "PerlCommandLine.h"
#include "Symbols.h"

FileSymbolMap loadAllFileSymbols(std::string path, std::string contextPath);

GlobalVariablesMap buildGlobalVariablesMap(const FileSymbolMap &fileSymbolsMap);


std::optional<Symbols> buildSymbols(std::string rootPath, std::string contextPath);

#endif //PERLPARSE_SYMBOLLOADER_H
