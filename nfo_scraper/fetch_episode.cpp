#include "fetch_episode.h"

#include <rapidjson/document.h>
#include <spdlog/spdlog.h>

#include <QDesktopServices>
#include <QFileDialog>
#include <QMessageBox>
#include <algorithm>
#include <pugixml/pugixml.hpp>  // 已禁用 XPATH

#include "add_episodes_dialog.h"
#include "tools.h"

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
#    include <shlwapi.h>
#endif

fetch_episode::fetch_episode(config* cfg, window_init_with_data* next_window, QWidget* parent)
    : window_init_with_data(parent), cfg(cfg), next_window(next_window), cur_row(-1), running(0) {
    ui.setupUi(this);
    ui.SeasonTable->horizontalHeader()->sectionResizeMode(QHeaderView::Fixed);
    ui.SeasonTable->verticalHeader()->sectionResizeMode(QHeaderView::Fixed);
    connect(ui.SeasonTable, SIGNAL(cellDoubleClicked(int, int)), this, SLOT(Cell_DoubleClicked(int, int)));
    connect(ui.LocalRemove, SIGNAL(clicked()), this, SLOT(LocalRemove_Clicked()));
    connect(ui.LocalUp, SIGNAL(clicked()), this, SLOT(LocalUp_Clicked()));
    connect(ui.LocalDown, SIGNAL(clicked()), this, SLOT(LocalDown_Clicked()));
    connect(ui.LocalAdd, SIGNAL(clicked()), this, SLOT(LocalAdd_Clicked()));
    connect(ui.RemoteRemove, SIGNAL(clicked()), this, SLOT(RemoteRemove_Clicked()));
    connect(ui.RemoteUp, SIGNAL(clicked()), this, SLOT(RemoteUp_Clicked()));
    connect(ui.RemoteDown, SIGNAL(clicked()), this, SLOT(RemoteDown_Clicked()));
    connect(ui.RemoteAdd, SIGNAL(clicked()), this, SLOT(RemoteAdd_Clicked()));
    connect(ui.SaveButton, SIGNAL(clicked()), this, SLOT(SaveButton_Clicked()));
    connect(ui.WriteButton, SIGNAL(clicked()), this, SLOT(WriteButton_Clicked()));
    connect(ui.NextButton, SIGNAL(clicked()), this, SLOT(NextButton_Clicked()));
    connect(ui.BrowseButton, SIGNAL(clicked()), this, SLOT(BrowseButton_Clicked()));
}

fetch_episode::~fetch_episode() {}

void fetch_episode::init(void* pointer) {
    show();
    using data_type = std::tuple<vec_paths, std::vector<std::tuple<QString, QString, QString>>, std::vector<std::tuple<int, bool, int>>, seasons_directories>;
    QApplication::processEvents();
    ui.centralwidget->setEnabled(false);
    path = std::move(std::get<0>(*(data_type*)pointer)), names = std::move(std::get<1>(*(data_type*)pointer)), seasons_path = std::move(std::get<3>(*(data_type*)pointer));
    std::vector<std::tuple<int, bool, int>>& data = std::get<2>(*(data_type*)pointer);
    ui.SeasonTable->setRowCount(names.size());
    for (size_t i = 0; i < names.size(); ++i) {
        ui.SeasonTable->setItem(i, 0, new QTableWidgetItem(std::get<0>(names[i])));
        ui.SeasonTable->setItem(i, 1, new QTableWidgetItem());
    }
    local_video.resize(path.size());
    remote_data.resize(path.size());
    for (size_t i = 0; i < path.size(); ++i) {
        for (const auto& it : std::filesystem::directory_iterator(path[i]))
            if (it.is_regular_file() && cfg->check_ext(it.path().extension().generic_u8string()))
                local_video[i].emplace_back(it.path());
            else if (it.is_directory())
                addition_folder.emplace_back(it.path());
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
        sort(local_video[i].begin(), local_video[i].end(), [](const fs_path& lhs, const fs_path& rhs) -> bool {
            return !~StrCmpLogicalW(lhs.generic_wstring().c_str(), rhs.generic_wstring().c_str());
        });
#else
        sort(local_video[i].begin(), local_video[i].end());
#endif
        update_thread* t = new update_thread(std::get<0>(data[i]), std::get<1>(data[i]), std::get<2>(data[i]), remote_data.data() + i, i, cfg, this);
        ++running;
        QThreadPool::globalInstance()->start(t);
    }
    if (path.empty()) ui.centralwidget->setEnabled(true);
    delete (data_type*)pointer;
}

