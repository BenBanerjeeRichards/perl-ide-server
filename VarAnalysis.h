//
// Created by Ben Banerjee-Richards on 2019-09-17.
//

#ifndef PERLPARSER_VARANALYSIS_H
#define PERLPARSER_VARANALYSIS_H

#include "Parser.h"
#include "Util.h"

struct Variable {
    std::string name;
    FilePos declaration;

    virtual bool isAccessibleAt(const FilePos &pos) {
        return false;
    }

    virtual std::string toStr() { return ""; }
};

class ScopedVariable : public Variable {
public:
    ScopedVariable(const std::string &name, FilePos declaration, FilePos scopeEnd) {
        this->name = name;
        this->declaration = declaration;
        this->scopeEnd = scopeEnd;
    }

    bool isAccessibleAt(const FilePos &pos) override;

    std::string toStr() override {
        return "[" + this->declaration.toStr() + "] " + name;
    }

private:
    FilePos scopeEnd;
};


class OurVariable : public ScopedVariable {
public:
    OurVariable(const std::string &name, FilePos declaration, FilePos scopeEnd, const std::string &package)
            : ScopedVariable(name, declaration, scopeEnd) {
        this->package = package;
    }

    std::string package;

    std::string toStr() override {
        return "[" + this->declaration.toStr() + "] " + package + "::" + name;
    }

};

std::vector<std::unique_ptr<Variable>>
findVariableDeclarations(const std::shared_ptr<Node> &tree, const std::vector<PackageSpan> &packages);

std::string findPackageAtPos(const std::vector<PackageSpan> &packages, FilePos pos);

#endif //PERLPARSER_VARANALYSIS_H
