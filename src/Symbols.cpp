//
// Created by Ben Banerjee-Richards on 21/12/2019.
//

#include "Symbols.h"

SymbolNode::SymbolNode(const FilePos &startPos, const FilePos &endPos, std::shared_ptr<BlockNode> blockNode) :
        startPos(startPos), endPos(endPos), blockNode(blockNode) {}

Import::Import(const FilePos &location, ImportType type, ImportMechanism mechanism, const std::string &data,
               const std::vector<std::string> &exports) : location(location), type(type), mechanism(mechanism),

                                                          data(data), exports(exports) {}

std::string Import::toStr() {
    std::string str = "[" + this->location.toStr() + "] ";
    if (this->mechanism == ImportMechanism::Require) str += "require ";
    if (this->mechanism == ImportMechanism::Use) str += "use ";

    std::string delim;
    if (this->type == ImportType::Path) delim = "\"";
    str += delim + this->data + delim + " [";

    for (int i = 0; i < this->exports.size(); i++) {
        str += this->exports[i];
        if (i != exports.size() - 1) str += ", ";
    }

    str += "]";
    return str;
}

void GlobalVariablesMap::addGlobal(GlobalVariable global, std::string path, std::vector<GlobalVariable> usages) {
    if (this->globalsMap.count(global) == 0) {
        this->globalsMap[global] = std::unordered_map<std::string, std::vector<GlobalVariable>>();
    }

    if (this->globalsMap[global].count(path) == 0) {
        this->globalsMap[global][path] = std::vector<GlobalVariable>();
    }

    this->globalsMap[global][path].insert(this->globalsMap[global][path].end(), usages.begin(), usages.end());
}

std::string GlobalVariablesMap::toStr() {
    std::string str;

    for (const auto &globalMap : this->globalsMap) {
        str += globalMap.first.getFullName() + "\n";

        for (const auto &pathMap : globalMap.second) {
            str += "\t" + pathMap.first + ": ";

            for (auto usage : pathMap.second) {
                str += "(" + usage.toStr() + ") ";
            }
            str += "\n";
        }
    }

    return str;
}

void
SubroutineMap::addSubUsage(SubroutineDecl declaration, std::string code, std::string &usagePath, Range &usageRange) {
    if (this->subsMap.count(declaration) == 0) {
        this->subsMap[declaration] = std::unordered_map<std::string, std::vector<SubroutineCode>>();
    }

    if (this->subsMap[declaration].count(usagePath) == 0) {
        this->subsMap[declaration][usagePath] = std::vector<SubroutineCode>();
    }

    this->subsMap[declaration][usagePath].emplace_back(SubroutineCode(usageRange, code));
}

void
SubroutineMap::addSubUsages(SubroutineDecl declaration, std::vector<SubroutineCode> &usages, std::string &usagePath) {
    if (this->subsMap.count(declaration) == 0) {
        this->subsMap[declaration] = std::unordered_map<std::string, std::vector<SubroutineCode>>();
    }

    if (this->subsMap[declaration].count(usagePath) == 0) {
        this->subsMap[declaration][usagePath] = std::vector<SubroutineCode>();
    }

    this->subsMap[declaration][usagePath].insert(this->subsMap[declaration][usagePath].end(), usages.begin(),
                                                 usages.end());

}

std::string SubroutineMap::toStr() {
    std::string str;

    for (const auto &subMapItem : this->subsMap) {
        str += subMapItem.first.subroutine.getFullName() + "\n";

        for (const auto &pathMap : subMapItem.second) {
            str += "\t" + pathMap.first + ": ";

            for (auto usage : pathMap.second) {
                str += "(" + usage.toStr() + ") ";
            }
            str += "\n";
        }
    }

    return str;
}

std::string Constant::getFullName() {
    return this->package + "::" + this->name;
}

Constant::Constant(const std::string &package, const std::string &name, const FilePos &loocation) : package(package),
                                                                                                    name(name),
                                                                                                    location(
                                                                                                            loocation) {}

std::string Constant::toStr() {
    return "[" + this->location.toStr() + "] " + this->getFullName();
}

SubroutineUsage::SubroutineUsage(const std::string &package, const std::string &name, std::string code,
                                 const Range &pos) : package(
        package), name(name), code(code), pos(pos) {}

std::string SubroutineUsage::toStr() {
    return this->pos.toStr() + " " + this->package + "::" + this->name;
}

SubroutineCode::SubroutineCode(const Range &location, const std::string &code) : location(location), code(code) {}
