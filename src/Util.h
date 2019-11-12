//
// Created by Ben Banerjee-Richards on 2019-08-25.
//

#ifndef PERLPARSER_UTIL_H
#define PERLPARSER_UTIL_H

#include <string>
#include <glob.h>
#include <vector>
#include "FilePos.h"

namespace console {
    const std::string bold = "\e[1m";
    const std::string dim = "\e[37m";
    const std::string clear = "\e[0m";
    const std::string red = "\e[31m";

}


std::string replace(std::string str, const std::string &what, const std::string &with);
std::vector<std::string> globglob(const std::string& pattern);
std::string fileName(const std::string& path);
std::string readFile(const std::string& path);
bool insideRange(FilePos start, FilePos end, FilePos pos);
std::string join(const std::vector<std::string> &vec, const char *delim);
std::vector<std::string> split(std::string s, const std::string& delimiter);

#endif //PERLPARSER_UTIL_H
