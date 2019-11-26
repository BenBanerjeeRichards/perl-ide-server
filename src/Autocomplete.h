//
// Created by Ben Banerjee-Richards on 2019-09-19.
//

#ifndef PERLPARSER_AUTOCOMPLETE_H
#define PERLPARSER_AUTOCOMPLETE_H

#include "VarAnalysis.h"
#include "Parser.h"
#include "Tokeniser.h"

std::vector<AutocompleteItem> autocomplete(const std::string &filePath, FilePos, char sigilContext);

#endif //PERLPARSER_AUTOCOMPLETE_H
