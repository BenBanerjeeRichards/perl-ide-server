#include <iostream>
#include <chrono>
#include "Tokeniser.h"
#include "Parser.h"
#include "VarAnalysis.h"
#include "Autocomplete.h"
#include "PerlCommandLine.h"

std::string CONSOLE_BOLD = "\e[1m";
std::string CONSOLE_DIM = "\e[37m";
std::string CONSOLE_CLEAR = "\e[0m";
std::string CONSOLE_RED = "\e[31m";

void printFileTokens(const std::string &file, bool includeLocation) {
    Tokeniser tokeniser(readFile(file));
    auto tokens = tokeniser.tokenise();

    for (auto token : tokens) {
        std::cout << token.startPos.position << " " << token.endPos.position << " " << token.toStr(false) << std::endl;
    }
}

// Bad symbol node = symbol node with end pos = 0:0
// Indicates parser has consumed entire file
std::shared_ptr<SymbolNode> findBadSymbolNode(std::shared_ptr<SymbolNode> node) {
    if (node->endPos.line == 0) return node;
    for (auto child : node->children) {
        auto res = findBadSymbolNode(child);
        if (res != nullptr) return res;
    }

    return nullptr;
}

void testFiles() {
    auto perlFiles = globglob("/Users/bbr/honours/perl-dl/src/download/1/*");
    for (auto file : perlFiles) {
        std::cout << file << std::endl;
        Tokeniser tokeniser(readFile(file));
        auto tokens = tokeniser.tokenise();
        auto parseTree = parse(tokens);
        FileSymbols fileSymbols;
        fileSymbols.packages = parsePackages(parseTree);
        auto symbolTree = buildVariableSymbolTree(parseTree, fileSymbols);

        if (auto badNode = findBadSymbolNode(symbolTree)) {
            std::cout << std::endl << CONSOLE_BOLD << CONSOLE_RED <<  "Bad SymbolNode found at position " << badNode->startPos.toStr() << CONSOLE_CLEAR <<  std::endl;
            return;
        }
    }
}

void basicOutput(std::string path) {
    // TODO enable second pass
    Tokeniser tokeniser(readFile(path), false);
    for (auto &token : tokeniser.tokenise()) {
        if (token.type == TokenType::Whitespace || token.type == TokenType::Newline ||
            token.type == TokenType::Comment)
            continue;
        std::cout << tokenTypeToString(token.type) << " ";
    }
}

void unitTest(std::string path) {
    Tokeniser tokeniser(readFile(path), false);
    for (auto &token : tokeniser.tokenise()) {
        if (token.type == TokenType::Whitespace || token.type == TokenType::Newline ||
            token.type == TokenType::Comment || token.type == TokenType::EndOfInput)
            continue;
        std::cout << token.toStr() << std::endl;
    }

}
int main(int argc, char **args) {
    std::string file = "../perl/input.pl";
    if (argc >= 2) file = args[1];

    if (argc == 2 && strncmp(args[1], "strtest", 7) == 0) {
        testFiles();
        return 0;
    }

    if (argc == 3 && strncmp(args[1], "unitTest", 8) == 0) {
        unitTest(std::string(args[2]));
        return 0;
    }

    if (argc == 3 && strncmp(args[1], "basicOutput", 7) == 0) {
        basicOutput(std::string(args[2]));
        return 0;
    }


    if (argc == 4) {
        auto pos = FilePos(std::atoi(args[2]), std::atoi(args[3]));
        for (const auto &c : autocomplete(file, pos)) {
            std::cout << c.name << std::endl << c.detail << std::endl;
        }
    } else if (argc > 1 && strncmp(args[1], "test", 4) == 0) {
        printFileTokens(args[2], true);
    } else {
        Tokeniser tokeniser(readFile(file));

        auto begin = std::chrono::steady_clock::now();
        auto tokens = tokeniser.tokenise();
        auto end = std::chrono::steady_clock::now();
        auto tokeniseTime = std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count();

        for (auto token : tokens) {
            if (token.type == TokenType::Comment || token.type == TokenType::Newline) std::cout << CONSOLE_DIM;
            std::cout << tokeniser.tokenToStrWithCode(token, true) << CONSOLE_CLEAR << std::endl;
        }
        std::cout << CONSOLE_CLEAR;

        begin = std::chrono::steady_clock::now();
        auto parseTree = parse(tokens);
        end = std::chrono::steady_clock::now();
        auto parseTime = std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count();

//        printParseTree(parseTree);

        FileSymbols fileSymbols;
        begin = std::chrono::steady_clock::now();
        fileSymbols.packages = parsePackages(parseTree);
        end = std::chrono::steady_clock::now();
        auto packageTime = std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count();

        begin = std::chrono::steady_clock::now();
        auto symbolTree = buildVariableSymbolTree(parseTree, fileSymbols);
        end = std::chrono::steady_clock::now();
        auto variableAnalysisTime = std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count();

        printFileSymbols(fileSymbols);

        std::cout << CONSOLE_BOLD << std::endl << "Variables at position" << CONSOLE_CLEAR << std::endl;
        auto pos = FilePos(30, 1);
        auto map = getSymbolMap(fileSymbols, pos);
        for (const auto &varItem : map) {
            std::cout << varItem.second->toStr() << std::endl;
        }

        std::cout << CONSOLE_BOLD << std::endl << "Variable usages" << CONSOLE_CLEAR << std::endl;
        for (auto it = fileSymbols.variableUsages.begin(); it != fileSymbols.variableUsages.end(); it++) {
            std::cout << it->first->toStr() << ": ";
            for (auto varPos: fileSymbols.variableUsages[it->first]) {
                std::cout << varPos.toStr() << " ";
            }

            std::cout << std::endl;
        }

        std::cout << std::endl << CONSOLE_BOLD << "Timing" << CONSOLE_CLEAR << std::endl;

        std::cout << "Tokenisation: " << tokeniseTime << "ms" << std::endl;
        std::cout << "Parsing: " << parseTime << "ms" << std::endl;
        std::cout << "Package analysis: " << packageTime << "ms" << std::endl;
        std::cout << "Var/Sub Analysis: " << variableAnalysisTime << "ms" << std::endl;

        auto badSymbol = findBadSymbolNode(fileSymbols.symbolTree);
        if (badSymbol != nullptr) {
            std::cout << std::endl << CONSOLE_BOLD << CONSOLE_RED <<  "Bad SymbolNode found at position " << badSymbol->startPos.toStr() << CONSOLE_CLEAR <<  std::endl;
        }

    }

    return 0;
}