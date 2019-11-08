//
// Created by Ben Banerjee-Richards on 2019-08-22.
//

#include<iostream>
#include<string>
#include <utility>

#ifndef PERLPARSER_TOKENISEEXCEPTION_H
#define PERLPARSER_TOKENISEEXCEPTION_H

class TokeniseException : std::exception {
public:
    explicit TokeniseException(std::string reason) {
        this->reason = std::move(reason);
    }

    std::string reason;
};

#endif //PERLPARSER_TOKENISEEXCEPTION_H
