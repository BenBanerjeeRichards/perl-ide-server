#include <iostream>
#include <chrono>
#include "Tokeniser.h"
#include "Parser.h"
#include "VarAnalysis.h"
#include "IOException.h"
#include "Test.h"
#include "PerlServer.h"
#include "PerlCommandLine.h"
#include "SymbolLoader.h"

struct TimeInfo {
    long long int tokenise;
    long long int parse;
    long long int analysis;
    long long int total;
};

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

FileSymbols analysisWithTime(const std::string &path, TimeInfo &timing, bool printTokens = false) {
    auto totalBegin = std::chrono::steady_clock::now();
    Tokeniser tokeniser(readFile(path));
    FileSymbols fileSymbols;

    auto begin = std::chrono::steady_clock::now();
    auto tokens = tokeniser.tokenise();
    auto end = std::chrono::steady_clock::now();
    timing.tokenise = std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count();

    begin = std::chrono::steady_clock::now();
    int partiallyParsed = -1;
    auto parseTree = buildParseTree(tokens, partiallyParsed);
    fileSymbols.partialParse = partiallyParsed;
    end = std::chrono::steady_clock::now();
    timing.parse = std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count();

    begin = std::chrono::steady_clock::now();
    parseFirstPass(parseTree, fileSymbols);
    fileSymbols.packages = parsePackages(parseTree);
    buildVariableSymbolTree(parseTree, fileSymbols);
    end = std::chrono::steady_clock::now();
    timing.analysis = std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count();
    auto totalEnd = std::chrono::steady_clock::now();

    timing.total = std::chrono::duration_cast<std::chrono::milliseconds>(totalEnd - totalBegin).count();
    return fileSymbols;
}

void testFiles() {
    auto perlFiles = globglob("/Users/bbr/honours/perl-dl/src/download/2/*");
    std::cout << "file,tokens,lines,total_ms,tok_ms,parse_ms,analysis_ms" << std::endl;
    for (auto file : perlFiles) {
        TimeInfo timing{};
        FileSymbols fileSymbols = analysisWithTime(file, timing);
        std::cout << file << "," << timing.total << ","
                  << timing.tokenise << "," << timing.parse << "," << timing.analysis << std::endl;

        if (auto badNode = findBadSymbolNode(fileSymbols.symbolTree)) {
            std::cout << std::endl << console::bold << console::red << "Bad SymbolNode found at position "
                      << badNode->startPos.toStr() << console::clear << std::endl;
            return;
        }

        if (fileSymbols.partialParse > -1) {
            std::cout << std::endl << console::bold << console::red << "Partial parse detected at line"
                      << fileSymbols.partialParse
                      << console::clear << std::endl;
            return;
        }

    }
}


void debugPrint(const std::string &path) {
    TimeInfo timeInfo{};
    FileSymbols fileSymbols = analysisWithTime(path, timeInfo, true);

    printFileSymbols(fileSymbols);

    std::cout << console::bold << std::endl << "Variables at position" << console::clear << std::endl;
    auto pos = FilePos(30, 1);
    auto map = getSymbolMap(fileSymbols, pos);
    for (const auto &varItem : map) {
        std::cout << varItem.second->toStr() << std::endl;
    }

    std::cout << console::bold << std::endl << "Variable usages" << console::clear << std::endl;
    for (auto it = fileSymbols.variableUsages.begin(); it != fileSymbols.variableUsages.end(); it++) {
        std::cout << it->first->toStr() << ": ";
        for (auto varPos: fileSymbols.variableUsages[it->first]) {
            std::cout << varPos.toStr() << " ";
        }

        std::cout << std::endl;
    }

    std::cout << console::bold << std::endl << "Imports" << console::clear << std::endl;
    for (auto import : fileSymbols.imports) {
        std::cout << import.toStr() << std::endl;
    }


    std::cout << std::endl << console::bold << "Timing" << console::clear << std::endl;
    std::cout << "Total: " << timeInfo.total << "ms" << std::endl;
    std::cout << "Tokenisation: " << timeInfo.tokenise << "ms" << std::endl;
    std::cout << "Parsing: " << timeInfo.parse << "ms" << std::endl;
    std::cout << "Var/Sub Analysis: " << timeInfo.analysis << "ms" << std::endl;

    auto badSymbol = findBadSymbolNode(fileSymbols.symbolTree);
    if (badSymbol != nullptr) {
        std::cout << std::endl << console::bold << console::red << "Bad SymbolNode found at position "
                  << badSymbol->startPos.toStr() << console::clear << std::endl;
    }

    if (fileSymbols.partialParse > -1) {
        std::cout << std::endl << console::bold << console::red << "Partial parse detected at line"
                  << fileSymbols.partialParse
                  << console::clear << std::endl;
    }
}


int main(int argc, char **args) {
    std::cout << join(splitPackage("Main::::Test'Package"), ", ") << std::endl;
    std::cout << join(splitPackage("Main::Test"), ", ") << std::endl;
    auto includePaths = getIncludePaths("/");
    std::cout << resolveModulePath(includePaths, splitPackage("XSLoader")) << std::endl;
    std::cout << resolveModulePath(includePaths, splitPackage("Math::::BigInt")) << std::endl;
    std::cout << resolvePath(includePaths, "Math/BigInt.pm") << std::endl;
    std::string file = "../perl/input.pl";

    loadSymbols("/Users/bbr/Documents/PerlInclude/main.pl");
    return 0;

    if (argc >= 2) file = args[1];

    if (argc == 2 && strncmp(args[1], "strtest", 7) == 0) {
        testFiles();
        return 0;
    }

    if (argc == 2 && strncmp(args[1], "test", 4) == 0) {
        runTests();
        return 0;
    }

    if (argc == 2 && strncmp(args[1], "serve", 6) == 0) {
        std::cout << "Started server on 1234" << std::endl;
        startAndBlock(1234);
        return 0;
    }

    if (argc == 3 && strncmp(args[1], "makeTest", 9) == 0) {
        auto name = std::string(args[2]);
        makeTest(name);
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
    if (argc == 5 && strncmp(args[1], "sub", 4) == 0) {
        auto pos = FilePos(std::atoi(args[3]), std::atoi(args[4]));
        auto vars = analysis::autocompleteSubs(args[2], pos);
        for (auto completion : vars) {
            std::cout << completion.name + " - " + completion.detail << std::endl;
        }
        return 0;
    }

    if (argc == 4) {
        auto pos = FilePos(std::atoi(args[2]), std::atoi(args[3]));
        for (const auto &c : analysis::autocompleteVariables(file, pos, 0)) {
            std::cout << c.name << std::endl << c.detail << std::endl;
        }
    } else if (argc > 1 && strncmp(args[1], "test", 4) == 0) {
        printFileTokens(args[2], true);
    } else {
        debugPrint(file);
    }

    //startAndBlock(1234);

    return 0;
}

