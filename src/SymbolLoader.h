//
// Created by Ben Banerjee-Richards on 27/12/2019.
//

#ifndef PERLPARSE_SYMBOLLOADER_H
#define PERLPARSE_SYMBOLLOADER_H

#include <string>
#include <unordered_map>
#include <queue>
#include <chrono>
#include <set>
#include "Util.h"
#include "FileAnalysis.h"
#include "PerlCommandLine.h"
#include "Symbols.h"
#include "Cache.h"

FileSymbolMap loadAllFileSymbols(std::string path, std::string contextPath, Cache &cache);

GlobalVariablesMap buildGlobalVariablesMap(const FileSymbolMap &fileSymbolsMap);

std::optional<Symbols> buildSymbols(std::string rootPath, std::string contextPath);

std::optional<Symbols> buildSymbols(std::string rootPath, std::string contextPath, Cache &cache);

std::unordered_map<std::string, PathNode>
loadProjectGraph(const std::vector<std::string> &projectFiles, std::vector<std::string> includes, Cache &cache);

std::string projGraphToDot(const std::unordered_map<std::string, PathNode> &graph, bool showParents = false);

std::set<std::string> pathsConnectedTo(std::string path, std::unordered_map<std::string, PathNode> &importGraph);

std::set<std::string> relatedFiles(std::string path, std::unordered_map<std::string, PathNode> graph);

std::optional<Symbols>
buildProjectSymbols(const std::string &rootPath, std::string contextPath, std::vector<std::string> projectPaths,
                    Cache &cache);

#endif //PERLPARSE_SYMBOLLOADER_H
