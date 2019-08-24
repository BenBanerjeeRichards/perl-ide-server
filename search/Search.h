//
// Created by Ben Banerjee-Richards on 2019-08-24.
//

#ifndef PERLPARSER_SEARCH_H
#define PERLPARSER_SEARCH_H

#include <string>
#include <iostream>
#include <algorithm>
#include <fstream>
#include <utility>
#include <vector>
#include <cmath>
#include <regex>

struct SearchResult {

    SearchResult(const std::string& haystack, int score) {
        this->score = score;
        this->haystack = haystack;
    }

    std::string haystack;
    int score;
};


std::vector<SearchResult> search(const std::vector<std::string>& haystacks, const std::string& needle, int numResults);

// TODO remove
std::vector<int> getCamelIndexes(const std::string& str);

#endif //PERLPARSER_SEARCH_H
