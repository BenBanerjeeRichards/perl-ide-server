//
// Created by Ben Banerjee-Richards on 2019-09-19.
//

#ifndef PERLPARSER_AUTOCOMPLETE_H
#define PERLPARSER_AUTOCOMPLETE_H

#include "VarAnalysis.h"
#include "Parser.h"
#include "Tokeniser.h"

struct AutocompleteItem {

    AutocompleteItem(const std::string &name, const std::string &detail);

    std::string name;
    std::string detail;
};

std::vector<AutocompleteItem> autocomplete(const std::string& filePath, FilePos);

#endif //PERLPARSER_AUTOCOMPLETE_H
