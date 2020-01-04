//
// Created by Ben Banerjee-Richards on 02/01/2020.
//

#ifndef PERLPARSE_CACHE_H
#define PERLPARSE_CACHE_H

#include <string>
#include <unordered_map>
#include "Util.h"
#include "Symbols.h"
#include "../lib/md5.h"

struct CacheItem {

    CacheItem(bool isSystemPath, std::string md5, std::shared_ptr<FileSymbols> fileSymbols);

    CacheItem();

    bool isSystemPath;
    std::string md5;
    std::shared_ptr<FileSymbols> fileSymbols;
};


class Cache {
    std::unordered_map<std::string, CacheItem> cache;

public:
    void addItem(std::string path, std::shared_ptr<FileSymbols> fileSymbols);

    std::optional<std::shared_ptr<FileSymbols>> getItem(std::string path);

    std::string toStr();

};

bool isSystemPath(const std::string &filePath);

std::string generateMd5Sum(const std::string &path);

#endif //PERLPARSE_CACHE_H
