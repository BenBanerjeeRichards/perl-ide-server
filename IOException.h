//
// Created by Ben Banerjee-Richards on 2019-08-29.
//

#include <exception>
#include <string>

#ifndef PERLPARSER_IOEXCEPTION_H
#define PERLPARSER_IOEXCEPTION_H

class IOException : std::exception {
public:
    explicit IOException(std::string reason) {
        this->reason = std::move(reason);
    }

    std::string reason;
};


#endif //PERLPARSER_IOEXCEPTION_H
