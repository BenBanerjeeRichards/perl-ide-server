//
// Created by Ben Banerjee-Richards on 31/01/2020.
//

#include "Serialize.h"

json toJson(FilePos &filePos) {
    json result;
    result["line"] = filePos.line;
    result["col"] = filePos.col;
    result["position"] = filePos.position;
    return result;
}

FilePos filePosFromJson(json j) {

    return FilePos(j["line"], j["col"], j["position"]);
}

json toJson(std::shared_ptr<Variable> variable) {
    json r;

    // First add base information
    r["name"] = variable->name;
    r["declaration"] = toJson(variable->declaration);
    r["symbolEnd"] = toJson(variable->symbolEnd);
    r["id"] = variable->id;

    // Now add instance specific stuff
    if (std::shared_ptr<OurVariable> ourVariable = std::dynamic_pointer_cast<OurVariable>(variable)) {
        r["package"] = ourVariable->package;
        r["scopeEnd"] = toJson(ourVariable->scopeEnd);
        r["type"] = "our";
    } else if (std::shared_ptr<LocalVariable> localVariable = std::dynamic_pointer_cast<LocalVariable>(variable)) {
        r["scopeEnd"] = toJson(localVariable->scopeEnd);
        r["type"] = "local";
    } else if (std::shared_ptr<ScopedVariable> scopedVariable = std::dynamic_pointer_cast<ScopedVariable>(variable)) {
        r["scopeEnd"] = toJson(scopedVariable->scopeEnd);
        r["type"] = "scoped";
    } else {
        cerr << "Serialize error - Unknown variable type" << endl;
    }

    return r;
}

std::shared_ptr<Variable> variableFromJson(json j) {
    if (j["type"] == "our") {
        return std::make_shared<OurVariable>(j["id"], j["name"], filePosFromJson(j["declaration"]),
                                             filePosFromJson(j["symbolEnd"]), filePosFromJson(j["scopeEnd"]),
                                             j["package"]);
    } else if (j["type"] == "local") {
        return std::make_shared<LocalVariable>(j["id"], j["name"], filePosFromJson(j["declaration"]),
                                               filePosFromJson(j["symbolEnd"]), filePosFromJson(j["scopeEnd"]));
    } else if (j["type"] == "scoped") {
        return std::make_shared<ScopedVariable>(j["id"], j["name"], filePosFromJson(j["declaration"]),
                                                filePosFromJson(j["symbolEnd"]), filePosFromJson(j["scopeEnd"]));
    } else {
        cerr << "Serialization variableFromJson - Unknown variable type " << j["type"] << endl;
    }

    return nullptr;
}

void doSymboLNodeToJson(SymbolNode &parentNode, json &parentJson) {
    json nodeJson;
    auto varJsons = std::vector<json>();

    for (const auto &var : parentNode.variables) {
        varJsons.emplace_back(toJson(var));
    }

    nodeJson["variables"] = varJsons;
    nodeJson["features"] = parentNode.features;
    nodeJson["startPos"] = toJson(parentNode.startPos);
    nodeJson["endPos"] = toJson(parentNode.endPos);
    nodeJson["children"] = std::vector<json>();

    for (const auto &child : parentNode.children) {
        doSymboLNodeToJson(*child, nodeJson);
    }

    parentJson["children"].emplace_back(nodeJson);
}

json toJson(SymbolNode &rootSymbolNode) {
    json j;
    j["children"] = std::vector<json>();
    doSymboLNodeToJson(rootSymbolNode, j);
    if (j["children"].empty()) {
        json empty;
        return empty;
    } else {
        return j["children"][0];
    }
}

void doSymbolNodeFromJson(json j, const std::shared_ptr<SymbolNode> &parentSymbolNode) {
    auto childNode = std::make_shared<SymbolNode>(filePosFromJson(j["startPos"]), filePosFromJson(j["endPos"]), nullptr,
                                                  j["features"]);
    std::vector<std::shared_ptr<Variable>> variables;
    for (const auto &var : j["variables"]) {
        variables.emplace_back(variableFromJson(var));
    }
    childNode->variables = variables;
    childNode->children = std::vector<std::shared_ptr<SymbolNode>>();

    for (auto child : j["children"]) {
        doSymbolNodeFromJson(child, childNode);
    }

    parentSymbolNode->children.emplace_back(childNode);
}

std::shared_ptr<SymbolNode> symbolNodeFromJson(const json &j) {
    auto parent = std::make_shared<SymbolNode>(FilePos(0, 0, 0), FilePos(0, 0, 0), nullptr, std::vector<std::string>());
    doSymbolNodeFromJson(j, parent);
    if (parent->children.size() > 0) {
        return parent->children[0];
    }

    return nullptr;
}

