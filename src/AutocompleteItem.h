//
// Created by Ben Banerjee-Richards on 2019-11-14.
//

#ifndef PERLPARSER_AUTOCOMPLETEITEM_H
#define PERLPARSER_AUTOCOMPLETEITEM_H

#include <string>

struct AutocompleteItem {

    AutocompleteItem(const std::string &name, const std::string &detail);

    std::string name;
    std::string detail;
};


#endif //PERLPARSER_AUTOCOMPLETEITEM_H
