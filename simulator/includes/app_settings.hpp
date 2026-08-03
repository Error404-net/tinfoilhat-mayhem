#pragma once
#include <cstdint>
#include <cstdio>
// Make "key"sv literals available in any file that includes this header,
// matching what Mayhem's real app_settings.hpp provides.
#include <string_view>
using namespace std::string_view_literals;
#include <cstring>
#include <initializer_list>
#include <string>
#include <string_view>
#include <vector>

namespace app_settings {

enum class Mode { RX, TX };

struct Setting {
    std::string_view key;
    int32_t* value;
};

class SettingsManager {
public:
    SettingsManager(std::string_view name, Mode, std::initializer_list<Setting> entries)
        : name_(name), entries_(entries) {
        load();
    }
    ~SettingsManager() { save(); }

private:
    std::string name_;
    std::vector<Setting> entries_;

    std::string path() const { return std::string(name_) + ".ini"; }

    void load() {
        FILE* f = fopen(path().c_str(), "r");
        if (!f) return;
        char line[64];
        while (fgets(line, sizeof(line), f)) {
            char* eq = strchr(line, '=');
            if (!eq) continue;
            *eq = '\0';
            std::string_view key{line};
            int32_t val = (int32_t)atoi(eq + 1);
            for (auto& e : entries_)
                if (key == e.key) *e.value = val;
        }
        fclose(f);
    }

    void save() {
        FILE* f = fopen(path().c_str(), "w");
        if (!f) return;
        for (auto& e : entries_)
            fprintf(f, "%.*s=%d\n", (int)e.key.size(), e.key.data(), *e.value);
        fclose(f);
    }
};

}  // namespace app_settings
