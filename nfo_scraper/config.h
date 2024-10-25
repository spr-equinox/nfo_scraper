#pragma once
#include <atomic>
#include <regex>
#include <string>
#include <unordered_set>
#include <vector>
#include <filesystem>

#include "tools.h"
class config {
public:
    bool check_ext(const std::string &ext);
    bool check_ignore(const std::string &name);
    std::string get_shows_name(const std::string &ext);
    std::string get_key();
    char get_save_type();
    fs_path get_save_path();
    bool is_proxy();
    std::string proxy();
    config(const char *file);
private:
    std::string key, proxy_addr;
    fs_path save_path;
    std::unordered_set<std::string> media_exts;
    std::vector<std::pair<std::regex, unsigned int>> reg_str;
    std::vector<std::regex> ignore_reg;
    bool use_proxy;
    char save_type;
};
