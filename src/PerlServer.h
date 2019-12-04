//
// Created by Ben Banerjee-Richards on 2019-11-24.
//

#ifndef PERLPARSER_PERLSERVER_H
#define PERLPARSER_PERLSERVER_H

#include "stdafx.h"
#include "IOException.h"
#include "FileAnalysis.h"
#include "../lib/json.hpp"

using json = nlohmann::json;

void startAndBlock(int port);

#endif //PERLPARSER_PERLSERVER_H
