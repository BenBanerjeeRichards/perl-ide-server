//
// Created by Ben Banerjee-Richards on 2019-09-19.
//

#ifndef PERLPARSER_AUTOCOMPLETE_H
#define PERLPARSER_AUTOCOMPLETE_H

#include "VarAnalysis.h"

std::vector<std::string> autocomplete(const std::string& filePath, FilePos);

#endif //PERLPARSER_AUTOCOMPLETE_H
