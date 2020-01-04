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

// Stores information about a single file in isolation
struct FileSymbols {
    // Stores all lexically scoped information - for example `my` variables
    // No lexically scoped information is exported to other files via full package qualification
    std::shared_ptr<SymbolNode> symbolTree;

    // The list of package definitions
    // It is critical to know the current package at each location in the program to be able to qualify global
    // variables
    std::vector<PackageSpan> packages;

    // All subroutines defined in the file. As of now, these are all exported BUT TODO private subs are allowed
    std::vector<Subroutine> subroutines;

    // All `require` or `use` imports. `require` imports are treated as static use imports as we can't do runtime
    // analysis! `require $myvar` will of course fail
    std::vector<Import> imports;

    // Package globals, map from fully qualified name to usages
    // Different to lexical variabels as package variables don't have a declaration
    // This is as they are implicitly declared when they are used
    // And it is impossible to find a first usage without running the perl code
    // These are all exported
    std::unordered_map<GlobalVariable, std::vector<Range>> globals;

    // Debug information, FIXME to be removed when a better alternative is found
    int partialParse;

    // Usages of each lexically scoped variable DOES NOT INCLUDE GLOBALS!
    // Includes definition.
    std::unordered_map<std::shared_ptr<Variable>, std::vector<Range>> variableUsages;
};

// Map from <file path> of a perl file to the parsed symbol table for that specific file
typedef std::unordered_map<std::string, FileSymbols> FileSymbolMap;


struct GlobalVariablesMap {
    // Global Variable -> (File path -> [usages])
    // So GlobalVariablesMap[var]["perl.pm"] is the usages of global var in file "perl.pm"
    std::unordered_map<GlobalVariable, std::unordered_map<std::string, std::vector<Range>>> globalsMap;

    void addGlobal(GlobalVariable global, std::string path, std::vector<Range> usages);

    std::string toStr();
};

struct Symbols {
    // Root file is the file that the analysis started from
    // So if a.pl imports B.pm and C.pm, then we have a graph with a at the root with children {B, C}
    std::string rootFilePath;

    // Needed for analysis required by this specific file
    // e.g. local variable rename
    FileSymbols rootFileSymbols;

    // Global variables across all files
    GlobalVariablesMap globalVariablesMap;

    // Subroutines defined across all files
    std::vector<Subroutine> subroutines;
};


#endif //PERLPARSE_SYMBOLS_H