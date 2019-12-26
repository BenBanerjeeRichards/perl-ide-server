//
// Created by Ben Banerjee-Richards on 21/12/2019.
//

#ifndef PERLPARSE_SYMBOLS_H
#define PERLPARSE_SYMBOLS_H


#include <string>
#include <unordered_map>
#include "Util.h"
#include "Variable.h"
#include "Subroutine.h"
#include "Node.h"
#include "Package.h"


enum class ImportType {
    Module, Path
};

enum class ImportMechanism {
    Use, Require
};

struct Import {
    Import(const FilePos &location, ImportType type, ImportMechanism mechanism, const std::string &data,
           const std::vector<std::string> &exports);

    FilePos location;
    ImportType type;
    ImportMechanism mechanism;
    std::string data;
    std::vector<std::string> exports;

    std::string toStr();
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


struct FileSymbols {
    std::shared_ptr<SymbolNode> symbolTree;
    std::vector<PackageSpan> packages;
    std::vector<Subroutine> subroutines;
    std::vector<Import> imports;

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


#endif //PERLPARSE_SYMBOLS_H
