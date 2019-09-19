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
};

class ScopedVariable : public Variable {
public:
    ScopedVariable(const std::string &name, FilePos declaration, FilePos scopeEnd) {
        this->name = name;
        this->declaration = declaration;
        this->scopeEnd = scopeEnd;
    }

    bool isAccessibleAt(const FilePos &pos) override;

private:
    FilePos scopeEnd;
};

std::vector<ScopedVariable> findVariableDeclarations(const std::shared_ptr<Node> &tree);

#endif //PERLPARSER_VARANALYSIS_H
