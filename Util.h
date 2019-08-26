//
// Created by Ben Banerjee-Richards on 2019-08-25.
//

#ifndef PERLPARSER_UTIL_H
#define PERLPARSER_UTIL_H

#include <string>

std::string replace(std::string str, const std::string &what, const std::string &with);
int numOccurrences(const std::string &str, const std::string& sub);

#endif //PERLPARSER_UTIL_H
