//
// Created by Ben Banerjee-Richards on 21/12/2019.
//

#ifndef PERLPARSE_SUBROUTINE_H
#define PERLPARSE_SUBROUTINE_H

#include "Util.h"

struct Subroutine {
    Range location;

    // Package subroutine is declared in
    std::string package;

    // Subroutine name, empty if anonymous
    std::string name;

    std::string signature;
    std::string prototype;


    std::string toStr();

    const std::string getFullName() const;

    bool operator==(const Subroutine &other) const;
};

// path does't belong in Subroutine as normally Subroutine is associated with a FileSymbols
struct SubroutineDecl {
    Subroutine subroutine;
    std::string path;

    SubroutineDecl(const Subroutine &subroutine, const std::string &path);

    SubroutineDecl(const SubroutineDecl &);

    SubroutineDecl();

    bool operator==(const SubroutineDecl &other) const;


};

namespace std {
    template<>
    struct hash<Subroutine> {
        std::size_t operator()(const Subroutine &sub) const {
            return std::hash<std::string>()(sub.getFullName());
        }
    };
}

namespace std {
    template<>
    struct hash<SubroutineDecl> {
        std::size_t operator()(const SubroutineDecl &sub) const {
            return std::hash<std::string>()(sub.subroutine.getFullName());
        }
    };
}


#endif //PERLPARSE_SUBROUTINE_H
