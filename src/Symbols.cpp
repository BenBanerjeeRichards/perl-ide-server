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
