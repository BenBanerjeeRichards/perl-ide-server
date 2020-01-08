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

namespace analysis {

    struct SymbolUsage {
        int line;
        int col;
        std::string sourceLine;

        SymbolUsage(int line, int col, const std::string &sourceLine);
    };


    enum class AnalysisDetail {
        // Do all analysis including local lexical usage
                FULL,
        // Only do analysis on stuff that is exported (subroutines and global package variables)
                PACKAGE_ONLY
    };


    typedef std::unordered_map<std::string, std::vector<FilePos>> UsagesMap;

    FileSymbols getFileSymbols(const std::string &path, AnalysisDetail analysisDetail);

    std::vector<AutocompleteItem>
    autocompleteVariables(const std::string &filePath, const std::string &contextPath, FilePos location,
                          std::vector<std::string> projectFiles, char sigilContext, Cache &cache);

    std::vector<AutocompleteItem> autocompleteSubs(const std::string &filePath, FilePos location);

    std::unordered_map<std::string, std::vector<Range>>
    findVariableUsages(const std::string &filePath, const std::string &contextPath, FilePos location,
                       std::vector<std::string> projectFiles, Cache &cache);

    std::optional<FilePos> findVariableDeclaration(const std::string &filePath, FilePos location);

    void indexProject(std::vector<std::string> projectFiles, Cache &cache);
}

#endif //PERLPARSE_FILEANALYSIS_H
