//
// Created by Ben Banerjee-Richards on 2019-08-24.
//

#include "Search.h"

static int FIRST_LETTER_NO_MATCH = -10;
static int FIRST_LETTERS_MATCH_EACH = 3;
static int NO_CHAR_MATCH = -1;
static int CHAR_MATCHED = 1;
static int INITIAL_BONUS_N_CHARS = 3;
static int CONSECUTIVE_MATCH = 5;
static int CAMEL_MATCH = 20;

static int NUM_RESULTS_TO_RETURN = 10;
static std::regex CAMEL_REGEX("[a-z]([A-Z])|^([A-Z])|([A-Z])[a-z]");

std::vector<int> getCamelIndexes(const std::string &str) {
    std::smatch matches;
    std::string::const_iterator searchStart(str.begin());
    std::vector<int> indexes;

    while (std::regex_search(searchStart, str.cend(), matches, CAMEL_REGEX)) {
        for (size_t i = 1; i < matches.size(); ++i) {
            if (!matches[i].str().empty()) {
                // This position is relative to the previous match (as we use the const iterator)
                int pos = matches.position(i);
                if (!indexes.empty()) pos += indexes[indexes.size() - 1] + 1;
                indexes.emplace_back(pos);
                break;
            }
        }
        searchStart = matches.suffix().first;
    }

    return indexes;
}

int score(const std::string &haystack, const std::string &needle) {
    auto camelIndexes = getCamelIndexes(haystack);

    int score = 0;

    if (haystack.empty() || needle.empty()) return -100;
    if (haystack[0] != needle[0]) score += FIRST_LETTER_NO_MATCH;

    unsigned int nChars = std::min(haystack.size(), needle.size());
    int matchedTo = 0;
    for (unsigned int i = 0; i < nChars; i++) {
        int charMatch = haystack.find_first_of(needle[i], matchedTo);
        if (charMatch == std::string::npos) {
            score += NO_CHAR_MATCH;
        } else {
            int prevMatch = matchedTo;
            matchedTo = charMatch;
            if (i < INITIAL_BONUS_N_CHARS && i == matchedTo) {
                score += FIRST_LETTERS_MATCH_EACH;
            } else {
                score += CHAR_MATCHED;
            }

            // Consecutive chars
            if (matchedTo == prevMatch + 1) {
                score += CONSECUTIVE_MATCH;
            }

            // Finally consider camel case bonus
            if (std::find(camelIndexes.begin(), camelIndexes.end(), matchedTo) != camelIndexes.end()) {
                score += CAMEL_MATCH;
            }
        }
    }

    return score;
}

std::vector<SearchResult> search(const std::vector<std::string> &haystacks, const std::string &needle, int numResults) {
    std::vector<SearchResult> scores;
    for (const std::string &haystack : haystacks) {
        int inputScore = score(haystack, needle);
        if (inputScore > 0) scores.emplace_back(SearchResult(haystack, inputScore));
    }

    std::sort(scores.begin(), scores.end(), [](const SearchResult &a, const SearchResult &b) -> bool {
        return a.score > b.score;
    });

    int numToReturn = std::min((int) scores.size(), numResults);
    for (int i = 0; i < numToReturn; i++) {
        std::cout << scores[i].haystack << "  " << scores[i].score << std::endl;
    }
    return scores;
}

