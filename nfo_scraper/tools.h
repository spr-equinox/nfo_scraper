#pragma once
#include <string>
#include <vector>
#include <filesystem>
#include <QMainWindow>

class config;

using fs_path = std::filesystem::path;
using vec_paths = std::vector<fs_path>;
using shows_directories = std::map<std::pair<bool, int>, std::pair<fs_path, std::wstring>>;
using seasons_directories = std::map<std::tuple<bool, int, int>, std::pair<fs_path, std::wstring>>;
using library_directories = std::unordered_map<fs_path, vec_paths>;

std::string request(const char* url, config* cfg);
bool create_directory_with_log(const fs_path& path);
std::wstring utf8_to_wchar(const char* str, int len);
void replace_illegal_char(std::wstring& str);

class window_init_with_data : public QMainWindow {
public:
    explicit window_init_with_data(QWidget* parent = nullptr, Qt::WindowFlags flags = Qt::WindowFlags())
        : QMainWindow(parent, flags){};
    virtual void init(void* pointer) = 0;
};