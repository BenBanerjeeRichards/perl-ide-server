//
// Created by Ben Banerjee-Richards on 2019-09-17.
//

#ifndef PERLPARSER_VARANALYSIS_H
#define PERLPARSER_VARANALYSIS_H

#include "Parser.h"
#include "Util.h"
#include <algorithm>

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
        return "our [" + this->declaration.toStr() + "] " + package + "::" + name;
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
    SymbolNode(const FilePos &startPos, const FilePos &endPos, std::vector<std::string> parentFeatures);

    // Variables declared in this scope
    std::vector<std::shared_ptr<Variable>> variables;

    // 'use feature ...'
    // These are lexically scoped and can also be turned off using 'no feature ...' (also lexically scoped)
    std::vector<std::string> features;

    // References to scoping start and end positions
    FilePos startPos;
    FilePos endPos;

    std::vector<std::shared_ptr<SymbolNode>> children;
};

struct Subroutine {
    FilePos pos;
    // Start and end of sub name to facilitate renaming
    FilePos nameStart;
    FilePos nameEnd;

    // Subroutine name, empty if anonymous
    std::string name;

    // TODO more processing on prototype/signatures to understand them
    std::string signature;
    std::string prototype;

    // Attributes
    std::vector<std::string> attributes;

    std::string toStr();
};

struct VariableUsage {
    FilePos pos;
    std::shared_ptr<Variable> variable;
};

struct FileSymbols {
    std::shared_ptr<SymbolNode> symbolTree;
    std::vector<PackageSpan> packages;
    std::vector<Subroutine> subroutines;

    // This is a temp measure until packages are fully implemented
    std::vector<std::shared_ptr<Variable>> globals;

    // Usages of each variable
    // Includes definition
    std::unordered_map<std::shared_ptr<Variable>, std::vector<FilePos>> variableUsages;
};


std::shared_ptr<SymbolNode> buildVariableSymbolTree(const std::shared_ptr<BlockNode> &tree, FileSymbols &fileSymbols);


std::string findPackageAtPos(const std::vector<PackageSpan> &packages, FilePos pos);

void printSymbolTree(const std::shared_ptr<SymbolNode> &node);

void printFileSymbols(FileSymbols& fileSymbols);

typedef std::unordered_map<std::string, std::shared_ptr<Variable>> SymbolMap;

SymbolMap getSymbolMap(const FileSymbols &fileSymbols, const FilePos &pos);

#endif //PERLPARSER_VARANALYSIS_H
