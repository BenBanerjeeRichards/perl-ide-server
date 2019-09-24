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

    virtual std::string getDetail() { return ""; }
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

class GlobalVariable : public Variable {
public:
    GlobalVariable(const std::string &name, FilePos declaration, const std::string &package) {
        this->name = name;
        this->declaration = declaration;
        this->package = package;
    }

    bool isAccessibleAt(const FilePos &pos) override;

    std::string toStr() override {
        return "[" + this->declaration.toStr() + "] " + name + " (GLOBAL)";
    }

    std::string getDetail() override {
        return this->package;
    }

    std::string package;

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

    std::string getDetail() override {
        return this->package;
    }

    std::string toStr() override {
        return "[" + this->declaration.toStr() + "] " + package + "::" + name;
    }
};

// Local variables ARE NOT SCOPED
// But we can't determine places where they are used
// As it depends on the call stack
// So instead we treat them as scoped variables and ban renames on them when possible
class LocalVariable : public ScopedVariable {
public:
    LocalVariable(const std::string &name, FilePos declaration, FilePos scopeEnd)
            : ScopedVariable(name, declaration, scopeEnd) {
    }

    std::string toStr() override {
        return "[" + this->declaration.toStr() + "] " + name + " (LOCAL)";
    }
};

class SymbolNode {
public:
    SymbolNode(const FilePos &startPos, const FilePos &endPos);

    std::vector<std::shared_ptr<Variable>> variables;

    // References to scoping start and end positions
    FilePos startPos;
    FilePos endPos;

    std::vector<std::shared_ptr<SymbolNode>> children;
};


std::shared_ptr<SymbolNode>
buildVariableSymbolTree(const std::shared_ptr<BlockNode> &tree, const std::vector<PackageSpan> &packages);


std::string findPackageAtPos(const std::vector<PackageSpan> &packages, FilePos pos);

void printSymbolTree(const std::shared_ptr<SymbolNode> &node);

typedef std::unordered_map<std::string, std::shared_ptr<Variable>> SymbolMap;

SymbolMap getSymbolMap(const std::shared_ptr<SymbolNode> &symbolTree, const FilePos &pos);

#endif //PERLPARSER_VARANALYSIS_H
