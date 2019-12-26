//
// Created by Ben Banerjee-Richards on 23/12/2019.
//

#ifndef PERLPARSE_PACKAGE_H
#define PERLPARSE_PACKAGE_H

#include "FilePos.h"
#include "Util.h"
#include  <vector>
#include <string>

struct PackageSpan {

    PackageSpan(FilePos start, FilePos end, const std::string &name);

    PackageSpan(const std::string &packageName) : packageName(packageName) {}

    FilePos start;
    FilePos end;
    std::string packageName;
};

std::string findPackageAtPos(const std::vector<PackageSpan> &packages, FilePos pos);

#endif //PERLPARSE_PACKAGE_H
