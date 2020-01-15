//
// Created by Ben Banerjee-Richards on 21/12/2019.
//

#include "Symbols.h"

SymbolNode::SymbolNode(const FilePos &startPos, const FilePos &endPos, std::shared_ptr<BlockNode> blockNode) :
        startPos(startPos), endPos(endPos), blockNode(blockNode) {}

SymbolNode::SymbolNode(const FilePos &startPos, const FilePos &endPos, std::shared_ptr<BlockNode> blockNode,
                       std::vector<std::string> parentFeatures)
        : startPos(startPos), endPos(endPos), blockNode(blockNode), features(parentFeatures) {
}

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

void GlobalVariablesMap::addGlobal(GlobalVariable global, std::string path, std::vector<Range> usages) {
    if (this->globalsMap.count(global) == 0) {
        this->globalsMap[global] = std::unordered_map<std::string, std::vector<Range>>();
    }

    if (this->globalsMap[global].count(path) == 0) {
        this->globalsMap[global][path] = std::vector<Range>();
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

void SubroutineMap::addSub(SubroutineDecl sub, std::string path, std::vector<Range> usages) {
//    if (this->subsMap.count(sub) == 0) {
//        this->subsMap[sub] = std::unordered_map<std::string, std::vector<Range>>();
//    }
//
//    if (this->subsMap[sub].count(path) == 0) {
//        this->subsMap[sub][path] = std::vector<Range>();
//    }
//
//    this->subsMap[sub][path].insert(this->subsMap[sub][path].end(), usages.begin(), usages.end());
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
