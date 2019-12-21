//
// Created by Ben Banerjee-Richards on 21/12/2019.
//

#include "Variable.h"

bool ScopedVariable::isAccessibleAt(const FilePos &pos) {
    return insideRange(this->declaration, this->scopeEnd, pos);
}

const std::string GlobalVariable::getFullName() const {
    return sigil + package + "::" + name;
}

const std::string &GlobalVariable::getPackage() const {
    return package;
}

const std::string &GlobalVariable::getName() const {
    return name;
}

const std::string &GlobalVariable::getSigil() const {
    return sigil;
}

std::string GlobalVariable::toStr() {
    return this->sigil + "," + this->package + "," + this->name;
}

const FilePos &GlobalVariable::getFilePos() const {
    return filePos;
}

void GlobalVariable::setFilePos(const FilePos &filePos) {
    GlobalVariable::filePos = filePos;
}


GlobalVariable::GlobalVariable(std::string sigil, std::string package, std::string name) {
    this->sigil = sigil;
    this->package = package;
    this->name = name;
}
