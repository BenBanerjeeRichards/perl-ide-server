//
// Created by Ben Banerjee-Richards on 2019-08-26.
//

//
// Created by Ben Banerjee-Richards on 2019-08-25.
//

#include <fstream>
#include "Util.h"
#include "IOException.h"

bool insideRange(FilePos start, FilePos end, FilePos pos) {
    // Note end is inclusive
    // TODO improve performance here
    bool correctLine = pos.line >= start.line && pos.line <= end.line;
    if (!correctLine) return false;

    if (pos.line == start.line) return pos.col >= start.col;
    if (pos.line == end.line)  return pos.col <= end.col;

    return true;
}

std::string replace(std::string str, const std::string &what, const std::string &with) {
    std::string baseString = str;
    while (baseString.find(what) != std::string::npos) {
        baseString.replace(baseString.find(what), what.size(), with);
    }

    return baseString;
}

// Based on https://stackoverflow.com/a/22489298
int numOccurrences(const std::string &str, const std::string& sub) {
    int occurrences = 0;
    std::string::size_type pos = 0;
    while ((pos = str.find(sub, pos)) != std::string::npos) {
        ++occurrences;
        pos += sub.length();
    }
    return occurrences;
}

std::vector<std::string> globglob(const std::string& pattern) {
    glob_t glob_result;
    std::vector<std::string> result;
    std::vector<char> chars(pattern.c_str(), pattern.c_str() + pattern.size() + 1u);
    glob(&chars[0], GLOB_TILDE, nullptr, &glob_result);
    for (unsigned int i = 0; i < (int)glob_result.gl_pathc; ++i) {
        result.emplace_back(glob_result.gl_pathv[i]);
    }

    return result;
}

// https://stackoverflow.com/questions/8520560/get-a-file-name-from-a-path
std::string fileName(const std::string& path) {
    std::string filename = path;
    const size_t last_slash_idx = filename.find_last_of("\\/");
    if (std::string::npos != last_slash_idx)
    {
        filename.erase(0, last_slash_idx + 1);
    }

    // Remove extension if present.
    const size_t period_idx = filename.rfind('.');
    if (std::string::npos != period_idx)
    {
        filename.erase(period_idx);
    }

    return filename;
}

std::string readFile(const std::string& path) {
    std::ifstream fileStream(path);
    if (!fileStream.is_open()) {
        throw IOException("Failed to open file " + path);
    }

    std::string contents((std::istreambuf_iterator<char>(fileStream)), (std::istreambuf_iterator<char>()));
    return contents;
}
