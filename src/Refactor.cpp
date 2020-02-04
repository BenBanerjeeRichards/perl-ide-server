//
// Created by Ben Banerjee-Richards on 03/02/2020.
//

#include "Refactor.h"

int Replacement::getLength() {
    return 1 + (location.to.position - location.from.position);
}

void Replacement::applyPosDelta(int delta) {
    this->location.from.position += delta;
    this->location.to.position += delta;
}

Replacement::Replacement(const Range &location, const std::string &replacement) : location(location),
                                                                                  replacement(replacement) {}

Replacement::Replacement() {
    this->location = Range();
    this->replacement = "";
}

std::string doReplacements(std::string str, std::vector<Replacement> replacements) {
    auto oldContents = str;
    for (int i = 0; i < replacements.size(); i++) {
        auto replacement = replacements[i];
        int len = (replacement.location.to.position - replacement.location.from.position) + 1;
        str.replace(replacement.location.from.position, len, replacement.replacement);


        int delta = replacement.replacement.size() - len;
        for (int j = i + 1; j < replacements.size(); j++) {
            replacements[j].location.from.position += delta;
            replacements[j].location.to.position += delta;

        }
    }

    return str;
}