Q_INVOKABLE void fetch_episode::update_return(int row) {
    ui.SeasonTable->item(row, 1)->setText(local_video[row].size() == remote_data[row].size() ? "√" : "×");
    if (!--running)
        ui.centralwidget->setEnabled(true);
    return;
}

Q_INVOKABLE void fetch_episode::write_return() {
    if (!--running)
        ui.centralwidget->setEnabled(true);
    return;
}

void fetch_episode::LocalRemove_Clicked() {
    for (auto&& it : ui.LocalList->selectedItems()) {
        cur_local.erase(cur_local.begin() + ui.LocalList->row(it));
        delete it;
    }
}

void fetch_episode::LocalUp_Clicked() {
    const int row = ui.LocalList->currentRow();
    if (!row || !~row) return;
    std::swap(cur_local[row], cur_local[row - 1]);
    QListWidgetItem* upper = ui.LocalList->takeItem(row - 1);
    ui.LocalList->insertItem(row, upper);
}

void fetch_episode::LocalDown_Clicked() {
    const int row = ui.LocalList->currentRow();
    if (row + 1 == cur_local.size() || !~row) return;
    std::swap(cur_local[row], cur_local[row + 1]);
    QListWidgetItem* lower = ui.LocalList->takeItem(row + 1);
    ui.LocalList->insertItem(row, lower);
}

void fetch_episode::LocalAdd_Clicked() {
    QFileDialog* open = new QFileDialog(ui.centralwidget);
    open->setAcceptMode(QFileDialog::AcceptOpen);
    open->setFileMode(QFileDialog::ExistingFiles);
    if (open->exec() != QDialog::Accepted) {
        spdlog::info("用户取消操作");
        return;
    }
    for (auto&& it : open->selectedFiles()) {
        fs_path now = it.toStdWString();
        now.make_preferred();
        ui.LocalList->addItem(QString::fromStdWString(now.filename().generic_wstring()));
        cur_local.emplace_back(std::move(now));
    }
}

void fetch_episode::RemoteAdd_Clicked() {
    vec_remotes rem = add_episodes_dialog::add_episodes(cfg, ui.centralwidget, ui.TitleText->text().toStdString().c_str());
    for (auto&& it : rem) {
        ui.RemoteList->addItem(it.name);
        cur_remote.emplace_back(std::move(it));
    }
}

void fetch_episode::RemoteDown_Clicked() {
    const int row = ui.RemoteList->currentRow();
    if (row + 1 == cur_remote.size() || !~row) return;
    std::swap(cur_remote[row], cur_remote[row + 1]);
    QListWidgetItem* lower = ui.RemoteList->takeItem(row + 1);
    ui.RemoteList->insertItem(row, lower);
}

void fetch_episode::SaveButton_Clicked() {
    if (~cur_row) {
        local_video[cur_row] = cur_local;
        remote_data[cur_row] = cur_remote;
        ui.SeasonTable->item(cur_row, 1)->setText(local_video[cur_row].size() == remote_data[cur_row].size() ? "√" : "×");
    }
}

void fetch_episode::WriteButton_Clicked() {
    ui.centralwidget->setEnabled(false);
    bool called = true;
    const auto& index = ui.SeasonTable->selectionModel()->selectedRows();
    for (auto&& it : index) {
        const int row = it.row();
        if (local_video[row].size() != remote_data[row].size()) {
            spdlog::warn("第 {} 行未就绪，跳过", row);
            continue;
        }
        for (size_t i = 0; i < local_video[row].size(); ++i) {
            called = false;
            write_thread* t = new write_thread(remote_data[row][i].series_id, remote_data[row][i].type, remote_data[row][i].season_number, remote_data[row][i].episode_number, local_video[row][i], cfg, this, &seasons_path);
            ++running;
            QThreadPool::globalInstance()->start(t);
        }
    }
    if (called) ui.centralwidget->setEnabled(true);
}

