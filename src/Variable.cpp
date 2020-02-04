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

Range GlobalVariable::getLocation() const {
    return this->location;
}

void GlobalVariable::setLocation(const Range &filePos) {
    this->location = filePos;
}

/**
 * Return the location at the end of the variable in the code
 * e.g
 *      unless $x; $Ben::test = 45;
 *                 ^        ^
 *      First ^ is filePos, second ^ is endPOs
 *
 * @return
 */
FilePos GlobalVariable::getEndPos() {
    return this->location.to;
}


GlobalVariable::GlobalVariable(std::string codeName, std::string sigil, std::string package, std::string name) {
    this->sigil = sigil;
    this->package = package;
    this->name = name;
    this->codeName = codeName;
}

const std::string GlobalVariable::getCodeName() const {
    return this->codeName;
}
