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

PackagedSymbol splitOnPackage(std::string canonicalSymbol, std::string packageContext) {
    std::vector<std::string> parts = split(canonicalSymbol, "::");
    PackagedSymbol packagedSymbol;

    // Remove actual name
    packagedSymbol.symbol = parts[parts.size() - 1];
    parts.pop_back();

    // Join again for package name
    std::string packageNameFromSymbol = parts.empty() ? "" : join(parts, "::");
    if (packageNameFromSymbol.empty()) {
        packagedSymbol.package = packageContext;
    } else {
        packagedSymbol.package = packageNameFromSymbol;
    }

    return packagedSymbol;
}

std::string getCanonicalPackageName(const std::string &package) {
    std::string canonical = package;

    // Now replace ' with ::, -> with ::
    for (int i = 0; i < canonical.size(); i++) {
        if (canonical[i] == '\'') {
            canonical = canonical.substr(0, i) + "::" + canonical.substr(i + 1, canonical.size() - i);
        } else if (canonical[i] == '-' && i < canonical.size() - 2 && canonical[i + 1] == '>') {
            canonical[i] = ':';
            canonical[i + 1] = ':';
        }
    }

    // Now replace :::: with ::
    for (int i = 0; i < (int) canonical.size(); i++) {
        if (canonical[i] == ':') {
            int j = 1;
            while (j < (int) canonical.size()) {
                if (canonical[i + j] != ':') break;
                j++;
            }

            int numDoubleColons = (int) j / 2;
            if (numDoubleColons > 1) {
                int deleteFrom = i + 2;
                int deleteTo = i + numDoubleColons * 2;
                canonical = canonical.substr(0, deleteFrom) + canonical.substr(deleteTo, canonical.size() - deleteTo);
            }

            i += 2;
        }
    }

    return canonical;
}

