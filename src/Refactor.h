//
// Created by Ben Banerjee-Richards on 03/02/2020.
//

#ifndef PERLPARSE_REFACTOR_H
#define PERLPARSE_REFACTOR_H

#include "FilePos.h"
#include "Symbols.h"
#include "Package.h"
#include "FileAnalysis.h"
#include <vector>
#include <unordered_map>


using std::cout;
using std::cerr;
using std::endl;
using std::unordered_map;
using std::string;
using std::vector;
using std::optional;

struct Replacement {
    Replacement(const Range &location, const std::string &replacement);

    Replacement();

    Range location;
    std::string replacement;

    int getLength();

    void applyPosDelta(int delta);
};

std::string doReplacements(std::string str, std::vector<Replacement> replacements);


#endif //PERLPARSE_REFACTOR_H
