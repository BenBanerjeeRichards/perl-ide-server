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
#include "Cache.h"

FileSymbolMap loadAllFileSymbols(std::string path, std::string contextPath, Cache &cache);
GlobalVariablesMap buildGlobalVariablesMap(const FileSymbolMap &fileSymbolsMap);
std::optional<Symbols> buildSymbols(std::string rootPath, std::string contextPath);

std::optional<Symbols> buildSymbols(std::string rootPath, std::string contextPath, Cache &cache);

#endif //PERLPARSE_SYMBOLLOADER_H
