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
