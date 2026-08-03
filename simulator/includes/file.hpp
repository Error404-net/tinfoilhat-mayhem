#pragma once
#include <cstdio>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>
#include <algorithm>

class File {
public:
    using Error = int;

    std::optional<Error> create(const std::filesystem::path& p) {
        file_ = fopen(p.string().c_str(), "w");
        return file_ ? std::optional<Error>{} : std::optional<Error>{-1};
    }

    void write_line(const std::string& line) {
        if (file_) { fputs(line.c_str(), file_); fputc('\n', file_); }
    }

    static std::optional<std::string> read_file(const std::filesystem::path& p) {
        FILE* f = fopen(p.string().c_str(), "r");
        if (!f) return {};
        fseek(f, 0, SEEK_END);
        long sz = ftell(f);
        rewind(f);
        std::string buf(sz, '\0');
        fread(buf.data(), 1, (size_t)sz, f);
        fclose(f);
        return buf;
    }

    ~File() { if (file_) fclose(file_); }

private:
    FILE* file_{nullptr};
};

inline void ensure_directory(const std::filesystem::path& p) {
    std::filesystem::create_directories(p);
}

// Returns the next path matching pattern like "TESTS/TH_????.csv".
// ponytail: hardcoded for TH_NNNN.csv pattern — generalise if needed.
inline std::filesystem::path next_filename_matching_pattern(const std::filesystem::path& pattern) {
    std::string dir = pattern.parent_path().string();
    std::string stem_prefix = "TH_";
    std::string ext = pattern.extension().string();
    if (dir.empty()) dir = ".";
    std::filesystem::create_directories(dir);
    for (int i = 1; i <= 9999; ++i) {
        char name[32];
        snprintf(name, sizeof(name), "%s%04d%s", stem_prefix.c_str(), i, ext.c_str());
        std::filesystem::path candidate = std::filesystem::path(dir) / name;
        if (!std::filesystem::exists(candidate)) return candidate;
    }
    return {};
}

// Lists filenames (not full paths) in dir matching a glob like "*.csv".
// ponytail: only checks extension, not full glob.
inline std::vector<std::filesystem::path> scan_root_files(
    const std::filesystem::path& dir, const std::filesystem::path& pattern)
{
    std::vector<std::filesystem::path> results;
    if (!std::filesystem::exists(dir)) return results;
    std::string ext = std::filesystem::path(pattern).extension().string();
    for (const auto& entry : std::filesystem::directory_iterator(dir)) {
        if (entry.is_regular_file() && entry.path().extension() == ext)
            results.push_back(entry.path().filename());
    }
    std::sort(results.begin(), results.end());
    return results;
}
