//
// Created by Ben Banerjee-Richards on 2019-11-24.
//

#include "PerlServer.h"

#include <utility>

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

bool hasParams(const httplib::Request &req, httplib::Response &res, const std::vector<std::string> &requiredParams) {
    for (const auto &requiredParam: requiredParams) {
        if (req.params.count(requiredParam) == 0) {
            json empty;
            sendJson(res, empty, "MISSING_PARAM", "Required parameter `" + requiredParam + "` not found");
            return false;
        }
    }

    return true;
}

bool getIntParam(const httplib::Request &req, const std::string &name, int &value) {
    try {
        value = std::stoi(req.params.find(name)->second);
    } catch (std::invalid_argument &) {
        return false;
    }

    return true;
}

void startAndBlock(int port) {
    httplib::Server httpServer;

    httpServer.Get("/autocomplete", [](const httplib::Request &req, httplib::Response &res) {
        if (hasParams(req, res, std::vector<std::string>{"path", "line", "col"})) {
            int line = -1;
            int col = -1;
            std::string path = req.params.find("path")->second;
            if (!getIntParam(req, "line", line)) {
                sendJson(res, "BAD_PARAM_TYPE", "Expected param line to be an int");
                goto end;
            }
            if (!getIntParam(req, "col", col)) {
                sendJson(res, "BAD_PARAM_TYPE", "Expected param line to be an int");
                goto end;
            }
            auto completeItems = autocomplete(path, FilePos(line, col));

            json response;
            std::vector<std::vector<std::string>> jsonFrom;
            for (const AutocompleteItem &completeItem : completeItems) {
                std::vector<std::string> itemForm{completeItem.name, completeItem.detail};
                jsonFrom.emplace_back(itemForm);
            }
            response = jsonFrom;
            sendJson(res, response);
        }

        end:
        int i;
    });

    httpServer.listen("localhost", port);
}

