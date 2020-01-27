//
// Created by Ben Banerjee-Richards on 2019-08-25.
//

#include "Util.h"

bool insideRange(FilePos start, FilePos end, FilePos pos) {
    // Note end is inclusive
    // TODO improve performance here
    bool correctLine = pos.line >= start.line && pos.line <= end.line;
    if (!correctLine) return false;

    if (pos.line == start.line) return pos.col >= start.col;
    if (pos.line == end.line) return pos.col <= end.col;

    return true;
}

bool insideRange(Range range, FilePos pos) {
    return insideRange(range.from, range.to, pos);
}

std::string replace(std::string str, const std::string &what, const std::string &with) {
    std::string baseString = str;
    while (baseString.find(what) != std::string::npos) {
        baseString.replace(baseString.find(what), what.size(), with);
    }

    return baseString;
}

std::vector<std::string> globglob(const std::string &pattern) {
    glob_t glob_result;
    std::vector<std::string> result;
    std::vector<char> chars(pattern.c_str(), pattern.c_str() + pattern.size() + 1u);
    glob(&chars[0], GLOB_TILDE, nullptr, &glob_result);
    for (unsigned int i = 0; i < (int) glob_result.gl_pathc; ++i) {
        result.emplace_back(glob_result.gl_pathv[i]);
    }

    return result;
}

// https://stackoverflow.com/questions/8520560/get-a-file-name-from-a-path
std::string fileName(const std::string &path) {
    std::string filename = path;
    const size_t last_slash_idx = filename.find_last_of("\\/");
    if (std::string::npos != last_slash_idx) {
        filename.erase(0, last_slash_idx + 1);
    }

    // Remove extension if present.
    const size_t period_idx = filename.rfind('.');
    if (std::string::npos != period_idx) {
        filename.erase(period_idx);
    }

    return filename;
}

std::string directoryOf(std::string path) {
    // Not the best solution but it works
    auto pathParts = split(path, "/");
    pathParts.pop_back();   // Remove file name
    auto joined = join(pathParts, "/");
    if (path[0] == '/') joined = "/" + joined;
    return joined;
}

std::string readFile(const std::string &path) {
    std::ifstream fileStream(path);
    if (!fileStream.is_open()) {
        throw IOException("Failed to open file " + path);
    }

    std::string contents((std::istreambuf_iterator<char>(fileStream)), (std::istreambuf_iterator<char>()));
    return contents;
}

// https://stackoverflow.com/a/18427254
std::string join(const std::vector<std::string> &vec, const char *delim) {
    if (vec.empty()) return "";

    std::stringstream res;
    copy(vec.begin(), vec.end(), std::ostream_iterator<std::string>(res, delim));
    std::string resStr = res.str();
    // Remove ending deliminator
    return resStr.substr(0, resStr.size() - strlen(delim));
}

bool endsWith(const std::string &s, const std::string &suffix) {
    return s.size() >= suffix.size() &&
           s.substr(s.size() - suffix.size()) == suffix;
}

// https://stackoverflow.com/questions/14265581/parse-split-a-string-in-c-using-string-delimiter-standard-c
std::vector<std::string> split(std::string s, const std::string &delimiter) {
    std::vector<std::string> tokens;

    for (size_t start = 0, end; start < s.length(); start = end + delimiter.length()) {
        size_t position = s.find(delimiter, start);
        end = position != std::string::npos ? position : s.length();

        std::string token = s.substr(start, end - start);
        if (!token.empty()) {
            tokens.push_back(token);
        }
    }

    if ((s.empty() || endsWith(s, delimiter))) {
        tokens.emplace_back("");
    }

    return tokens;
}

/**
 * Convert ASCII string to lowercase
 * @param str
 * @return
 */
std::string toLower(const std::string &str) {
    // Don't mutate input
    std::string lowercase = str;
    std::transform(lowercase.begin(), lowercase.end(), lowercase.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    return lowercase;
}