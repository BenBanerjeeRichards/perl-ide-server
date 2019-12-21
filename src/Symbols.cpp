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
