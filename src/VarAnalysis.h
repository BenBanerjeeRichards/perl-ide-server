//
// Created by Ben Banerjee-Richards on 2019-09-17.
//

#ifndef PERLPARSER_VARANALYSIS_H
#define PERLPARSER_VARANALYSIS_H

#include "AutocompleteItem.h"
#include "Token.h"
#include "Util.h"
#include "Node.h"
#include <algorithm>
#include <utility>
#include <unordered_map>

class GlobalVariable {
    // Doesn't include sigil
    std::string package;

    std::string name;

    std::string sigil;

    FilePos filePos;


public:

    GlobalVariable(std::string sigil, std::string package, std::string name);

    const std::string getFullName() const;

    const std::string &getPackage() const;

    const std::string &getName() const;

    const std::string &getSigil() const;

    const FilePos &getFilePos() const;

    void setFilePos(const FilePos &filePos);

    std::string toStr();

    bool operator==(const GlobalVariable &other) const {
        return this->getFullName() == other.getFullName();
    }

};

struct Variable {
    std::string name;
    FilePos declaration;
    int id;

    bool operator==(const Variable &other) const {
        return this->id == other.id;
    }

    virtual bool isAccessibleAt(const FilePos &pos) {
        return false;
    }

    virtual std::string toStr() { return ""; }

    virtual std::string getDetail() { return ""; }
};


namespace std {
    template<>
    struct hash<Variable> {
        std::size_t operator()(const Variable &var) const {
            return std::hash<int>()(var.id);
        }
    };
}

namespace std {
    template<>
    struct hash<GlobalVariable> {
        std::size_t operator()(const GlobalVariable &var) const {
            return std::hash<std::string>()(var.getFullName());
        }
    };
}


class ScopedVariable : public Variable {
public:
    ScopedVariable(int id, const std::string &name, FilePos declaration, FilePos scopeEnd) {
        this->name = name;
        this->declaration = declaration;
        this->scopeEnd = scopeEnd;
        this->id = id;
    }

    bool isAccessibleAt(const FilePos &pos) override;

    std::string toStr() override {
        return "[" + this->declaration.toStr() + "] (#" + std::to_string(id) + ")" + name;
    }


private:
    FilePos scopeEnd;
};


class OurVariable : public ScopedVariable {
public:
    OurVariable(int id, const std::string &name, FilePos declaration, FilePos scopeEnd, const std::string &package)
            : ScopedVariable(id, name, declaration, scopeEnd) {
        this->package = package;
    }

    std::string package;

    std::string getDetail() override {
        return this->package;
    }

    std::string toStr() override {
        if (name.empty()) return "<NO NAME>";
        return "[" + this->declaration.toStr() + "] (#" + std::to_string(id) + ") our " + name[0] + package + "::" +
               name.substr(1, name.size() - 1);
    }
};

// Local variables ARE NOT SCOPED
// But we can't determine places where they are used
// As it depends on the call stack
// So instead we treat them as scoped variables and ban renames on them when possible
class LocalVariable : public ScopedVariable {
public:
    LocalVariable(int id, const std::string &name, FilePos declaration, FilePos scopeEnd)
            : ScopedVariable(id, name, declaration, scopeEnd) {
    }

    std::string toStr() override {
        return "[" + this->declaration.toStr() + "] (#" + std::to_string(id) + ")" + name + " (LOCAL)";
    }
};

class SymbolNode {
public:
    SymbolNode(const FilePos &startPos, const FilePos &endPos, std::shared_ptr<BlockNode> blockNode);

    SymbolNode(const FilePos &startPos, const FilePos &endPos, std::shared_ptr<BlockNode> blockNode,
               std::vector<std::string> parentFeatures);

    // Reference to tokens
    std::shared_ptr<BlockNode> blockNode;

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

struct FileSymbols {
    std::shared_ptr<SymbolNode> symbolTree;
    std::vector<PackageSpan> packages;
    std::vector<Subroutine> subroutines;

    // Package globals, map from fully qualified name to usages
    // Different to lexical variabels as package variables don't have a declaration
    // This is as they are implicitly declared when they are used
    // And it is impossible to find a first usage without running the perl code
    std::unordered_map<GlobalVariable, std::vector<FilePos>> globals;

    int partialParse;

    // Usages of each variable
    // Includes definition.
    std::unordered_map<std::shared_ptr<Variable>, std::vector<FilePos>> variableUsages;
};


std::shared_ptr<SymbolNode> buildVariableSymbolTree(const std::shared_ptr<BlockNode> &tree, FileSymbols &fileSymbols);


std::string findPackageAtPos(const std::vector<PackageSpan> &packages, FilePos pos);

void printSymbolTree(const std::shared_ptr<SymbolNode> &node);

void printFileSymbols(FileSymbols &fileSymbols);

typedef std::unordered_map<std::string, std::shared_ptr<Variable>> SymbolMap;

SymbolMap getSymbolMap(const FileSymbols &fileSymbols, const FilePos &pos);

std::string getCanonicalVariableName(std::string variableName);

std::vector<AutocompleteItem> variableNamesAtPos(const FileSymbols &fileSymbols, const FilePos &filePos);


GlobalVariable getFullyQualifiedVariableName(std::string packageVariableName, std::string packageContext);


#endif //PERLPARSER_VARANALYSIS_H
