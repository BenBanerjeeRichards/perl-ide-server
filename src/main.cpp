#include <iostream>
#include <chrono>
#include "Tokeniser.h"
#include "Parser.h"
#include "VarAnalysis.h"
#include "IOException.h"
#include "Test.h"
#include "PerlServer.h"
#include "SymbolLoader.h"
#include "Serialize.h"

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
    fileSymbols.packages = parsePackages(parseTree);
    parseFirstPass(parseTree, fileSymbols);
    buildVariableSymbolTree(parseTree, fileSymbols);
    end = std::chrono::steady_clock::now();
    timing.analysis = std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count();
    auto totalEnd = std::chrono::steady_clock::now();

    timing.total = std::chrono::duration_cast<std::chrono::milliseconds>(totalEnd - totalBegin).count();

//    std::cout << "Names - " << std::endl;
//    for (auto token : tokens) {
//        if (token.type == TokenType::Name) {
//            std::cout << "[" + token.startPos.toStr() << "]" << token.data << std::endl;
//        }
//    }

//    auto prog = readFile(path);
//    for (auto token : tokens) {
//        std::cout << tokenToStrWithCode(token, prog) << std::endl;
//    }

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
//    return;
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

    std::cout << console::bold << std::endl << "Constants" << console::clear << std::endl;
    for (auto constant : fileSymbols.constants) {
        std::cout << constant.toStr() << std::endl;
    }

    std::cout << console::bold << std::endl << "Possible subroutinue usages" << console::clear << std::endl;
    for (auto usage : fileSymbols.possibleSubroutineUsages) {
        std::cout << usage.toStr() << std::endl;
    }

    std::cout << console::bold << std::endl << "File subroutine usages" << console::clear << std::endl;
    for (auto usage : fileSymbols.fileSubroutineUsages) {
        std::string usageRanges = "";
        for (auto range : usage.second) {
            usageRanges += range.toStr() + " ";
        }

        std::cout << usage.first.getFullName() << " - " << usageRanges << std::endl;
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


    auto begin = std::chrono::steady_clock::now();
    auto asJson = toJson(fileSymbols);
    writeFile(fileName(path) + ".json", asJson.dump());
    auto end = std::chrono::steady_clock::now();
    auto timeTaken = std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count();

//    begin = std::chrono::steady_clock::now();
//    auto newSyms = symbolNodeFromJson(asJson);
//    end = std::chrono::steady_clock::now();
//    auto timeTaken2 = std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count();


    std::cout << "DONE " << timeTaken << " - " << std::endl;
}


void buildSymbolsTest() {
    Cache cache;
    buildSymbols("/Users/bbr/Documents/PerlInclude/main.pl", "/Users/bbr/Documents/PerlInclude/main.pl", cache);
}

int main(int argc, char **args) {
    std::string file = "../perl/input.pl";

//    std::string file = "/System/Library/Perl/5.18/Math/BigFloat.pm";
    std::string arg1 = argc >= 2 ? std::string(args[1]) : "";
    std::string arg2 = argc >= 3 ? std::string(args[2]) : "";

    if (arg1 == "symbols") {
        buildSymbolsTest();
        return 0;
    }

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
    } else if (argc > 1 && strncmp(args[1], "test", 4) == 0) {
        printFileTokens(args[2], true);
    } else {
        debugPrint(file);
    }

    //startAndBlock(1234);

    return 0;
}

