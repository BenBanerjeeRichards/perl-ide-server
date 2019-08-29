#include <iostream>
#include "Tokeniser.h"
#include "search/Search.h"
#include <sstream>

int searchMain() {
//    std::string s{"UnrealAudioDeviceXAudio2.cpp"};
//
//    for (auto i : getCamelIndexes(s)) {
//        std::cout << s[i];
//    }
//
//    std::cout << std::endl;

    std::ifstream fileStream("../filenames.txt");
    if (!fileStream.is_open()) {
        std::cerr << "Failed to open file!" << std::endl;
        return 1;
    }

    std::string filenamesFull((std::istreambuf_iterator<char>(fileStream)), (std::istreambuf_iterator<char>()));
    std::stringstream filesStream(filenamesFull);
    std::string segment;
    std::vector<std::string> seglist;

    while (std::getline(filesStream, segment, '\n')) {
        std::replace(segment.begin(), segment.end(), '\r', ' ');
        seglist.push_back(segment);
    }
// BlueprintCompilerCppBackend
//    search(std::vector<std::string>{"BlueprintCompilerCppBackend"}, "BCCB",10);
    search(seglist, "BCCB", 10);

    return 0;
}

void printTokenStandard(const Token& token, bool printLocation = true) {
    std::string tokenStr = tokenTypeToString(token.type);
    if (!token.data.empty()) {
        auto d1 = replace(token.data, "\n", "\\n");
        auto d2 = replace(d1, "\r", "\\r");
        tokenStr += "(" + d2 + ")";
    }

    if (printLocation) {
        std::cout << token.startPos.line << ":" << token.startPos.col << " " << token.endPos.line << ":"
                  << token.endPos.col << " ";
    }

    std::cout << tokenStr << std::endl;
}

int main(int argc, char **args) {
    if (argc == 2) {
        std::ifstream fileStream(args[1]);
        if (!fileStream.is_open()) {
            std::cerr << "Failed to open file " << args[1] << std::endl;
            return 1;
        }
        std::string program((std::istreambuf_iterator<char>(fileStream)), (std::istreambuf_iterator<char>()));
        Tokeniser tokeniser(program);
        auto token = tokeniser.nextToken();

        while (token.type != TokenType::EndOfInput) {
            printTokenStandard(token);
            token = tokeniser.nextToken();
        }

        return 0;
    }

    std::ifstream fileStream("../perl/test.pl");
    if (!fileStream.is_open()) {
        std::cerr << "Failed to open file!" << std::endl;
        return 1;
    }
    std::string program((std::istreambuf_iterator<char>(fileStream)), (std::istreambuf_iterator<char>()));
    Tokeniser tokeniser(program);
    auto token = tokeniser.nextToken();

    while (token.type != TokenType::EndOfInput) {
        printTokenStandard(token, false);
        token = tokeniser.nextToken();
    }

    return 0;
}