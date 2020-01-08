//
// Created by Ben Banerjee-Richards on 2019-11-24.
//

#include "PerlServer.h"


void sendJson(httplib::Response &res, json &jsonObject, std::string error = "", std::string errorMessage = "") {
    json response;
    response["body"] = jsonObject;
    if (error.empty()) {
        response["success"] = true;
    } else {
        response["success"] = false;
        response["error"] = error;
        response["errorMessage"] = errorMessage;
        res.status = 400;
    }

    res.set_content(response.dump(), "application/json");
}

void sendJson(httplib::Response &res, std::string error = "", std::string errorMessage = "") {
    json null;
    sendJson(res, null, std::move(error), std::move(errorMessage));
}

void handleAutocompleteVariable(httplib::Response &res, json params) {
    if (!params.contains("path") || !params.contains("sigil")) {
        sendJson(res, "BAD_PARAMS", "Bad parameters");
        return;
    }

    int line, col;
    try {
        line = params["line"];
        col = params["col"];
    } catch (json::exception &) {
        sendJson(res, "BAD_PARAMS", "Bad Params");
        return;
    }

    std::vector<AutocompleteItem> completeItems;
    try {
        completeItems = analysis::autocompleteVariables(params["path"], FilePos(line, col),
                                                        std::string(params["sigil"])[0]);
    } catch (IOException &) {
        sendJson(res, "PATH_NOT_FOUND", "File " + std::string(params["path"]) + " not found");
        return;
    } catch (TokeniseException &ex) {
        sendJson(res, "PARSE_ERROR", "Error occured during tokenization " + ex.reason);
        return;
    }

    json response;
    std::vector<std::vector<std::string>> jsonFrom;
    for (const AutocompleteItem &completeItem : completeItems) {
        std::vector<std::string> itemForm{completeItem.name, completeItem.detail};
        jsonFrom.emplace_back(itemForm);
    }
    response = jsonFrom;
    sendJson(res, response);
}

void handleAutocompleteSubroutine(httplib::Response &res, json params) {
    std::string path = params["path"];
    int line, col;
    try {
        line = params["line"];
        col = params["col"];
    } catch (json::exception &) {
        sendJson(res, "BAD_PARAMS", "Bad Params 2");
        return;
    }

    std::vector<AutocompleteItem> completeItems;
    try {
        completeItems = analysis::autocompleteSubs(path, FilePos(line, col));
    } catch (IOException &) {
        sendJson(res, "PATH_NOT_FOUND", "File " + path + " not found");
        return;
    } catch (TokeniseException &ex) {
        sendJson(res, "PARSE_ERROR", "Error occurred during tokenization " + ex.reason);
        return;
    }

    json response;
    std::vector<std::vector<std::string>> jsonFrom;
    for (const AutocompleteItem &completeItem : completeItems) {
        std::vector<std::string> itemForm{completeItem.name, completeItem.detail};
        jsonFrom.emplace_back(itemForm);
    }
    response = jsonFrom;
    sendJson(res, response);
}

void handleFindUsages(httplib::Response &res, json params, Cache &cache) {
    try {
        std::string path = params["path"];
        std::string contextPath = params["context"];
        int line = params["line"];
        int col = params["col"];
        std::vector<std::string> projectFiles = params["projectFiles"];

        std::map<std::string, std::vector<std::vector<int>>> jsonFrom;
        for (const auto &fileWithUsages : analysis::findVariableUsages(path, contextPath, FilePos(line, col),
                                                                       projectFiles, cache)) {
            std::vector<std::vector<int>> fileLocations;
            for (const Range &usage : fileWithUsages.second) {
                fileLocations.emplace_back(std::vector<int>{usage.from.line, usage.from.col});
            }

            std::sort(fileLocations.begin(), fileLocations.end());
            if (!fileLocations.empty()) jsonFrom[fileWithUsages.first] = fileLocations;
        }

        json response = jsonFrom;
        sendJson(res, response);
    } catch (json::exception &) {
        sendJson(res, "BAD_PARAMS", "Bad Params");
        return;
    }

}


void handleFindDeclaration(httplib::Response &res, json params) {
    std::string path = params["path"];
    std::string context = params["context"];
    int line = params["line"];
    int col = params["col"];

    // TODO expand search to multiple files
    json response;
    auto maybeDecl = analysis::findVariableDeclaration(path, FilePos(line, col));
    if (!maybeDecl.has_value()) {
        response["exists"] = false;
    } else {
        response["exists"] = true;
        response["file"] = context;
        response["line"] = maybeDecl.value().line;
        response["col"] = maybeDecl.value().col;
    }

    sendJson(res, response);
}

void handleIndexProject(httplib::Response &res, json params, Cache &cache) {
    if (!params.contains("projectFiles")) {
        sendJson(res, "BAD_PARAMS", "No project files provided");
        return;
    }

    std::vector<std::string> projectFiles = params["projectFiles"];
    analysis::indexProject(projectFiles, cache);
    json response;
    sendJson(res, response);
}


void startAndBlock(int port) {
    httplib::Server httpServer;

    // Setup cache
    Cache cache;

    httpServer.Post("/", [&](const httplib::Request &req, httplib::Response &res) {
        json reqJson;
        try {
            reqJson = json::parse(req.body);
        } catch (json::exception &ex) {
            sendJson(res, "BAD_JSON", "Failed to parse json request");
            return;
        }

        if (!reqJson.contains("method")) {
            sendJson(res, "NO_METHOD", "No method provided");
            return;
        }
        if (!reqJson.contains("params")) {
            sendJson(res, "BAD_PARAMS", "No params provided");
            return;
        }

        json params = reqJson["params"];

        if (reqJson["method"] == "autocomplete-var") {
            handleAutocompleteVariable(res, params);
        } else if (reqJson["method"] == "autocomplete-sub") {
            handleAutocompleteSubroutine(res, params);
        } else if (reqJson["method"] == "find-usages") {
            handleFindUsages(res, params, cache);
        } else if (reqJson["method"] == "find-declaration") {
            handleFindDeclaration(res, params);
        } else if (reqJson["method"] == "index-project") {
            handleIndexProject(res, params, cache);
        } else {
            sendJson(res, "UNKNOWN_METHOD", "Method " + std::string(reqJson["method"]) + " not supported");
            return;
        }
    });

    httpServer.Get("/ping", [](const httplib::Request &req, httplib::Response &res) {
        res.set_content("ok", "text/plain");
    });


    httpServer.listen("localhost", port);
}