void fetch_episode::NextButton_Clicked() {
    close();
    if (cfg->get_save_type() == 1) next_window->init(new vec_paths(std::move(addition_folder)));
    destroy();
}

void fetch_episode::BrowseButton_Clicked() {
    QDesktopServices::openUrl(QUrl::fromLocalFile(ui.PathEdit->text()));
}

void fetch_episode::RemoteUp_Clicked() {
    const int row = ui.RemoteList->currentRow();
    if (!row || !~row) return;
    std::swap(cur_remote[row], cur_remote[row - 1]);
    QListWidgetItem* upper = ui.RemoteList->takeItem(row - 1);
    ui.RemoteList->insertItem(row, upper);
}

void fetch_episode::RemoteRemove_Clicked() {
    for (auto&& it : ui.RemoteList->selectedItems()) {
        cur_remote.erase(cur_remote.begin() + ui.RemoteList->row(it));
        delete it;
    }
}

fetch_episode::update_thread::update_thread(int id, bool type, int season, vec_remotes* remotes, int row, config* cfg, QObject* object)
    : id(id), type(type), season(season), remotes(remotes), row(row), cfg(cfg), obj(object) {}

void fetch_episode::update_thread::run() {
    if (!id) {
        spdlog::warn("ID 为空，跳过");
        QMetaObject::invokeMethod(obj, "update_return", Q_ARG(int, row));
        return;
    }
    if (type) {
        const std::string requrl = std::string("https://api.themoviedb.org/3/movie/") + std::to_string(id) + "?language=zh-CN";
        std::string reqdata = request(requrl.c_str(), cfg);
        if (reqdata.empty()) {
            QMetaObject::invokeMethod(obj, "update_return", Q_ARG(int, row));
            return;
        }

        using namespace rapidjson;
        Document json;
        if (json.Parse(reqdata.data()).HasParseError()) {
            spdlog::error("获取到的 JSON 解析错误 数据：{}", reqdata);
            QMetaObject::invokeMethod(obj, "update_return", Q_ARG(int, row));
            return;
        }

        remotes->emplace_back(id, true, -1, -1, json["title"].GetString());
    } else {
        if (!~season) {
            spdlog::warn("Season 为空，跳过");
            QMetaObject::invokeMethod(obj, "update_return", Q_ARG(int, row));
            return;
        }
        const std::string requrl = std::string("https://api.themoviedb.org/3/tv/") + std::to_string(id) + "/season/" + std::to_string(season) + "?language=zh-CN";
        std::string reqdata = request(requrl.c_str(), cfg);
        if (reqdata.empty()) {
            QMetaObject::invokeMethod(obj, "update_return", Q_ARG(int, row));
            return;
        }

        using namespace rapidjson;
        Document json;
        if (json.Parse(reqdata.data()).HasParseError()) {
            spdlog::error("获取到的 JSON 解析错误 数据：{}", reqdata);
            QMetaObject::invokeMethod(obj, "update_return", Q_ARG(int, row));
            return;
        }

        for (auto&& it : json["episodes"].GetArray()) {
            const auto tmp = it.GetObj();
            const int epi = tmp["episode_number"].GetInt();
            remotes->emplace_back(id, false, season, epi, QString::number(epi) + " - " + tmp["name"].GetString());
        }
    }

    QMetaObject::invokeMethod(obj, "update_return", Q_ARG(int, row));
    return;
}

void fetch_episode::Cell_DoubleClicked(int row, int column) {
    const auto& [name, title, season_title] = names[row];
    ui.PathEdit->setText(QString::fromStdWString(path[row].generic_wstring()));
    ui.TitleText->setText(title);
    ui.SeasonText->setText(season_title);
    cur_row = row;
    cur_local = local_video[row];
    cur_remote = remote_data[row];
    ui.LocalList->clear();
    ui.RemoteList->clear();
    for (auto&& it : cur_local)
        ui.LocalList->addItem(QString::fromStdWString(it.filename().generic_wstring()));
    for (auto&& it : cur_remote)
        ui.RemoteList->addItem(it.name);
}

