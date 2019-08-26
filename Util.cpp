//
// Created by Ben Banerjee-Richards on 2019-08-26.
//

//
// Created by Ben Banerjee-Richards on 2019-08-25.
//

#include "Util.h"

std::string replace(std::string str, const std::string &what, const std::string &with) {
    std::string baseString = str;
    while (baseString.find(what) != std::string::npos) {
        baseString.replace(baseString.find(what), what.size(), with);
    }

    return baseString;
}

// Based on https://stackoverflow.com/a/22489298
int numOccurrences(const std::string &str, const std::string& sub) {
    int occurrences = 0;
    std::string::size_type pos = 0;
    while ((pos = str.find(sub, pos)) != std::string::npos) {
        ++occurrences;
        pos += sub.length();
    }
    return occurrences;
}

