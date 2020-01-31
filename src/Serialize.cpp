//
// Created by Ben Banerjee-Richards on 31/01/2020.
//

#include "Serialize.h"

enum VariableType {
    Our = 1,
    Local = 2,
    Scoped = 3
};

json toJson(FilePos &filePos) {
    return std::vector<int>{filePos.line, filePos.col, filePos.position};
}

FilePos filePosFromJson(const json &j) {
    std::vector<int> jList = j;
    return FilePos(jList[0], jList[1], jList[2]);
}

json toJson(std::shared_ptr<Variable> variable) {
    std::vector<json> r;
    r.emplace_back(""); // Type, to be decided

    // Format is [type 0, id 1, name 2, declaration 3, symbolEnd 4, scopeEnd 5, package 6]

    // First add base information
    r.emplace_back(variable->id);
    r.emplace_back(variable->name);
    r.emplace_back(toJson(variable->declaration));
    r.emplace_back(toJson(variable->symbolEnd));

    // Now add instance specific stuff
    if (std::shared_ptr<OurVariable> ourVariable = std::dynamic_pointer_cast<OurVariable>(variable)) {
        r.emplace_back(toJson(ourVariable->scopeEnd));
        r.emplace_back(ourVariable->package);
        r[0] = Our;
    } else if (std::shared_ptr<LocalVariable> localVariable = std::dynamic_pointer_cast<LocalVariable>(variable)) {
        r.emplace_back(toJson(localVariable->scopeEnd));
        r[0] = Local;
    } else if (std::shared_ptr<ScopedVariable> scopedVariable = std::dynamic_pointer_cast<ScopedVariable>(variable)) {
        r.emplace_back(toJson(scopedVariable->scopeEnd));
        r[0] = Scoped;
    } else {
        cerr << "Serialize error - Unknown variable type" << endl;
    }

    return r;
}

std::shared_ptr<Variable> variableFromJson(json j) {
    int type = j[0];
    if (type == Our) {
        return std::make_shared<OurVariable>(j[1], j[2], filePosFromJson(j[3]),
                                             filePosFromJson(j[4]), filePosFromJson(j[5]),
                                             j[6]);
    } else if (type == Local) {
        return std::make_shared<LocalVariable>(j[1], j[2], filePosFromJson(j[3]),
                                               filePosFromJson(j[4]), filePosFromJson(j[5]));

    } else if (type == Scoped) {
        return std::make_shared<ScopedVariable>(j[1], j[2], filePosFromJson(j[3]),
                                                filePosFromJson(j[4]), filePosFromJson(j[5]));
    } else {
        cerr << "Serialization variableFromJson - Unknown variable type " << j["type"] << endl;
    }

    return nullptr;
}

void doSymboLNodeToJson(SymbolNode &parentNode, json &parentJson) {
    std::vector<json> nodeData;
    auto varJsons = std::vector<json>();

    for (const auto &var : parentNode.variables) {
        varJsons.emplace_back(toJson(var));
    }

    // List format: [variables, startPos, endPos, children]
    nodeData.emplace_back(varJsons);
    nodeData.emplace_back(toJson(parentNode.startPos));
    nodeData.emplace_back(toJson(parentNode.endPos));

    nodeData.emplace_back(std::vector<json>());
    json nodeDataJson = nodeData;

    for (const auto &child : parentNode.children) {
        doSymboLNodeToJson(*child, nodeDataJson);
    }

    // 3 = children
    parentJson[3].emplace_back(nodeDataJson);
}

json toJson(SymbolNode &rootSymbolNode) {
    json j;
    std::vector<json>{};
    j.emplace_back(json());
    j.emplace_back(json());
    j.emplace_back(json());
    j.emplace_back(std::vector<json>());

    doSymboLNodeToJson(rootSymbolNode, j);
    if (j[3].empty()) {
        json empty;
        return empty;
    } else {
        return j[3][0];
    }
}

void doSymbolNodeFromJson(json j, const std::shared_ptr<SymbolNode> &parentSymbolNode) {
    auto childNode = std::make_shared<SymbolNode>(filePosFromJson(j[1]), filePosFromJson(j[2]), nullptr);
    std::vector<std::shared_ptr<Variable>> variables;
    for (const auto &var : j[0]) {
        variables.emplace_back(variableFromJson(var));
    }
    childNode->variables = variables;
    childNode->children = std::vector<std::shared_ptr<SymbolNode>>();

    for (auto child : j[3]) {
        doSymbolNodeFromJson(child, childNode);
    }

    parentSymbolNode->children.emplace_back(childNode);
}

std::shared_ptr<SymbolNode> symbolNodeFromJson(const json &j) {
    auto parent = std::make_shared<SymbolNode>(FilePos(0, 0, 0), FilePos(0, 0, 0), nullptr);
    doSymbolNodeFromJson(j, parent);
    if (parent->children.size() > 0) {
        return parent->children[0];
    }

    return nullptr;
}

