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

namespace analysis {

    struct SymbolUsage {
        int line;
        int col;
        std::string sourceLine;

        SymbolUsage(int line, int col, const std::string &sourceLine);
    };

    typedef std::unordered_map<std::string, std::vector<FilePos>> UsagesMap;

    FileSymbols getFileSymbols(const std::string &path);

    std::vector<AutocompleteItem>
    autocompleteVariables(const std::string &filePath, FilePos location, char sigilContext);

    std::vector<AutocompleteItem> autocompleteSubs(const std::string &filePath, FilePos location);

    std::unordered_map<std::string, std::vector<Range>>
    findVariableUsages(const std::string &filePath, std::string contextPath, FilePos location);

    std::optional<FilePos> findVariableDeclaration(const std::string &filePath, FilePos location);
}

#endif //PERLPARSE_FILEANALYSIS_H
