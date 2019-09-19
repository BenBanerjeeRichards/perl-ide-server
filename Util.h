//
// Created by Ben Banerjee-Richards on 2019-08-25.
//

#ifndef PERLPARSER_UTIL_H
#define PERLPARSER_UTIL_H

#include <string>
#include <glob.h>
#include <vector>
#include "Tokeniser.h"

std::string replace(std::string str, const std::string &what, const std::string &with);
int numOccurrences(const std::string &str, const std::string& sub);
std::vector<std::string> globglob(const std::string& pattern);
std::string fileName(const std::string& path);
std::string readFile(const std::string& path);
bool insideRange(FilePos start, FilePos end, FilePos pos);

#endif //PERLPARSER_UTIL_H
