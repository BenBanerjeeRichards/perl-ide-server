//
// Created by Ben Banerjee-Richards on 21/12/2019.
//

#ifndef PERLPARSE_SUBROUTINE_H
#define PERLPARSE_SUBROUTINE_H

#include "Util.h"

struct Subroutine {
    FilePos pos;
    // Start and end of sub name to facilitate renaming
    FilePos nameStart;
    FilePos nameEnd;

    // Package subroutine is declared in
    std::string package;

    // Subroutine name, empty if anonymous
    std::string name;

    // TODO more processing on prototype/signatures to understand them
    std::string signature;
    std::string prototype;

    // Attributes
    std::vector<std::string> attributes;

    std::string toStr();
};


#endif //PERLPARSE_SUBROUTINE_H
