#include "config.h"

#include <rapidjson/document.h>
#include <rapidjson/error/en.h>
#include <spdlog/spdlog.h>

#include <regex>

bool config::check_ext(const std::string& ext) {
    return media_exts.count(ext);
}

bool config::check_ignore(const std::string& name) {
    for (auto&& reg : ignore_reg)
        if (std::regex_match(name, reg))
            return true;
    return false;
}

std::string config::get_shows_name(const std::string& name) {
    for (auto&& [reg, idx] : reg_str) {
        std::smatch results;
        if (std::regex_match(name, results, reg)) {
            spdlog::info("名称：{} 提取为：{}", name, std::string(results[idx]));
            return results[idx];
        }
    }
    return std::string();
}

std::string config::get_key() {
    return key;
}

char config::get_save_type() {
    return save_type;
}

fs_path config::get_save_path() {
    return save_path;
}

bool config::is_proxy() {
    return use_proxy;
}

std::string config::proxy() {
    return proxy_addr;
}

config::config(const char* file) {
    using namespace rapidjson;
    use_proxy = false;
    save_type = -1;
    std::FILE* f = nullptr;
    errno_t error_code = fopen_s(&f, file, "rb");
    if (error_code) {
        spdlog::error("配置文件打开失败 路径：{} 错误信息：{}", file, std::strerror(error_code));
        return;
    }
    _fseeki64_nolock(f, 0, SEEK_END);
    const size_t size = _ftelli64_nolock(f);
    _fseeki64_nolock(f, 0, SEEK_SET);

    char* data = (char*)std::malloc(size + 1);
    if (!data) {
        spdlog::error("分配内存失败");
        return;
    }
    data[size] = 0;
    _fread_nolock_s(data, size + 1, sizeof(char), size, f);
    _fclose_nolock(f);
    Document json;
    ParseResult json_error_code = json.Parse<kParseCommentsFlag>(data);
    free(data);
    if (!json_error_code) {
        spdlog::error("配置 JSON 解析错误：{} ({})", GetParseError_En(json_error_code.Code()), json_error_code.Offset());
        return;
    }
    if (json.HasMember("media_extension") && json["media_extension"].IsArray()) {
        for (auto&& it : json["media_extension"].GetArray())
            if (!it.IsString()) {
                spdlog::warn("配置文件中 media_extension 项有错误的类型");
                continue;
            } else media_exts.emplace(it.GetString());
    } else spdlog::warn("配置文件中未找到 media_extension 或格式错误");
    spdlog::info("视频文件后缀名：[{}]", fmt::join(media_exts, ", "));

    if (json.HasMember("title_regex_expression") && json["title_regex_expression"].IsArray())
        for (auto&& it : json["title_regex_expression"].GetArray())
            if (!it.IsArray() || it.Size() != 2 || !it[0].IsString() || !it[1].IsInt()) {
                spdlog::warn("配置文件中 title_regex_expression 项有错误的类型");
                continue;
            } else reg_str.emplace_back(it[0].GetString(), it[1].GetInt());
    else spdlog::warn("配置文件中未找到 title_regex_expression 或格式错误");
    spdlog::info("加载{}个标题匹配表达式", reg_str.size());

    if (json.HasMember("tmdb_api_key") && json["tmdb_api_key"].IsString())
        key = json["tmdb_api_key"].GetString();
    else spdlog::warn("配置文件中未找到 tmdb_api_key 或格式错误");
    if (key.empty()) spdlog::error("API key 为空");
    else spdlog::info("已读取 API key");

    if (json.HasMember("using_proxy") && json["using_proxy"].IsBool())
        use_proxy = json["using_proxy"].GetBool();
    if (use_proxy)
        if (json.HasMember("proxy_address") && json["proxy_address"].IsString()) {
            proxy_addr = json["proxy_address"].GetString();
            spdlog::info("代理地址：{}", proxy_addr);
        } else spdlog::warn("代理地址配置错误");
    else spdlog::info("不使用代理");

    if (json.HasMember("ignore_expression") && json["ignore_expression"].IsArray())
        for (auto&& it : json["ignore_expression"].GetArray())
            if (!it.IsString()) {
                spdlog::warn("配置文件中 ignore_expression 项有错误的类型");
                continue;
            } else ignore_reg.emplace_back(it.GetString());
    else spdlog::info("配置文件中未找到 ignore_expression 或格式错误");
    spdlog::info("加载{}个标忽略匹配表达式", ignore_reg.size());

    if (json.HasMember("save_type") && json["save_type"].IsString()) {
        const std::string type = json["save_type"].GetString();
        spdlog::info("保存类型：{}", type);
        if (type == "strm") {
            save_type = 2;
        } else if (type == "nfo") {
            save_type = 1;
        }
    } else spdlog::error("配置文件中 save_type 项类型错误");
    if (save_type == 2)
        if (json.HasMember("save_path") && json["save_path"].IsString()) {
            save_path = json["save_path"].GetString();
            spdlog::info("保存地址：{}", save_path.generic_u8string());
            if (!std::filesystem::exists(save_path)) {
                spdlog::error("保存地址 {} 不存在", save_path.generic_u8string());
                save_type = -1;
            } else if (std::filesystem::is_directory(save_path) && !std::filesystem::is_empty(save_path)) {
                spdlog::error("保存地址 {} 不为空白文件夹，为了安全，请选择空白文件夹", save_path.generic_u8string());
                save_type = -1;
            } else if (!create_directory_with_log(save_path / L"剧集") || !create_directory_with_log(save_path / L"电影")) {
                spdlog::error("保存地址 {} 无法创建文件夹", save_path.generic_u8string());
                save_type = -1;
            }
        } else spdlog::error("配置文件中 save_type 项类型错误");
}