fetch_episode::write_thread::write_thread(int id, bool type, int season, int episode, fs_path path, config* cfg, QObject* object, seasons_directories* seasons)
    : id(id), type(type), season(season), episode(episode), path(std::move(path)), cfg(cfg), obj(object), seasons_path(seasons) {}

void fetch_episode::write_thread::run() {
    switch (cfg->get_save_type()) {
    case 1:
        write_nfo();
        break;
    case 2:
        write_strm();
        break;
    }
    QMetaObject::invokeMethod(obj, "write_return");
}

void fetch_episode::write_thread::write_nfo() {
    using namespace rapidjson;
    const std::string id_str = std::to_string(id);
    std::string requrl;
    if (type) requrl = std::string("https://api.themoviedb.org/3/movie/") + id_str + "?language=zh-CN";
    else requrl = std::string("https://api.themoviedb.org/3/tv/") + id_str + "/season/" + std::to_string(season) + "/episode/" + std::to_string(episode) + "?language=zh-CN";
    std::string reqdata = request(requrl.c_str(), cfg);
    if (reqdata.empty()) {
        spdlog::error("无法获取详细信息 路径：{}", path.generic_u8string());
        return;
    }
    Document details, keywords, credits;
    if (details.Parse(reqdata.data()).HasParseError()) {
        spdlog::error("详细信息获取到的 JSON 解析错误 数据：{}", reqdata);
        return;
    }
    if (type) {
        requrl = std::string("https://api.themoviedb.org/3/movie/") + id_str + "/keywords";
        reqdata = request(requrl.c_str(), cfg);
        if (reqdata.empty()) {
            spdlog::error("无法获取关键词 路径：{}", path.generic_u8string());
            return;
        }
        if (keywords.Parse(reqdata.data()).HasParseError()) {
            spdlog::error("关键词获取到的 JSON 解析错误 数据：{}", reqdata);
            return;
        }
    }
    if (type)
        requrl = std::string("https://api.themoviedb.org/3/movie/") + id_str + "/credits?language=zh-CN";
    else
        requrl = std::string("https://api.themoviedb.org/3/tv/") + id_str + "/season/" + std::to_string(season) + "/episode/" + std::to_string(episode) + "/credits?language=zh-CN";
    reqdata = request(requrl.c_str(), cfg);
    if (reqdata.empty()) {
        spdlog::error("无法获取制作人员数据 路径：{}", path.generic_u8string());
        return;
    }
    if (credits.Parse(reqdata.data()).HasParseError()) {
        spdlog::error("制作人员数据获取到的 JSON 解析错误 数据：{}", reqdata);
        return;
    }

    using namespace pugi;
    xml_document doc;
    xml_node dec = doc.prepend_child(node_declaration);
    if (type) doc.append_child(node_comment).set_value("这个视频是电影，这个文件是用电影的数据按照剧集格式生成的");
    dec.append_attribute("version").set_value("1.0");
    dec.append_attribute("encoding").set_value("utf-8");
    dec.append_attribute("standalone").set_value("yes");
    xml_node episodedetails = doc.append_child("episodedetails");
    episodedetails.append_child("title").append_child(node_pcdata).set_value(details[type ? "title" : "name"].GetString());
    if (type) episodedetails.append_child("originaltitle").append_child(node_pcdata).set_value(details["original_title"].GetString());
    if (type) episodedetails.append_child("showtitle").append_child(node_pcdata).set_value(details["original_title"].GetString());
    if (!type) episodedetails.append_child("season").append_child(node_pcdata).set_value(std::to_string(season).c_str());
    if (!type) episodedetails.append_child("seasonnumber").append_child(node_pcdata).set_value(std::to_string(season).c_str());
    if (!type) episodedetails.append_child("episode").append_child(node_pcdata).set_value(std::to_string(episode).c_str());
    episodedetails.append_child("plot").append_child(node_pcdata).set_value(details["overview"].GetString());
    if (details[type ? "release_date" : "air_date"].IsString())
        episodedetails.append_child("premiered").append_child(node_pcdata).set_value(details[type ? "release_date" : "air_date"].GetString());
    if (type) episodedetails.append_child("status").append_child(node_pcdata).set_value(details["status"].GetString());
    episodedetails.append_child("episodeguide").append_child(node_pcdata).set_value(("{\"tmdb\": \"" + id_str + "\"}").c_str());
    xml_node ratings = episodedetails.append_child("ratings").append_child("rating");
    ratings.append_attribute("name").set_value("themoviedb");
    ratings.append_attribute("max").set_value("10");
    ratings.append_attribute("default").set_value("yes");
    ratings.append_child("value").append_child(node_pcdata).set_value(std::to_string(details["vote_average"].GetFloat()).c_str());
    ratings.append_child("votes").append_child(node_pcdata).set_value(std::to_string(details["vote_count"].GetInt64()).c_str());

    xml_node uniqueid = episodedetails.append_child("uniqueid");
    uniqueid.append_child(node_pcdata).set_value(id_str.c_str());
    uniqueid.append_attribute("type").set_value("tmdb");
    uniqueid.append_attribute("default").set_value("true");
    if (type) {
        for (auto&& it : details["genres"].GetArray())
            episodedetails.append_child("genre").append_child(node_pcdata).set_value(it.GetObj()["name"].GetString());
        for (auto&& it : keywords[type ? "keywords" : "results"].GetArray())
            episodedetails.append_child("tag").append_child(node_pcdata).set_value(it.GetObj()["name"].GetString());
        for (auto&& it : details["production_companies"].GetArray())
            episodedetails.append_child("studio").append_child(node_pcdata).set_value(it.GetObj()["name"].GetString());
    }

    xml_node art = episodedetails.append_child("art");
    const std::string img_site("https://image.tmdb.org/t/p/original");

    if (type) {
        if (details["poster_path"].IsString()) {
            xml_node thumb1 = episodedetails.append_child("thumb");
            thumb1.append_child(node_pcdata).set_value((img_site + details["poster_path"].GetString()).c_str());
            thumb1.append_attribute("aspect").set_value("poster");
            art.append_child("poster").append_child(node_pcdata).set_value((img_site + details["poster_path"].GetString()).c_str());
        }
        if (details["backdrop_path"].IsString()) {
            xml_node thumb2 = episodedetails.append_child("thumb");
            thumb2.append_child(node_pcdata).set_value((img_site + details["backdrop_path"].GetString()).c_str());
            thumb2.append_attribute("aspect").set_value("banner");
            art.append_child("poster").append_child(node_pcdata).set_value((img_site + details["backdrop_path"].GetString()).c_str());
        }
    } else if (details["still_path"].IsString()) {
        xml_node thumb1 = episodedetails.append_child("thumb");
        thumb1.append_child(node_pcdata).set_value((img_site + details["still_path"].GetString()).c_str());
        thumb1.append_attribute("aspect").set_value("thumb");
        art.append_child("thumb").append_child(node_pcdata).set_value((img_site + details["still_path"].GetString()).c_str());
    }

    for (auto&& it : credits["cast"].GetArray()) {
        const auto&& obj = it.GetObj();
        xml_node actor = episodedetails.append_child("actor");

        actor.append_child("name").append_child(node_pcdata).set_value(obj["name"].GetString());
        if (obj["profile_path"].IsString())
            actor.append_child("thumb").append_child(node_pcdata).set_value((img_site + obj["profile_path"].GetString()).c_str());
        actor.append_child("order").append_child(node_pcdata).set_value(std::to_string(obj["order"].GetInt()).c_str());
        actor.append_child("role").append_child(node_pcdata).set_value(obj["character"].GetString());
    }

    path.replace_extension(".nfo");

    if (std::filesystem::exists(path)) {
        int ret = -1;
        QMetaObject::invokeMethod(obj, "cover_check", Qt::BlockingQueuedConnection, Q_RETURN_ARG(int, ret), Q_ARG(QString, "文件操作确认"), Q_ARG(QString, QString::fromStdWString(path.generic_wstring()) + " 已存在，是否覆盖"));
        if (ret == QMessageBox::Yes) {
            if (QFile::moveToTrash(QString::fromStdWString(path.generic_wstring())))
                spdlog::info("{} 已被移至回收站", path.generic_u8string());
            else {
                spdlog::error("{} 移至回收站失败", path.generic_u8string());
                return;
            }
        } else {
            spdlog::info("用户放弃覆盖文件：{}", path.generic_u8string());
            return;
        }
    }
    if (!doc.save_file(path.generic_wstring().c_str())) {
        spdlog::error("XML 输出故障 文件：{}", path.generic_u8string());
        return;
    }
    spdlog::info("元数据写入成功 文件：{}", path.generic_u8string());
    return;
}

