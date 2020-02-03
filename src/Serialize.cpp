//
// Created by Ben Banerjee-Richards on 31/01/2020.
//

#include "Serialize.h"

enum VariableTypeCode {
    Our = 1,
    Local = 2,
    Scoped = 3
};


json toJson(const FilePos &filePos) {
    return std::vector<int>{filePos.line, filePos.col, filePos.position};
}


FilePos filePosFromJson(const json &j) {
    std::vector<int> jList = j;
    return FilePos(jList[0], jList[1], jList[2]);
}

json toJson(Range &range) {
    return std::vector<json>{toJson(range.from), toJson(range.to)};
}

Range rangeFromJson(const json &j) {
    return Range(filePosFromJson(j[0]), filePosFromJson(j[1]));
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

json toJson(PackageSpan &packageSpan) {
    return std::vector<json>{packageSpan.packageName, toJson(packageSpan.start), toJson(packageSpan.end)};
}

PackageSpan packageSpanFromJson(const json &j) {
    return PackageSpan(filePosFromJson(j[1]), filePosFromJson(j[2]), j[0]);
}

json toJson(Subroutine &sub) {
    return std::vector<json>{toJson(sub.location), sub.package, sub.name, sub.signature, sub.prototype};
}

Subroutine subFromJson(const json &j) {
    Subroutine sub;
    sub.location = rangeFromJson(j[0]);
    sub.package = j[1];
    sub.name = j[2];
    sub.signature = j[3];
    sub.prototype = j[4];
    return sub;
}

json toJson(SubroutineUsage &subUsage) {
    return std::vector<json>{subUsage.package, subUsage.name, toJson(subUsage.pos)};
}

SubroutineUsage subUsageFromJson(const json &j) {
    return SubroutineUsage(j[0], j[1], rangeFromJson(j[2]));
}


json toJson(Constant &subUsage) {
    return std::vector<json>{subUsage.package, subUsage.name, toJson(subUsage.location)};
}

Constant constantFromJson(const json &j) {
    return Constant(j[0], j[1], filePosFromJson(j[2]));
}

json toJson(Import &import) {
    return std::vector<json>{toJson(import.location), (int) import.type, (int) import.mechanism, import.data,
                             import.exports};
}

Import importFromJson(const json &j) {
    return Import(filePosFromJson(j[0]), (ImportType) j[1], (ImportMechanism) j[2], j[3], j[4]);
}

json toJson(const GlobalVariable &globalVar) {
    return std::vector<json>{globalVar.getPackage(), globalVar.getName(), globalVar.getSigil(), globalVar.getCodeName(),
                             toJson(globalVar.getFilePos())};
}


GlobalVariable globalVarFromJson(const json &j) {
    auto global = GlobalVariable(j[3], j[2], j[0], j[1]);
    global.setFilePos(filePosFromJson(j[4]));
    return global;
}

json toJson(FileSymbols &fileSymbols) {
    json j;
    j["symbolTree"] = toJson(*fileSymbols.symbolTree);
    j["packages"] = std::vector<json>();
    j["imports"] = std::vector<json>();
    j["constants"] = std::vector<json>();
    j["globals"] = std::vector<json>();
    j["subroutineDeclarations"] = std::vector<json>();
    j["possibleSubroutines"] = std::vector<json>();

    for (auto package : fileSymbols.packages) j["packages"].emplace_back(toJson(package));
    for (auto import : fileSymbols.imports) j["imports"].emplace_back(toJson(import));
    for (auto constant : fileSymbols.constants) j["constants"].emplace_back(toJson(constant));
    for (auto subUsage : fileSymbols.possibleSubroutineUsages) j["possibleSubroutines"].emplace_back(toJson(subUsage));

    // j[globals] has format [[<global>, <ranges...>, ...]]
    for (const auto &globalWithRanges : fileSymbols.globals) {
        json rangesVector = std::vector<json>();
        for (auto range : globalWithRanges.second) rangesVector.emplace_back(toJson(range));
        j["globals"].emplace_back(std::vector<json>{toJson(globalWithRanges.first), rangesVector});
    }

    for (const auto &subNameWithSub : fileSymbols.subroutineDeclarations) {
        j["subroutineDeclarations"].emplace_back(toJson(*subNameWithSub.second));
    }

    j["fileSubroutineUsages"] = std::unordered_map<std::string, std::vector<json>>();
    for (const auto &subUsageItem : fileSymbols.fileSubroutineUsages) {
        std::vector<json> rangeList;
        for (auto range : subUsageItem.second) rangeList.emplace_back(toJson(range));
        j["fileSubroutineUsages"][subUsageItem.first.getFullName()] = rangeList;
    }


    for (const auto &variableUsage : fileSymbols.variableUsages) {
        json rangesVector = std::vector<json>();
        for (auto range : variableUsage.second) rangesVector.emplace_back(toJson(range));
        j["variableUsages"].emplace_back(std::vector<json>{toJson(variableUsage.first), rangesVector});
    }

    return j;
}


