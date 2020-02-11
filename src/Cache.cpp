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
    auto fileContentsStr = readFile(path);
    auto fileContents = fileContentsStr.c_str();
    if (strlen(fileContents) == 0) {
        return "<EMPTY>";
    };
    md5_append(&state, (const md5_byte_t *) fileContents, strlen(fileContents));
    md5_byte_t digest[16];
    md5_finish(&state, digest);
    char hex_output[16 * 2 + 1];

    for (int di = 0; di < 16; ++di)
        sprintf(hex_output + di * 2, "%02x", digest[di]);

    return hex_output;
}

void Cache::addItem(std::string path, std::shared_ptr<FileSymbols> fileSymbols) {
    this->evictItems();
    CacheItem cacheItem(isSystemPath(path), generateMd5Sum(path), fileSymbols);
    this->cache[path] = cacheItem;
}

std::optional<std::shared_ptr<FileSymbols>> Cache::getItem(std::string path) {
    if (this->cache.count(path) == 0) {
        return {};
    }

    auto cacheItem = this->cache[path];
    if (!cacheItem.isSystemPath && generateMd5Sum(path) != cacheItem.md5) {
        this->cache.erase(path);
        return {};
    }

    cacheItem.markJustUsed();
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

void Cache::evictItems() {
    // Evict 10% of the cache size ONLY IF it is full
    int overflow = (int) this->cache.size() - constant::CACHE_MAX_ITEMS;
    if (overflow < 0) return;
    int toEvict = (int) (constant::CACHE_MAX_ITEMS * 0.1) + overflow + 1;

    // Sort map
    // This is a pain, but remember that CACHE_MAX_ITEMS is quite small and each CacheItem takes up very little space
    std::vector<std::pair<std::string, CacheItem>> mapEntries;
    mapEntries.reserve(mapEntries.size());
    for (const auto &entry : this->cache) mapEntries.emplace_back(entry);
    std::sort(mapEntries.begin(), mapEntries.end(), [](auto a, auto b) { return a > b; });
    toEvict = mapEntries.size() < toEvict ? (int) mapEntries.size() : toEvict;

    for (int i = 0; i < toEvict; i++) {
        std::cout << "Evicting item " << mapEntries[i].first << " inserted at " << mapEntries[i].second.lastUsed
                  << std::endl;
        this->cache.erase(mapEntries[i].first);
    }
}

void CacheItem::markJustUsed() {
    this->lastUsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
}


CacheItem::CacheItem(bool isSystemPath, std::string md5, std::shared_ptr<FileSymbols> fileSymbols) {
    this->isSystemPath = isSystemPath;
    this->md5 = md5;
    this->fileSymbols = fileSymbols;
    this->markJustUsed();
}

CacheItem::CacheItem() {
    this->isSystemPath = false;
    this->md5 = "";
    this->fileSymbols = nullptr;
    this->markJustUsed();
}
