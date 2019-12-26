//
// Created by Ben Banerjee-Richards on 26/12/2019.
//

#include "Package.h"

std::string findPackageAtPos(const std::vector<PackageSpan> &packages, FilePos pos) {
    for (const PackageSpan &package : packages) {
        if (insideRange(package.start, package.end, pos)) return package.packageName;
    }

    return "main";
}

PackageSpan::PackageSpan(FilePos start, FilePos end, const std::string &name) {
    this->start = start;
    this->end = end;
    this->packageName = name;
}
