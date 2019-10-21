//
// Created by Ben Banerjee-Richards on 2019-10-18.
//

#ifndef PERLPARSER_PERLCOMMANDLINE_H
#define PERLPARSER_PERLCOMMANDLINE_H

#include "PerlProject.h"
#include "lib/pstreams.h"
#include "Util.h"
#include <iostream>
#include <vector>
#include <fstream>

struct RunResult {
    // Each element represents a new line
    std::vector<std::string> output;
    std::vector<std::string> error;

    std::string toStr();
};

RunResult runCommand(const PerlProject &project, const std::string &arguments);

std::vector<std::string> getIncludePaths(PerlProject &project);

std::string resolveModulePath(const PerlProject &project, const std::vector<std::string>& module);


#endif //PERLPARSER_PERLCOMMANDLINE_H
