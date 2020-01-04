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
    autocompleteVariables(const std::string &filePath, FilePos location, char sigilContext);

    std::vector<AutocompleteItem> autocompleteSubs(const std::string &filePath, FilePos location);

    std::unordered_map<std::string, std::vector<Range>>
    findVariableUsages(const std::string &filePath, std::string contextPath, FilePos location, Cache &cache);

    std::optional<FilePos> findVariableDeclaration(const std::string &filePath, FilePos location);
}

#endif //PERLPARSE_FILEANALYSIS_H
