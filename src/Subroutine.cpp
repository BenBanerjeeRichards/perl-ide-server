//
// Created by Ben Banerjee-Richards on 21/12/2019.
//

#include "Subroutine.h"

std::string Subroutine::toStr() {
    std::string str;
    auto nameStr = name.empty() ? "<ANOM>" : name;
    str = pos.toStr() + " " + package + "::" + nameStr + "()";

    if (!signature.empty()) {
        str += " signature=" + signature;
    }

    if (!prototype.empty()) {
        str += " prototype=" + prototype;
    }

    return str;
}

const std::string Subroutine::getFullName() const {
    return this->package + "::" + this->name;

}

bool Subroutine::operator==(const Subroutine &other) const {
    return this->package == other.package && this->name == other.name && (this->pos == other.pos);
}

SubroutineDecl::SubroutineDecl(const Subroutine &subroutine, const std::string &path) : subroutine(subroutine),
                                                                                        path(path) {}

bool SubroutineDecl::operator==(const SubroutineDecl &other) const {
    return other.subroutine.getFullName() == this->subroutine.getFullName();
}

SubroutineDecl::SubroutineDecl(const SubroutineDecl &old) {
    this->path = old.path;
    this->subroutine = old.subroutine;
}

SubroutineDecl::SubroutineDecl() {
    this->path = "";
    this->subroutine = Subroutine();
}

