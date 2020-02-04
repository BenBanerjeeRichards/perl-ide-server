//
// Created by Ben Banerjee-Richards on 2019-11-28.
//

#ifndef PERLPARSE_FILEANALYSIS_H
#define PERLPARSE_FILEANALYSIS_H

#include "VarAnalysis.h"
#include "Tokeniser.h"
#include "Parser.h"
#include "Symbols.h"
#include "SymbolLoader.h"
#include "Cache.h"
#include "Refactor.h"

using std::cout;
using std::cerr;
using std::endl;
using std::unordered_map;
using std::string;
using std::vector;
using std::optional;

namespace analysis {

    struct Declaration {
        std::string path;
        FilePos pos;
    };

    typedef std::unordered_map<std::string, std::vector<FilePos>> UsagesMap;

    FileSymbols getFileSymbols(const std::string &path);

    std::vector<AutocompleteItem>
    autocompleteVariables(const std::string &filePath, const std::string &contextPath, FilePos location,
                          std::vector<std::string> projectFiles, char sigilContext, Cache &cache);

    std::vector<AutocompleteItem>
    autocompleteSubs(const std::string &filePath, const std::string &contextPath, FilePos location,
                     std::vector<std::string> projectFiles, Cache &cache);

    std::optional<std::unordered_map<std::string, std::vector<Range>>>
    findVariableUsages(const std::string &filePath, const std::string &contextPath, FilePos location,
                       Symbols &symbols);

    std::optional<FilePos> findVariableDeclaration(const std::string &filePath, FilePos location);

    void indexProject(std::vector<std::string> projectFiles, Cache &cache);

    std::optional<analysis::Declaration>
    findSubroutineDeclaration(const std::string &filePath, const std::string &contextPath, FilePos location,
                              std::vector<std::string> projectFiles, Cache &cache);

    std::unordered_map<std::string, std::vector<Range>>
    findUsages(const std::string &filePath, const std::string &contextPath, FilePos location,
               std::vector<std::string> projectFiles, Cache &cache);

    std::optional<std::unordered_map<std::string, std::vector<Range>>>
    findSubroutineUsages(const std::string &filePath, const std::string &contextPath, FilePos location,
                         Symbols &symbols);

    void renameSymbol(const std::string &filePath, FilePos location, std::string renameTo,
                      std::vector<std::string> projectFiles, Cache &cache);

    bool isSymbol(const std::string &filePath, FilePos location);

    optional<unordered_map<string, vector<SubroutineCode>>>
    findSubroutineUsagesCode(const std::string &filePath, const std::string &contextPath, FilePos location,
                             Symbols &symbols);

    optional<unordered_map<string, vector<GlobalVariable>>>
    findGlobalVariableUsages(const string &filePath, const string &contextPath, FilePos location, Symbols &symbols);

    optional<GlobalVariable>
    findGlobalVariable(const string &filePath, const string &contextPath, FilePos location, Symbols &symbols);

    optional<vector<Range>>
    findLocalVariableUsages(const string &filePath, const string &contextPath, FilePos location, Symbols &symbols);

    optional<SubroutineDecl> doFindSubroutineDeclaration(string contextPath, FilePos location, Symbols &symbols);
}

#endif //PERLPARSE_FILEANALYSIS_H
