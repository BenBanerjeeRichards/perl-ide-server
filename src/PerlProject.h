//
// Created by Ben Banerjee-Richards on 2019-10-18.
//

#ifndef PERLPARSER_PERLPROJECT_H
#define PERLPARSER_PERLPROJECT_H

#include <string>
#include <vector>

class PerlProject {
public:
    PerlProject(const std::string &rootDirectory, const std::string &perlPath);

    // Root directory of perl project
    std::string rootDirectory;

    // Perl interpreter path
    std::string perlPath;

    // Include paths for this installation
    std::vector<std::string> includePaths;
};


#endif //PERLPARSER_PERLPROJECT_H
