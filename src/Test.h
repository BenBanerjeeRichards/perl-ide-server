//
// Created by Ben Banerjee-Richards on 2019-11-01.
//

#ifndef PERLPARSER_TEST_H
#define PERLPARSER_TEST_H

#include "Token.h"
#include "Tokeniser.h"
#include "IOException.h"
#include "Util.h"
#include <fstream>

void runTests();
void makeTest(std::string &name);

#endif //PERLPARSER_TEST_H
