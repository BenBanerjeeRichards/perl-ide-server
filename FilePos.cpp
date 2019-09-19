//
// Created by Ben Banerjee-Richards on 2019-09-19.
//

#include "FilePos.h"

std::string FilePos::toStr() {
    return std::to_string(this->line) + ":" + std::to_string(this->col);
}
