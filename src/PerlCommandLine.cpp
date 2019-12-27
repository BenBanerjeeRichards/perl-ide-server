//
// Created by Ben Banerjee-Richards on 2019-10-18.
//

#include "PerlCommandLine.h"

RunResult runCommand(std::string perlPath, const std::string &arguments) {
    redi::ipstream proc(perlPath + " " + arguments, redi::pstreams::pstdout | redi::pstreams::pstderr);
    std::string line;
    RunResult res;

    while (std::getline(proc.out(), line)) {
        res.output.emplace_back(line);
    }
    proc.clear();

    // read child's stderr
    while (std::getline(proc.err(), line)) {
        res.error.emplace_back(line);
    }

    return res;
}

std::vector<std::string> getIncludePaths(const std::string &contextPath) {
    auto includeCommandRes = runCommand("perl", "-e \"print join('\n', @INC)\"");

    // TODO Do proper path expansion
    for (auto & i : includeCommandRes.output) {
        if (i == ".") {
            i = contextPath;
        }
    }

    return includeCommandRes.output;
}

std::string resolveModulePath(const std::vector<std::string> &includePaths, const std::vector<std::string> &module) {
    std::string path;
    for (const auto &modulePart : module) {
        path += "/" + modulePart;
    }

    path = path.substr(1, path.size() - 1);
    // Module syntax assumes .pm extension
    return resolvePath(includePaths, path + ".pm");
}

std::string resolvePath(const std::vector<std::string> &includePaths, const std::string &path) {
    for (auto includePath : includePaths) {
        auto fullPath = includePath + "/" + path;
//        std::cout << "\t" << fullPath << std::endl;
        std::ifstream f(fullPath);
        if (f.good()) return fullPath;
    }

    return "";
}

std::string RunResult::toStr() {
    if (error.empty()) return join(output, ",");
    return "STDOUT:\n" + (output.empty() ? "<NONE>" : join(output, ",")) + "\n\nSTDERROR:\n" + join(error, ",");
}