void fetch_episode::write_thread::write_strm() {
    std::lock_guard lk(lock);
    auto it = seasons_path->find({type, id, season});
    if (it == seasons_path->end()) {
        using namespace rapidjson;
        const std::string id_str = std::to_string(id);
        std::string requrl = std::string("https://api.themoviedb.org/3/") + (type ? "movie/" : "tv/") + id_str + "?language=zh-CN";
        std::string reqdata = request(requrl.c_str(), cfg);
        if (reqdata.empty()) {
            spdlog::error("无法获取详细信息 路径：{}", path.generic_u8string());
            return;
        }
        Document details;
        if (details.Parse(reqdata.data()).HasParseError()) {
            spdlog::error("详细信息获取到的 JSON 解析错误 数据：{}", reqdata);
            return;
        }

        std::wstring name = utf8_to_wchar(details[type ? "title" : "name"].GetString(), details[type ? "title" : "name"].GetStringLength());
        std::wstring ori_name = name;
        if (details[type ? "release_date" : "first_air_date"].IsString()) {
            const char* tmp = details[type ? "release_date" : "first_air_date"].GetString();
            name += std::wstring(L" (") + wchar_t(tmp[0]) + wchar_t(tmp[1]) + wchar_t(tmp[2]) + wchar_t(tmp[3]) + L')';
        }
        name += L" [tmdbid-" + std::to_wstring(id) + L']';
        replace_illegal_char(name);
        fs_path folder = cfg->get_save_path() / (type ? L"电影" : L"剧集") / name;
        create_directory_with_log(folder);
        if (type) {
            seasons_path->emplace(std::tuple<bool, int, int>{true, id, -1}, std::pair<fs_path, std::wstring>{folder, std::move(ori_name)});
            return;
        }
        folder = folder / (std::wstring(L"Season ") + (season < 10 ? (L'0' + std::to_wstring(season)) : std::to_wstring(season)));
        it = seasons_path->emplace(std::tuple<bool, int, int>{false, id, season}, std::pair<fs_path, std::wstring>{folder, std::move(ori_name)}).first;
        create_directory_with_log(folder);
    }
    fs_path folder = it->second.first;
    std::wstring name = it->second.second;
    if (!type) name += std::wstring(L" S") + (season < 10 ? (L'0' + std::to_wstring(season)) : std::to_wstring(season)) + L'E' + (episode < 10 ? (L'0' + std::to_wstring(episode)) : std::to_wstring(episode));
    name += L".strm";
    folder /= name;
    if (std::filesystem::exists(folder)) {
        spdlog::warn("{} 已存在，无法链接到 {}", folder.generic_u8string(), path.generic_u8string());
        
        return;
    }
    std::ofstream o(folder);
    o << path.generic_u8string();
    if (o.fail()) spdlog::error("{} 写入错误", folder.generic_u8string());
    else spdlog::info("创建文件：{}", folder.generic_u8string());
    o.close();
}

Q_INVOKABLE int fetch_episode::cover_check(const QString& title, const QString& content) {
    return ui.CoverCheck->isChecked() ? QMessageBox::warning(this, title, content, QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes) : QMessageBox::Yes;
}