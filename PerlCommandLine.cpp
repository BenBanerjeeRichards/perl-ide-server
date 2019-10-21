//
// Created by Ben Banerjee-Richards on 2019-10-18.
//

#include "PerlCommandLine.h"

RunResult runCommand(const PerlProject &project, const std::string &arguments) {
    redi::ipstream proc(project.perlPath + " " + arguments, redi::pstreams::pstdout | redi::pstreams::pstderr);
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

std::vector<std::string> getIncludePaths(PerlProject &project) {
    if (!project.includePaths.empty()) return project.includePaths;

    auto includeCommandRes = runCommand(project, "-e \"print join('\n', @INC)\"");

    // TODO Do proper path expansion
    for (auto & i : includeCommandRes.output) {
        if (i == ".") {
            i = project.rootDirectory;
        }
    }

    // Cache results
    project.includePaths.assign(includeCommandRes.output.begin(),includeCommandRes.output.end());
    return includeCommandRes.output;
}

std::string resolveModulePath(const PerlProject& project, const std::vector<std::string>& module) {
    for (auto path : project.includePaths) {
        std::string searchPath = path;
        for (const auto& modulePart : module) {
            path += "/" + modulePart;
        }

        path += ".pm";

        // See if this file exists
        std::ifstream f(path);
        if (f.good()) return path;
    }

    return "";
}

std::string RunResult::toStr() {
    if (error.empty()) return join(output, ",");
    return "STDOUT:\n" + (output.empty() ? "<NONE>" : join(output, ",")) + "\n\nSTDERROR:\n" + join(error, ",");
}
