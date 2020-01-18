//
// Created by Ben Banerjee-Richards on 02/01/2020.
//

#include "Cache.h"

#include <utility>

bool isSystemPath(const std::string &filePath) {
    // FIXME path comparison pretty bad - for example what about  /Network//Library/...
    std::string path = toLower(filePath);   // Also bad assumption...
    if (path.find("/library/", 0) == 0) return true;
    if (path.find("/network/library/", 0) == 0) return true;
    if (path.find("/system/library/", 0) == 0) return true;
    if (path.find("/usr", 0) == 0) return true;
    return false;
}

std::string generateMd5Sum(const std::string &path) {
    md5_state_t state;
    md5_init(&state);
    // TODO what if path doesn't exist? Add error testing
    auto fileContents = readFile(path).c_str();
    if (strlen(fileContents) == 0) return "<EMPTY>";
    md5_append(&state, (const md5_byte_t *) fileContents, strlen(fileContents));
    md5_byte_t digest[16];
    md5_finish(&state, digest);
    char hex_output[16 * 2 + 1];

    for (int di = 0; di < 16; ++di)
        sprintf(hex_output + di * 2, "%02x", digest[di]);

    return hex_output;
}

void Cache::addItem(std::string path, std::shared_ptr<FileSymbols> fileSymbols) {
    CacheItem cacheItem(isSystemPath(path), generateMd5Sum(path), fileSymbols);
    this->cache[path] = cacheItem;
}

std::optional<std::shared_ptr<FileSymbols>> Cache::getItem(std::string path) {
    if (this->cache.count(path) == 0) {
        return std::optional<std::shared_ptr<FileSymbols>>();
    }

    auto cacheItem = this->cache[path];
    if (!cacheItem.isSystemPath && generateMd5Sum(path) != cacheItem.md5) {
        this->cache.erase(path);
        return std::optional<std::shared_ptr<FileSymbols>>();
    }

    return cacheItem.fileSymbols;
}

std::string Cache::toStr() {
    std::string str = "";
    for (auto pathCacheItem : this->cache) {
        str += "system=" + std::to_string(pathCacheItem.second.isSystemPath) + " ";
        str += "md5=" + pathCacheItem.second.md5 + " ";
        str += pathCacheItem.first;
        str += "\n";
    }

    return str;
}


CacheItem::CacheItem(bool isSystemPath, std::string md5, std::shared_ptr<FileSymbols> fileSymbols) {
    this->isSystemPath = isSystemPath;
    this->md5 = md5;
    this->fileSymbols = fileSymbols;
}

CacheItem::CacheItem() {
    this->isSystemPath = false;
    this->md5 = "";
    this->fileSymbols = nullptr;
}
