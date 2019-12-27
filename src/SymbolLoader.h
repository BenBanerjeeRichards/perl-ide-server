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

typedef std::unordered_map<std::string, FileSymbols> FileSymbolMap;

FileSymbolMap loadSymbols(std::string path);


#endif //PERLPARSE_SYMBOLLOADER_H
