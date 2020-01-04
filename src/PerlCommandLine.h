//
// Created by Ben Banerjee-Richards on 2019-10-18.
//

#ifndef PERLPARSER_PERLCOMMANDLINE_H
#define PERLPARSER_PERLCOMMANDLINE_H

#include "PerlProject.h"
#include "../lib/pstreams.h"
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

RunResult runCommand(std::string command);

RunResult runCommand(const std::string &perlPath, const std::string &arguments);

std::vector<std::string> getIncludePaths(const std::string &contextPath);

std::string resolveModulePath(const std::vector<std::string> &includePaths, const std::vector<std::string> &module);

std::string resolvePath(const std::vector<std::string> &includePaths, const std::string &path);
#endif //PERLPARSER_PERLCOMMANDLINE_H
