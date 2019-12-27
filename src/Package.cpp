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

/**
 * Split a package name up
 *
 * So Main::Package = <Main, Package>
 *    Main::::Package'SubPackage = <Main, Package, SubPackage>
 *
 * @param package A package name found in the code
 * @return
 */
std::vector<std::string> splitPackage(const std::string &package) {
    std::vector<std::string> parts;
    std::string part;

    for (auto c : package) {
        if (c == ':' || c == '\'') {
            if (!part.empty()) {
                parts.emplace_back(part);
                part = "";
            }
        } else {
            part += c;
        }
    }

    if (!part.empty()) parts.emplace_back(part);
    return parts;
}