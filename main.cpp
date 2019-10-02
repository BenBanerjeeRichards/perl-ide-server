#include <iostream>
#include "Tokeniser.h"
#include "Parser.h"
#include "VarAnalysis.h"
#include "Autocomplete.h"

void printFileTokens(const std::string &file, bool includeLocation) {
    Tokeniser tokeniser(readFile(file));
    auto tokens = tokeniser.tokenise();

    for (auto token : tokens) {
        std::cout << token.toStr(includeLocation) << std::endl;
    }
}

int main(int argc, char **args) {
    std::string file = "../perl/input.pl";

    if (argc >= 2) file = args[1];
//    printFileTokens(file, argc == 2);

    if (argc == 4) {
        auto pos = FilePos(std::atoi(args[2]), std::atoi(args[3]));
        for (const auto& c : autocomplete(file, pos)) {
            std::cout << c.name << std::endl << c.detail << std::endl;
        }
    } else if (argc > 1 && strncmp(args[1], "test", 4) == 0) {
        printFileTokens(args[2], true);
    } else {
        Tokeniser tokeniser(readFile(file));
        auto tokens = tokeniser.tokenise();

        for (auto token : tokens) {
            std::cout << tokeniser.tokenToStrWithCode(token, true) << std::endl;
        }

        auto parseTree = parse(tokens);
        printParseTree(parseTree);

        FileSymbols fileSymbols;
        fileSymbols.packages = parsePackages(parseTree);
        auto symbolTree = buildVariableSymbolTree(parseTree, fileSymbols);
        printFileSymbols(fileSymbols);


        std::cout << std::endl << "Variables at position" << std::endl;
        auto pos = FilePos(30, 1);
        auto map = getSymbolMap(symbolTree, pos);
        for (const auto& varItem : map) {
            std::cout << varItem.second->toStr() << std::endl;
        }

        std::cout << "Done" << std::endl;
    }

}