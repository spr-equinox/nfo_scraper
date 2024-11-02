#include <map>
#include <filesystem>
#include <vector>
#include <list>
#include <xutility>
#include <fstream>
#include <iostream>
#include <algorithm>

#include <Windows.h>
#include <atlbase.h>
#include <shobjidl.h>

#pragma warning(disable : 4996)

#define SPDLOG_WCHAR_TO_UTF8_SUPPORT
#include <spdlog/spdlog.h>

using fs_path = std::filesystem::path;

std::map <fs_path, std::vector<std::wstring>> subtitles;
std::list <std::pair<fs_path, fs_path>> strms, not_exist;
fs_path library;

std::array subtitle_extension = {
    L".ass", L".srt", L".sup", L".mks", L".mka"
};

std::wstring folder_open_dialog()
{
    std::wstring ret;
    CComPtr<IFileOpenDialog> spFileOpenDialog;
    if (SUCCEEDED(spFileOpenDialog.CoCreateInstance(__uuidof(FileOpenDialog)))) {
        FILEOPENDIALOGOPTIONS options;
        if (SUCCEEDED(spFileOpenDialog->GetOptions(&options))) {
            spFileOpenDialog->SetOptions(options | FOS_PICKFOLDERS);
            if (SUCCEEDED(spFileOpenDialog->Show(nullptr))) {
                CComPtr<IShellItem> spResult;
                if (SUCCEEDED(spFileOpenDialog->GetResult(&spResult))) {
                    wchar_t* name;
                    if (SUCCEEDED(spResult->GetDisplayName(SIGDN_FILESYSPATH, &name))) {
                        ret = name;
                        CoTaskMemFree(name);
                    }
                }
            }
        }
    }
    return std::move(ret);
}

std::wstring utf8_to_wchar(const char* str, int len) {
    int wcsLen = MultiByteToWideChar(CP_UTF8, NULL, str, len, NULL, 0);
    std::wstring ret;
    ret.resize(wcsLen);
    MultiByteToWideChar(CP_UTF8, NULL, str, len, ret.data(), wcsLen);
    return ret;
}

std::wstring local_to_wchar(const char* str, int len) {
    int wcsLen = MultiByteToWideChar(GetACP(), NULL, str, len, NULL, 0);
    std::wstring ret;
    ret.resize(wcsLen);
    MultiByteToWideChar(GetACP(), NULL, str, len, ret.data(), wcsLen);
    return ret;
}

bool check_extension(std::wstring ext) {
    for (auto& it : ext) it = towlower(it);
    for (auto it : subtitle_extension)
        if (it == ext) return true;
    return false;
}

int main() {
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
    SetConsoleCP(65001);
    SetConsoleOutputCP(65001);
    CoInitialize(nullptr);
#endif

    std::wstring folder = folder_open_dialog();
    spdlog::info(L"选择文件夹：{}", folder);
    library = std::move(folder);
    if (!std::filesystem::exists(library)) {
        spdlog::error("文件夹不存在");
        CoUninitialize();
        return 1;
    }

    for (auto& it : std::filesystem::recursive_directory_iterator(library)) {
        if (it.is_directory() || it.path().extension() != L".strm") continue;
        std::ifstream f(it.path());
        std::string str;
        str.resize(it.file_size());
        f.read(str.data(), str.size());
        if (f.bad()) {
            spdlog::error("读取文件 {} 时发生了 I/O 错误", it.path().generic_u8string());
            continue;
        }
        fs_path now = utf8_to_wchar(str.c_str(), str.size());
        (std::filesystem::exists(now) ? strms : not_exist).emplace_back(it.path(), std::move(now));
    }
    spdlog::info("找到 {} 个 strm 文件", not_exist.size() + strms.size());
    for (const auto& [dst, src] : not_exist) {
        std::cout << "文件：" << dst.generic_u8string() << "\n" << "链接点：" << src.generic_u8string() << "\n";
    }
    if (!not_exist.empty() && []() {
        while (true) {
            std::cout << "对应文件已不存在，是否删除 [yes/no]:";
            std::cout.flush();
            std::string input;
            std::cin >> input;
            if (input == "yes") return true;
            if (input == "no") return false;
            spdlog::warn("输入无法识别，重试");
        }
        }()) {
        for (const auto& [dst, src] : not_exist) {
            if (std::filesystem::remove(dst)) spdlog::info("删除文件 {} 成功", dst.generic_u8string());
            else spdlog::error("删除文件 {} 失败", dst.generic_u8string());
        }
    }
    for (const auto& [dst, src] : strms) {
        const fs_path parent = src.parent_path();
        auto it = subtitles.find(parent);
        if (it == subtitles.end()) {
            it = subtitles.emplace(parent, std::vector<std::wstring>()).first;
            for (auto& file : std::filesystem::directory_iterator(parent)) {
                if (!file.is_directory() && check_extension(file.path().extension())) {
                    it->second.emplace_back(file.path().filename());
                }
            }
            std::sort(it->second.begin(), it->second.end());
        }
        std::wstring prefix = src.stem();
        auto it2 = std::lower_bound(it->second.begin(), it->second.end(), prefix);
        while (it2 != it->second.end()) {
            if (it2->size() < prefix.size() || memcmp(prefix.data(), it2->data(), prefix.size() * sizeof(wchar_t))) break;
            std::error_code code;
            const fs_path from = src.parent_path() / *it2, to = dst.parent_path() / (dst.stem().wstring() + it2->substr(prefix.size()));
            std::filesystem::create_hard_link(from, to, code);
            if (code) {
                std::string message = code.message();
                spdlog::error(L"源：{} 至：{} 硬链接错误：{} 尝试符号链接", from.generic_wstring(), to.generic_wstring(), local_to_wchar(message.c_str(), message.size()));
                std::filesystem::create_symlink(from, to, code);
                if (code) {
                    message = code.message();
                    spdlog::error(L"源：{} 至：{} 符号链接错误：{} 跳过", from.generic_wstring(), to.generic_wstring(), local_to_wchar(message.c_str(), message.size()));
                } else spdlog::info("源：{} 至：{} 符号链接", from.generic_u8string(), to.generic_u8string());
            } else spdlog::info("源：{} 至：{} 硬链接", from.generic_u8string(), to.generic_u8string());
            ++it2;
        }
    }
    CoUninitialize();
}
