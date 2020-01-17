//
// Created by Ben Banerjee-Richards on 2019-09-19.
//

#include "FilePos.h"

std::string FilePos::toStr() {
    return std::to_string(this->line) + ":" + std::to_string(this->col);
}

Range::Range(FilePos from, FilePos to) {
    this->from = from;
    this->to = to;
}

Range::Range(FilePos from, int symbolLength) {
    this->from = from;
    this->to = from;
    this->to.position += symbolLength;
    this->to.col += symbolLength;
}

std::string Range::toStr() {
    if (this->from.line == this->to.line) {
        return std::to_string(this->from.line) + ":" + std::to_string(this->from.col) + "-" +
               std::to_string(this->to.col);
    }

    return "[" + this->from.toStr() + "] - [" + this->to.toStr() + "]";
}

bool Range::operator==(const Range other) {
    return this->to == other.to && this->from == other.from;
}

