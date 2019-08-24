#include <iostream>
#include "Token.h"
#include "Tokeniser.h"
#include <fstream>
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

    std::string filenamesFull((std::istreambuf_iterator<char>(fileStream) ), (std::istreambuf_iterator<char>()    ) );
    std::stringstream filesStream(filenamesFull);
    std::string segment;
    std::vector<std::string> seglist;

    while(std::getline(filesStream, segment, '\n'))
    {
        std::replace(segment.begin(), segment.end(), '\r', ' ');
        seglist.push_back(segment);
    }
// BlueprintCompilerCppBackend
//    search(std::vector<std::string>{"BlueprintCompilerCppBackend"}, "BCCB",10);
    search(seglist, "BCCB",10);

    return 0;
}

int main() {
    std::ifstream fileStream("../test.pl");
    if (!fileStream.is_open()) {
        std::cerr << "Failed to open file!" << std::endl;
        return 1;
    }
    std::string program((std::istreambuf_iterator<char>(fileStream) ), (std::istreambuf_iterator<char>()    ) );
    Tokeniser tokeniser(program);
    auto token = tokeniser.nextToken();
    while (true) {
        std::cout << token.toString() << std::endl;
        token = tokeniser.nextToken();
    }

    std::cout << "DONE" << std::endl;
    return 0;
}