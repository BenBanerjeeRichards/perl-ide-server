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

struct PackagedSymbol {
    std::string package;
    std::string symbol;
};

std::string findPackageAtPos(const std::vector<PackageSpan> &packages, FilePos pos);

std::vector<std::string> splitPackage(const std::string &package);

PackagedSymbol splitOnPackage(std::string canonicalSymbol, std::string packageContext);

std::string getCanonicalPackageName(const std::string &package);

#endif //PERLPARSE_PACKAGE_H
