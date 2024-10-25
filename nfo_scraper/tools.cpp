#include <curl/curl.h>
#include <spdlog/spdlog.h>

#include <chrono>
#include <string>

#include "config.h"

size_t network_callback(char* ptr, size_t size, size_t nmemb, void* userdata) {
    std::string* str = (std::string*)userdata;
    const size_t len = size * nmemb;
    const int strsize = str->size();
    str->resize(strsize + len);
    memcpy(str->data() + strsize, ptr, len);
    return len;
}

std::string request(const char* url, config* cfg) {
    std::string reqdata;
    for (short i = 1; i < 6; i++) {
        CURL* hnd = curl_easy_init();

        curl_easy_setopt(hnd, CURLOPT_CUSTOMREQUEST, "GET");
        curl_easy_setopt(hnd, CURLOPT_WRITEDATA, &reqdata);
        curl_easy_setopt(hnd, CURLOPT_WRITEFUNCTION, network_callback);
        curl_easy_setopt(hnd, CURLOPT_TIMEOUT, 2);
        curl_easy_setopt(hnd, CURLOPT_URL, url);
        if (cfg->is_proxy()) curl_easy_setopt(hnd, CURLOPT_PROXY, cfg->proxy().c_str());

        struct curl_slist* headers = NULL;
        headers = curl_slist_append(headers, "accept: application/json");
        headers = curl_slist_append(headers, (std::string(("Authorization: Bearer ") + cfg->get_key()).c_str()));
        curl_easy_setopt(hnd, CURLOPT_HTTPHEADER, headers);

        auto st = std::chrono::steady_clock::now();
        CURLcode ret = curl_easy_perform(hnd);
        int http_code;
        curl_easy_getinfo(hnd, CURLINFO_RESPONSE_CODE, &http_code);
        if (ret == CURLE_OK && http_code == 200) {
            spdlog::info("URL 请求成功 耗时{}ms", std::chrono::duration_cast<std::chrono::duration<double, std::milli>>(std::chrono::steady_clock::now() - st).count());
            curl_easy_cleanup(hnd);
            break;
        }

        spdlog::warn("curl_easy_perform() 执行错误：{} HTTP状态码：{} URL：{}", curl_easy_strerror(ret), http_code, url);
        spdlog::warn("正在重试第 {}/5 遍", i);
        curl_easy_cleanup(hnd);
        reqdata.clear();
    }
    if (reqdata.empty()) spdlog::error("网络请求错误次数过多");
    return reqdata;
}

bool create_directory_with_log(const fs_path& path) {
    if (std::filesystem::exists(path)) {
        spdlog::warn("已存在目录：{}", path.generic_u8string());
        return true;
    }
    if (std::filesystem::create_directories(path)) {
        spdlog::info("成功创建目录：{}", path.generic_u8string());
        return true;
    } else {
        spdlog::error("创建目录 {} 失败", path.generic_u8string());
        return false;
    }
}

std::wstring utf8_to_wchar(const char* str, int len) {
    int wcsLen = MultiByteToWideChar(CP_UTF8, NULL, str, len, NULL, 0);
    std::wstring ret;
    ret.resize(wcsLen);
    MultiByteToWideChar(CP_UTF8, NULL, str, len, ret.data(), wcsLen);
    return ret;
}

void replace_illegal_char(std::wstring& str) {
    for (auto&& it : str) {
        switch (it) {
        case L'\\':
            it = L'＼';
            break;
        case L'/':
            it = L'／';
            break;
        case L':':
            it = L'：';
            break;
        case L'*':
            it = L'×';
            break;
        case L'?':
            it = L'？';
            break;
        case L'\"':
            it = L'\'';
            break;
        case L'<':
            it = L'＜';
            break;
        case L'>':
            it = L'＞';
            break;
        case L'|':
            it = L'丨';
            break;
        default:
            break;
        }
    }
}
