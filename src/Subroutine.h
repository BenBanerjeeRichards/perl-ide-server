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

    std::string signature;
    std::string prototype;

    // Attributes
    std::vector<std::string> attributes;

    std::string toStr();

    const std::string getFullName() const;
};

struct SubroutineDecl {
    Subroutine subroutine;
    std::string path;
    Range location;
};


namespace std {
    template<>
    struct hash<Subroutine> {
        std::size_t operator()(const Subroutine &sub) const {
            return std::hash<std::string>()(sub.getFullName());
        }
    };
}


#endif //PERLPARSE_SUBROUTINE_H
