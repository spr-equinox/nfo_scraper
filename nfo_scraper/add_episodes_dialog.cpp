#include "add_episodes_dialog.h"

#include <curl/curl.h>
#include <rapidjson/document.h>
#include <spdlog/spdlog.h>

#include <QMessageBox>

#include "search_dialog.h"

add_episodes_dialog::add_episodes_dialog(int i, config* c, QWidget* parent)
    : QDialog(parent) {
    id = i, cfg = c, accepted = false;
    ui.setupUi(this);

    using namespace rapidjson;
    const std::string requrl = std::string("https://api.themoviedb.org/3/tv/") + std::to_string(id) + "?language=zh-CN";
    std::string reqdata = request(requrl.c_str(), cfg);
    if (reqdata.empty()) {
        return;
    }
    Document json;
    if (json.Parse(reqdata.data()).HasParseError()) {
        spdlog::error("获取到的 JSON 解析错误 数据：{}", reqdata);
        return;
    }
    seasons.reserve(json["seasons"].Size());
    for (auto&& it : json["seasons"].GetArray()) {
        const auto obj = it.GetObj();
        const int season_id = obj["season_number"].GetInt();
        seasons.push_back(season_id);
        ui.SeasonList->addItem(QString::number(season_id) + " - " + QString::fromStdString(obj["name"].GetString()));
    }

    connect(ui.GetButton, SIGNAL(clicked()), this, SLOT(GetButton_Clicked()));
    connect(ui.ConfirmButton, SIGNAL(clicked()), this, SLOT(ConfirmButton_Clicked()));
}

add_episodes_dialog::~add_episodes_dialog() {
}

fetch_episode::vec_remotes add_episodes_dialog::add_episodes(config* c, QWidget* parent, const char* str) {
    const auto [id, type, name, overview] = search_dialog::search_tvshow(c, parent, str);
    if (!id) return fetch_episode::vec_remotes();
    if (type) return {fetch_episode::remote(id, type, -1, -1, name)};
    add_episodes_dialog dialog(id, c, parent);
    dialog.exec();
    return dialog.accepted ? dialog.epis : fetch_episode::vec_remotes();
}

void add_episodes_dialog::ConfirmButton_Clicked() {
    close();
    accepted = true;
    fetch_episode::vec_remotes res;
    res.reserve(ui.EpisodeList->selectedItems().size());
    for (auto&& it : ui.EpisodeList->selectedItems())
        res.push_back(std::move(epis[ui.EpisodeList->row(it)]));
    epis = std::move(res);
}

void add_episodes_dialog::GetButton_Clicked() {
    epis.clear();
    ui.EpisodeList->clear();
    const int season = seasons[ui.SeasonList->currentIndex()];
    if (!~season) {
        spdlog::warn("Season 为空，跳过");
        return;
    }
    const std::string requrl = std::string("https://api.themoviedb.org/3/tv/") + std::to_string(id) + "/season/" + std::to_string(season) + "?language=zh-CN";
    std::string reqdata = request(requrl.c_str(), cfg);
    if (reqdata.empty()) return;
    using namespace rapidjson;
    Document json;
    if (json.Parse(reqdata.data()).HasParseError()) {
        spdlog::error("获取到的 JSON 解析错误 数据：{}", reqdata);
        return;
    }
    for (auto&& it : json["episodes"].GetArray()) {
        const auto tmp = it.GetObj();
        const int epi = tmp["episode_number"].GetInt();
        const QString name = QString::number(epi) + " - " + tmp["name"].GetString();
        epis.emplace_back(id, false, season, epi, name);
        ui.EpisodeList->addItem(name);
    }
}