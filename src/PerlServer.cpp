//
// Created by Ben Banerjee-Richards on 2019-11-24.
//

#include "PerlServer.h"

#include <utility>
#include <chrono>
#include <thread>

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

void startAndBlock(int port) {
    httplib::Server httpServer;

    httpServer.Post("/", [](const httplib::Request &req, httplib::Response &res) {
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

        if (reqJson["method"] == "autocomplete-var") {
            if (!reqJson.contains("params") || !reqJson["params"].contains("path") ||
                !reqJson["params"].contains("sigil")) {
                sendJson(res, "BAD_PARAMS", "Bad parameters");
                return;
            }

            json params = reqJson["params"];
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
        } else if (reqJson["method"] == "autocomplete-sub") {
            json params = reqJson["params"];
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
        } else if (reqJson["method"] == "find-usages") {
            try {
                auto params = reqJson["params"];
                std::string path = params["path"];
                std::string context = params["context"];
                int line = params["line"];
                int col = params["col"];

                std::map<std::string, std::vector<std::vector<int>>> jsonFrom;
                for (auto usage : analysis::findVariableUsages(path, FilePos(line, col))) {
                    std::vector<std::vector<int>> fileLocations;
                    for (auto pos : usage.second) {
                        auto posList = std::vector<int>{pos.line, pos.col};
                        fileLocations.emplace_back(posList);
                    }

                    jsonFrom[usage.first] = fileLocations;
                }

                json response = jsonFrom;
                sendJson(res, response);
            } catch (json::exception &) {
                sendJson(res, "BAD_PARAMS", "Bad Params");
                return;
            }
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

