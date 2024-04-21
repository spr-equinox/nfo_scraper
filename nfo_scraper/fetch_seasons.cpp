#include "fetch_seasons.h"

#include <curl/curl.h>
#include <rapidjson/document.h>
#include <spdlog/spdlog.h>

#include <QComboBox>
#include <QDesktopServices>
#include <QFile>
#include <QLineEdit>
#include <QMessageBox>
#include <QUrl>
#include <chrono>
#include <fstream>
#include <pugixml/pugixml.hpp>  // 已禁用 XPATH

#include "search_dialog.h"
#include "tools.h"

fetch_seasons::fetch_seasons(config* c, fetch_episode* w, QWidget* parent)
    : QMainWindow(parent) {
    cfg = c, next_window = w;
    ui.setupUi(this);
    ui.seasonsTable->horizontalHeader()->sectionResizeMode(QHeaderView::Fixed);
    ui.seasonsTable->verticalHeader()->sectionResizeMode(QHeaderView::Fixed);
    menu = new QMenu(this);
    auto act = new QAction(menu);
    act->setText("打开所选项的路径");
    connect(act, SIGNAL(triggered()), this, SLOT(RightMenuAction_Clicked()));
    menu->addAction(act);
    connect(ui.SearchSelected, SIGNAL(clicked()), this, SLOT(SearchSelected_Clicked()));
    connect(ui.UpdateSelected, SIGNAL(clicked()), this, SLOT(UpdateSelected_Clicked()));
    connect(ui.WriteSelected, SIGNAL(clicked()), this, SLOT(WriteSelected_Clicked()));
    connect(ui.Next, SIGNAL(clicked()), this, SLOT(Next_Clicked()));
    connect(ui.seasonsTable, SIGNAL(customContextMenuRequested(const QPoint&)), this, SLOT(RightMenu_Clicked(const QPoint&)));
    connect(ui.DialogSearchSelected, SIGNAL(clicked()), this, SLOT(DialogSearchSelected_Clicked()));
    connect(ui.seasonsTable, SIGNAL(cellDoubleClicked(int, int)), this, SLOT(Cell_DoubleClicked(int, int)));
}

void fetch_seasons::set_library(std::unordered_map<path, vec_paths>&& source, std::vector<std::tuple<QString, QString, bool> >&& vals) {
    library = std::move(source);
    int idx = 0;
    for (auto&& it : library)
        idx += it.second.size();
    ui.seasonsTable->setRowCount(idx);
    seasons.reserve(idx);
    idx = 0;
    uint now = 0;
    all_setEnable(false);
    for (auto&& it : library) {
        const QString folder = QString::fromStdString(it.first.generic_u8string());
        for (auto&& it2 : it.second) {
            const std::string name = it2.generic_u8string();

            auto item0 = new QTableWidgetItem(folder);
            item0->setFlags(item0->flags() ^ Qt::ItemIsEditable);
            ui.seasonsTable->setItem(idx, 0, item0);

            auto item1 = new QTableWidgetItem(QString::fromStdString(name));
            item1->setFlags(item1->flags() ^ Qt::ItemIsEditable);
            ui.seasonsTable->setItem(idx, 1, item1);

            auto item2 = new QTableWidgetItem(QString::fromStdString(cfg->get_shows_name(name)));
            ui.seasonsTable->setItem(idx, 2, item2);

            auto item3 = new QLineEdit(ui.seasonsTable);
            item3->setValidator(new QIntValidator());
            item3->setText(std::get<0>(vals[now]));
            ui.seasonsTable->setCellWidget(idx, 3, item3);
            cell_pos.emplace(item3, idx);
            connect(item3, SIGNAL(textEdited(const QString&)), this, SLOT(ID_Changed(const QString&)));

            auto item4 = new QTableWidgetItem();
            item4->setFlags(item4->flags() ^ Qt::ItemIsEditable);
            item4->setText(std::get<1>(vals[now]));
            ui.seasonsTable->setItem(idx, 4, item4);

            auto item5 = new QComboBox();
            item5->addItem(QString("tv"));
            item5->addItem(QString("movie"));
            item5->setCurrentIndex(std::get<2>(vals[now]));
            ui.seasonsTable->setCellWidget(idx, 5, item5);
            cell_pos.emplace(item5, idx);
            connect(item5, SIGNAL(currentIndexChanged(int)), this, SLOT(Type_Changed(int)));

            auto item6 = new QComboBox();
            ui.seasonsTable->setCellWidget(idx, 6, item6);
            cell_pos.emplace(item6, idx);
            connect(item6, SIGNAL(currentIndexChanged(int)), this, SLOT(Season_Changed(int)));

            auto item7 = new QTableWidgetItem();
            item7->setFlags(item7->flags() ^ Qt::ItemIsEditable);
            ui.seasonsTable->setItem(idx, 7, item7);

            seasons.emplace_back(it.first / it2, vec_data());
            ++idx;
        }
        ++now;
        // QApplication::processEvents();
    }
    all_setEnable(true);
    // ui.seasonsTable->resizeColumnsToContents();
    // ui.seasonsTable->resizeRowsToContents();
}

Q_INVOKABLE void fetch_seasons::thread_return(QString result, int row, QString title, int type, QString overview, vec_data data) {
    QLineEdit* edit = (QLineEdit*)ui.seasonsTable->cellWidget(row, 3);
    edit->blockSignals(true);
    edit->setText(std::move(result));
    edit->blockSignals(false);
    ui.seasonsTable->item(row, 4)->setText(std::move(title));
    QComboBox* pt = (QComboBox*)ui.seasonsTable->cellWidget(row, 5);
    pt->blockSignals(true);
    pt->setCurrentIndex(type);
    pt->blockSignals(false);
    pt = (QComboBox*)ui.seasonsTable->cellWidget(row, 6);
    pt->blockSignals(true);
    pt->clear();
    seasons[row].second = std::move(data);
    bool set_to_1 = false;
    for (auto&& [id, name, overview] : seasons[row].second) {
        if (!id) set_to_1 = true;
        pt->addItem(QString::number(id) + " - " + QString::fromStdString(name));
    }
    if (set_to_1 && seasons[row].second.size() > 1) {
        pt->setCurrentIndex(1);
        ui.seasonsTable->item(row, 7)->setText(QString::fromStdString(seasons[row].second[1].overview));
    } else if (!seasons[row].second.empty())
        ui.seasonsTable->item(row, 7)->setText(QString::fromStdString(seasons[row].second[0].overview));
    else ui.seasonsTable->item(row, 7)->setText(std::move(overview));
    pt->blockSignals(false);
    if (!QThreadPool::globalInstance()->activeThreadCount()) {
        // ui.seasonsTable->resizeColumnsToContents();
        // ui.seasonsTable->resizeRowsToContents();
        all_setEnable(true);
        QMessageBox::information(ui.Widget1, "信息", "已完成抓取", QMessageBox::Ok, QMessageBox::Ok);
    }
}

Q_INVOKABLE void fetch_seasons::write_return() {
    if (!QThreadPool::globalInstance()->activeThreadCount()) {
        // ui.seasonsTable->resizeColumnsToContents();
        // ui.seasonsTable->resizeRowsToContents();
        all_setEnable(true);
        QMessageBox::information(ui.Widget1, "信息", "已完成写入", QMessageBox::Ok, QMessageBox::Ok);
    }
}

Q_INVOKABLE int fetch_seasons::cover_check(const QString& title, const QString& content) {
    return ui.CoverCheck->isChecked() ? QMessageBox::warning(this, title, content, QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes) : QMessageBox::Yes;
}

fetch_seasons::~fetch_seasons() {}

void fetch_seasons::SearchSelected_Clicked() {
    const auto& index = ui.seasonsTable->selectionModel()->selectedRows();
    if (index.empty()) return;
    all_setEnable(false);
    for (auto&& it : index) {
        const std::string search = ui.seasonsTable->item(it.row(), 2)->text().toStdString();
        search_thread* t = new search_thread(search, it.row(), cfg, this);
        QThreadPool::globalInstance()->start(t);
    }
}

void fetch_seasons::all_setEnable(bool status) {
    ui.Widget1->setEnabled(status);
}

void fetch_seasons::UpdateSelected_Clicked() {
    const auto& index = ui.seasonsTable->selectionModel()->selectedRows();
    if (index.empty()) return;
    all_setEnable(false);
    for (auto&& it : index) {
        const std::string search = ui.seasonsTable->item(it.row(), 2)->text().toStdString();
        QLineEdit* edit = (QLineEdit*)ui.seasonsTable->cellWidget(it.row(), 3);
        QComboBox* type = (QComboBox*)ui.seasonsTable->cellWidget(it.row(), 5);
        update_thread* t = new update_thread(edit->text().toInt(), it.row(), type->currentIndex(), cfg, this);
        QThreadPool::globalInstance()->start(t);
    }
}

void fetch_seasons::WriteSelected_Clicked() {
    all_setEnable(false);
    bool called = true;
    const auto& index = ui.seasonsTable->selectionModel()->selectedRows();
    for (auto&& it : index) {
        QString edit = ((QLineEdit*)ui.seasonsTable->cellWidget(it.row(), 3))->text();
        if (edit.isEmpty()) {
            spdlog::warn("缺少 TMDB ID，项目：{}", seasons[it.row()].first.generic_u8string());
            continue;
        }
        bool type = ((QComboBox*)ui.seasonsTable->cellWidget(it.row(), 5))->currentIndex();
        if (!type && seasons[it.row()].second.empty()) {
            spdlog::warn("缺少季度信息，项目：{}", seasons[it.row()].first.generic_u8string());
            continue;
        }
        called = false;
        write_thread* t = new write_thread(edit.toInt(), type ? -1 : seasons[it.row()].second[((QComboBox*)ui.seasonsTable->cellWidget(it.row(), 6))->currentIndex()].id, type, seasons[it.row()].first, cfg, this);
        QThreadPool::globalInstance()->start(t);
    }
    if (called) all_setEnable(true);
}

void fetch_seasons::RightMenu_Clicked(const QPoint& point) {
    menu->exec(QCursor::pos());
}

void fetch_seasons::RightMenuAction_Clicked() {
    const auto& index = ui.seasonsTable->selectionModel()->selectedRows();
    for (auto&& it : index) {
        QDesktopServices::openUrl(QUrl::fromLocalFile(ui.seasonsTable->item(it.row(), 0)->text()));
    }
}

void fetch_seasons::ID_Changed(const QString& text) {
    int row = cell_pos.find(sender())->second;
    ui.seasonsTable->item(row, 4)->setText(QString());
    QComboBox* pt = (QComboBox*)ui.seasonsTable->cellWidget(row, 5);
    pt->blockSignals(true);
    pt->setCurrentIndex(0);
    pt->blockSignals(false);
    pt = (QComboBox*)ui.seasonsTable->cellWidget(row, 6);
    pt->blockSignals(true);
    pt->clear();
    pt->blockSignals(false);
    seasons[row].second.clear();
    ui.seasonsTable->item(row, 7)->setText(QString());
}

void fetch_seasons::Type_Changed(int index) {
    int row = cell_pos.find(sender())->second;
    ui.seasonsTable->item(row, 4)->setText(QString());
    QComboBox* pt = (QComboBox*)ui.seasonsTable->cellWidget(row, 6);
    pt->blockSignals(true);
    pt->clear();
    pt->blockSignals(false);
    seasons[row].second.clear();
    ui.seasonsTable->item(row, 7)->setText(QString());
}

void fetch_seasons::Season_Changed(int index) {
    int row = cell_pos.find(sender())->second;
    ui.seasonsTable->item(row, 7)->setText(QString::fromStdString(seasons[row].second[index].overview));
}

fetch_seasons::search_thread::search_thread(const std::string& n, int r, config* c, QObject* object) {
    name = n, row = r, cfg = c, obj = object;
}

void fetch_seasons::search_thread::run() {
    char* url = curl_escape(name.c_str(), name.size());
    std::string requrl = std::string("https://api.themoviedb.org/3/search/multi?query=") + url + "&include_adult=false&language=zh-CN&page=1";
    curl_free(url);
    std::string reqdata = request(requrl.c_str(), cfg);
    if (reqdata.empty()) {
        QMetaObject::invokeMethod(obj, "thread_return", Q_ARG(QString, QString()), Q_ARG(int, row), Q_ARG(QString, QString()), Q_ARG(int, 0), Q_ARG(QString, QString()), Q_ARG(vec_data, vec_data()));
        return;
    }

    using namespace rapidjson;
    Document json;
    if (json.Parse(reqdata.data()).HasParseError()) {
        spdlog::error("获取到的 JSON 解析错误 数据：{}", reqdata);
        QMetaObject::invokeMethod(obj, "thread_return", Q_ARG(QString, QString()), Q_ARG(int, row), Q_ARG(QString, QString()), Q_ARG(int, 0), Q_ARG(QString, QString()), Q_ARG(vec_data, vec_data()));
        return;
    }

    QString id, title, overview;
    bool type = false;

    if (!json.HasMember("results") || json["results"].Empty()) {
        spdlog::warn("什么也没搜到 标题：{}", name);
        QMetaObject::invokeMethod(obj, "thread_return", Q_ARG(QString, QString()), Q_ARG(int, row), Q_ARG(QString, QString()), Q_ARG(int, 0), Q_ARG(QString, QString()), Q_ARG(vec_data, vec_data()));
        return;
    } else {
        auto&& tmp = json["results"][0].GetObj();
        id = QString::number(tmp["id"].GetInt());
        if (!std::strcmp(tmp["media_type"].GetString(), "tv")) {
            type = false;
            title = tmp["name"].GetString();
        } else if (!std::strcmp(tmp["media_type"].GetString(), "movie")) {
            type = true;
            title = tmp["title"].GetString();
        } else {
            spdlog::warn("未知的类别 标题：{}", name, tmp["media_type"].GetString());
        }
        overview = tmp["overview"].GetString();
    }

    vec_data data;
    if (!type) {
        const std::string requrl = std::string("https://api.themoviedb.org/3/tv/") + id.toStdString() + "?language=zh-CN";
        std::string reqdata = request(requrl.c_str(), cfg);
        if (reqdata.empty()) {
            QMetaObject::invokeMethod(obj, "thread_return", Q_ARG(QString, id), Q_ARG(int, row), Q_ARG(QString, std::move(title)), Q_ARG(int, type), Q_ARG(QString, std::move(overview)), Q_ARG(vec_data, data));
            return;
        }
        Document json;
        if (json.Parse(reqdata.data()).HasParseError()) {
            spdlog::error("获取到的 JSON 解析错误 数据：{}", reqdata);
            QMetaObject::invokeMethod(obj, "thread_return", Q_ARG(QString, id), Q_ARG(int, row), Q_ARG(QString, std::move(title)), Q_ARG(int, type), Q_ARG(QString, std::move(overview)), Q_ARG(vec_data, data));
            return;
        }
        data.reserve(json["seasons"].Size());
        for (auto&& it : json["seasons"].GetArray()) {
            const auto obj = it.GetObj();
            data.emplace_back(obj["season_number"].GetInt(), obj["name"].GetString(), obj["overview"].GetString());
        }
    }
    QMetaObject::invokeMethod(obj, "thread_return", Q_ARG(QString, id), Q_ARG(int, row), Q_ARG(QString, std::move(title)), Q_ARG(int, type), Q_ARG(QString, std::move(overview)), Q_ARG(vec_data, data));
    return;
}

fetch_seasons::update_thread::update_thread(int i, int r, bool t, config* c, QObject* object) {
    id = i, row = r, type = t, cfg = c, obj = object;
}

void fetch_seasons::update_thread::run() {
    if (!id) {
        spdlog::warn("第 {} 行 ID 为空", row);
        QMetaObject::invokeMethod(obj, "thread_return", Q_ARG(QString, QString()), Q_ARG(int, row), Q_ARG(QString, QString()), Q_ARG(int, 0), Q_ARG(QString, QString()), Q_ARG(vec_data, vec_data()));
        return;
    }
    const std::string requrl = std::string("https://api.themoviedb.org/3/") + (type ? "movie/" : "tv/") + std::to_string(id) + "?language=zh-CN";
    std::string reqdata = request(requrl.c_str(), cfg);
    if (reqdata.empty()) {
        QMetaObject::invokeMethod(obj, "thread_return", Q_ARG(QString, QString()), Q_ARG(int, row), Q_ARG(QString, QString()), Q_ARG(int, 0), Q_ARG(QString, QString()), Q_ARG(vec_data, vec_data()));
        return;
    }

    using namespace rapidjson;
    Document json;
    if (json.Parse(reqdata.data()).HasParseError()) {
        spdlog::error("获取到的 JSON 解析错误 数据：{}", reqdata);
        QMetaObject::invokeMethod(obj, "thread_return", Q_ARG(QString, QString()), Q_ARG(int, row), Q_ARG(QString, QString()), Q_ARG(int, 0), Q_ARG(QString, QString()), Q_ARG(vec_data, vec_data()));
        return;
    }

    QString i = QString::number(id), title, overview;
    vec_data data;
    if (type) title = json["title"].GetString();
    else {
        title = json["name"].GetString();
        data.reserve(json["seasons"].Size());
        for (auto&& it : json["seasons"].GetArray()) {
            const auto obj = it.GetObj();
            data.emplace_back(obj["season_number"].GetInt(), obj["name"].GetString(), obj["overview"].GetString());
        }
    }
    overview = json["overview"].GetString();

    QMetaObject::invokeMethod(obj, "thread_return", Q_ARG(QString, i), Q_ARG(int, row), Q_ARG(QString, std::move(title)), Q_ARG(int, type), Q_ARG(QString, std::move(overview)), Q_ARG(vec_data, std::move(data)));
    return;
}

fetch_seasons::write_thread::write_thread(int i, int s, bool t, path p, config* c, QObject* object) {
    id = i, season_num = s, pth = p, type = t, cfg = c, obj = object;
}

void fetch_seasons::write_thread::run() {
    using namespace rapidjson;
    const std::string id_str = std::to_string(id);
    std::string requrl;
    if (type)
        requrl = std::string("https://api.themoviedb.org/3/movie/") + id_str + "?language=zh-CN";
    else
        requrl = std::string("https://api.themoviedb.org/3/tv/") + id_str + "/season/" + std::to_string(season_num) + "?language=zh-CN";
    std::string reqdata = request(requrl.c_str(), cfg);
    if (reqdata.empty()) {
        spdlog::error("无法获取详细信息 路径：{}", pth.generic_u8string());
        QMetaObject::invokeMethod(obj, "write_return");
        return;
    }
    Document details, keywords, credits;
    if (details.Parse(reqdata.data()).HasParseError()) {
        spdlog::error("详细信息获取到的 JSON 解析错误 数据：{}", reqdata);
        QMetaObject::invokeMethod(obj, "write_return");
        return;
    }
    if (type) {
        requrl = std::string("https://api.themoviedb.org/3/movie/") + id_str + "/keywords";
        reqdata = request(requrl.c_str(), cfg);
        if (reqdata.empty()) {
            spdlog::error("无法获取关键词 路径：{}", pth.generic_u8string());
            QMetaObject::invokeMethod(obj, "write_return");
            return;
        }
        if (keywords.Parse(reqdata.data()).HasParseError()) {
            spdlog::error("关键词获取到的 JSON 解析错误 数据：{}", reqdata);
            QMetaObject::invokeMethod(obj, "write_return");
            return;
        }
    }
    if (type)
        requrl = std::string("https://api.themoviedb.org/3/movie/") + id_str + "/credits?language=zh-CN";
    else
        requrl = std::string("https://api.themoviedb.org/3/tv/") + id_str + "/season/" + std::to_string(season_num) + "/aggregate_credits?language=zh-CN";
    reqdata = request(requrl.c_str(), cfg);
    if (reqdata.empty()) {
        spdlog::error("无法获取制作人员数据 路径：{}", pth.generic_u8string());
        QMetaObject::invokeMethod(obj, "write_return");
        return;
    }
    if (credits.Parse(reqdata.data()).HasParseError()) {
        spdlog::error("制作人员数据获取到的 JSON 解析错误 数据：{}", reqdata);
        QMetaObject::invokeMethod(obj, "write_return");
        return;
    }

    using namespace pugi;
    xml_document doc;
    xml_node dec = doc.prepend_child(node_declaration);
    if (type) doc.append_child(node_comment).set_value("这个视频是电影，这个文件是用电影的数据按照剧集格式生成的");
    dec.append_attribute("version").set_value("1.0");
    dec.append_attribute("encoding").set_value("utf-8");
    dec.append_attribute("standalone").set_value("yes");
    xml_node season = doc.append_child("season");
    season.append_child("title").append_child(node_pcdata).set_value(details[type ? "title" : "name"].GetString());
    if (type) season.append_child("originaltitle").append_child(node_pcdata).set_value(details["original_title"].GetString());
    if (type) season.append_child("showtitle").append_child(node_pcdata).set_value(details["original_title"].GetString());
    if (!type) season.append_child("season").append_child(node_pcdata).set_value(std::to_string(season_num).c_str());
    if (!type) season.append_child("seasonnumber").append_child(node_pcdata).set_value(std::to_string(season_num).c_str());
    season.append_child("plot").append_child(node_pcdata).set_value(details["overview"].GetString());
    season.append_child("premiered").append_child(node_pcdata).set_value(details[type ? "release_date" : "air_date"].GetString());
    if (type) season.append_child("status").append_child(node_pcdata).set_value(details["status"].GetString());
    season.append_child("episodeguide").append_child(node_pcdata).set_value(("{\"tmdb\": \"" + id_str + "\"}").c_str());
    xml_node ratings = season.append_child("ratings").append_child("rating");
    ratings.append_attribute("name").set_value("themoviedb");
    ratings.append_attribute("max").set_value("10");
    ratings.append_attribute("default").set_value("yes");
    ratings.append_child("value").append_child(node_pcdata).set_value(std::to_string(details["vote_average"].GetFloat()).c_str());
    if (type) ratings.append_child("votes").append_child(node_pcdata).set_value(std::to_string(details["vote_count"].GetInt64()).c_str());

    xml_node uniqueid = season.append_child("uniqueid");
    uniqueid.append_child(node_pcdata).set_value(id_str.c_str());
    uniqueid.append_attribute("type").set_value("tmdb");
    uniqueid.append_attribute("default").set_value("true");
    if (type) {
        for (auto&& it : details["genres"].GetArray())
            season.append_child("genre").append_child(node_pcdata).set_value(it.GetObj()["name"].GetString());
        for (auto&& it : keywords[type ? "keywords" : "results"].GetArray())
            season.append_child("tag").append_child(node_pcdata).set_value(it.GetObj()["name"].GetString());
        for (auto&& it : details["production_companies"].GetArray())
            season.append_child("studio").append_child(node_pcdata).set_value(it.GetObj()["name"].GetString());
    }

    xml_node art = season.append_child("art");
    const std::string img_site("https://image.tmdb.org/t/p/original");
    if (details["poster_path"].IsString()) {
        xml_node thumb1 = season.append_child("thumb");
        thumb1.append_child(node_pcdata).set_value((img_site + details["poster_path"].GetString()).c_str());
        thumb1.append_attribute("aspect").set_value("poster");
        art.append_child("poster").append_child(node_pcdata).set_value((img_site + details["poster_path"].GetString()).c_str());
    }

    if (type && details["backdrop_path"].IsString()) {
        xml_node thumb2 = season.append_child("thumb");
        thumb2.append_child(node_pcdata).set_value((img_site + details["backdrop_path"].GetString()).c_str());
        thumb2.append_attribute("aspect").set_value("banner");
        art.append_child("poster").append_child(node_pcdata).set_value((img_site + details["backdrop_path"].GetString()).c_str());
    }

    for (auto&& it : credits["cast"].GetArray()) {
        const auto&& obj = it.GetObj();
        xml_node actor = season.append_child("actor");

        actor.append_child("name").append_child(node_pcdata).set_value(obj["name"].GetString());
        if (obj["profile_path"].IsString())
            actor.append_child("thumb").append_child(node_pcdata).set_value((img_site + obj["profile_path"].GetString()).c_str());
        actor.append_child("order").append_child(node_pcdata).set_value(std::to_string(obj["order"].GetInt()).c_str());
        if (type) actor.append_child("role").append_child(node_pcdata).set_value(obj["character"].GetString());
        else actor.append_child("role").append_child(node_pcdata).set_value(obj["roles"][0].GetObj()["character"].GetString());
    }

    const path write = pth / "season.nfo";

    if (std::filesystem::exists(write)) {
        int ret = -1;
        QMetaObject::invokeMethod(obj, "cover_check", Qt::BlockingQueuedConnection, Q_RETURN_ARG(int, ret), Q_ARG(QString, "文件操作确认"), Q_ARG(QString, QString::fromStdString(write.u8string()) + " 已存在，是否覆盖"));
        if (ret == QMessageBox::Yes) {
            if (QFile::moveToTrash(QString::fromStdString(write.u8string())))
                spdlog::info("{} 已被移至回收站", write.u8string());
            else {
                spdlog::error("{} 移至回收站失败", write.u8string());
                QMetaObject::invokeMethod(obj, "write_return");
                return;
            }
        } else {
            spdlog::info("用户放弃覆盖文件：{}", write.u8string());
            QMetaObject::invokeMethod(obj, "write_return");
            return;
        }
    }
    if (!doc.save_file(write.generic_wstring().c_str())) {
        spdlog::error("XML 输出故障 文件：{}", write.u8string());
        QMetaObject::invokeMethod(obj, "write_return");
        return;
    }
    spdlog::info("元数据写入成功 文件：{}", write.u8string());

    QMetaObject::invokeMethod(obj, "write_return");
    return;
}

void fetch_seasons::DialogSearchSelected_Clicked() {
    const auto& index = ui.seasonsTable->selectionModel()->selectedRows();
    for (auto&& it : index) {
        const std::string search = ui.seasonsTable->item(it.row(), 2)->text().toStdString();
        const auto& [id, type, name, overview] = search_dialog::search_tvshow(cfg, ui.Widget1, search.c_str());

        QLineEdit* edit = (QLineEdit*)ui.seasonsTable->cellWidget(it.row(), 3);
        edit->blockSignals(true);
        edit->setText(id ? QString::number(id) : QString());
        edit->blockSignals(false);
        ui.seasonsTable->item(it.row(), 4)->setText(name);
        QComboBox* pt = (QComboBox*)ui.seasonsTable->cellWidget(it.row(), 5);
        pt->blockSignals(true);
        pt->setCurrentIndex(type);
        pt->blockSignals(false);
        pt = (QComboBox*)ui.seasonsTable->cellWidget(it.row(), 6);
        pt->blockSignals(true);
        pt->clear();

        vec_data data;
        if (!type) {
            using namespace rapidjson;
            const std::string requrl = std::string("https://api.themoviedb.org/3/tv/") + std::to_string(id) + "?language=zh-CN";
            std::string reqdata = request(requrl.c_str(), cfg);
            if (!reqdata.empty()) {
                Document json;
                if (!json.Parse(reqdata.data()).HasParseError()) {
                    data.reserve(json["seasons"].Size());
                    for (auto&& it : json["seasons"].GetArray()) {
                        const auto obj = it.GetObj();
                        data.emplace_back(obj["season_number"].GetInt(), obj["name"].GetString(), obj["overview"].GetString());
                    }
                } else spdlog::error("获取到的 JSON 解析错误 数据：{}", reqdata);
            }
        }

        seasons[it.row()].second = std::move(data);
        bool set_to_1 = false;
        for (auto&& [id, name, overview] : seasons[it.row()].second) {
            if (!id) set_to_1 = true;
            pt->addItem(QString::number(id) + " - " + QString::fromStdString(name));
        }
        if (set_to_1 && seasons[it.row()].second.size() > 1) {
            pt->setCurrentIndex(1);
            ui.seasonsTable->item(it.row(), 7)->setText(QString::fromStdString(seasons[it.row()].second[1].overview));
        } else if (!seasons[it.row()].second.empty())
            ui.seasonsTable->item(it.row(), 7)->setText(QString::fromStdString(seasons[it.row()].second[0].overview));
        pt->blockSignals(false);
    }
}

void fetch_seasons::Cell_DoubleClicked(int row, int column) {
    const std::string search = ui.seasonsTable->item(row, 2)->text().toStdString();
    const auto& [id, type, name, overview] = search_dialog::search_tvshow(cfg, ui.Widget1, search.c_str());

    QLineEdit* edit = (QLineEdit*)ui.seasonsTable->cellWidget(row, 3);
    edit->blockSignals(true);
    edit->setText(id ? QString::number(id) : QString());
    edit->blockSignals(false);
    ui.seasonsTable->item(row, 4)->setText(name);
    QComboBox* pt = (QComboBox*)ui.seasonsTable->cellWidget(row, 5);
    pt->blockSignals(true);
    pt->setCurrentIndex(type);
    pt->blockSignals(false);
    pt = (QComboBox*)ui.seasonsTable->cellWidget(row, 6);
    pt->blockSignals(true);
    pt->clear();

    vec_data data;
    if (!type) {
        using namespace rapidjson;
        const std::string requrl = std::string("https://api.themoviedb.org/3/tv/") + std::to_string(id) + "?language=zh-CN";
        std::string reqdata = request(requrl.c_str(), cfg);
        if (!reqdata.empty()) {
            Document json;
            if (!json.Parse(reqdata.data()).HasParseError()) {
                data.reserve(json["seasons"].Size());
                for (auto&& it : json["seasons"].GetArray()) {
                    const auto obj = it.GetObj();
                    data.emplace_back(obj["season_number"].GetInt(), obj["name"].GetString(), obj["overview"].GetString());
                }
            } else spdlog::error("获取到的 JSON 解析错误 数据：{}", reqdata);
        }
    }

    seasons[row].second = std::move(data);
    bool set_to_1 = false;
    for (auto&& [id, name, overview] : seasons[row].second) {
        if (!id) set_to_1 = true;
        pt->addItem(QString::number(id) + " - " + QString::fromStdString(name));
    }
    if (set_to_1 && seasons[row].second.size() > 1) {
        pt->setCurrentIndex(1);
        ui.seasonsTable->item(row, 7)->setText(QString::fromStdString(seasons[row].second[1].overview));
    } else if (!seasons[row].second.empty())
        ui.seasonsTable->item(row, 7)->setText(QString::fromStdString(seasons[row].second[0].overview));
    pt->blockSignals(false);
}

void fetch_seasons::Next_Clicked() {
    spdlog::info("第四阶段结束");
    close();
    vec_paths pth;
    std::vector<std::tuple<QString, QString, QString> > names;
    std::vector<std::tuple<int, bool, int> > data;
    pth.reserve(seasons.size());
    for (size_t i = 0; i < seasons.size(); ++i) {
        pth.emplace_back(std::move(seasons[i].first));
        names.emplace_back(ui.seasonsTable->item(i, 2)->text(), ui.seasonsTable->item(i, 4)->text(), ((QComboBox*)ui.seasonsTable->cellWidget(i, 6))->currentText());
        int id = ((QLineEdit*)ui.seasonsTable->cellWidget(i, 3))->text().toInt();
        bool type = ((QComboBox*)ui.seasonsTable->cellWidget(i, 5))->currentIndex();
        if (id && !type) data.emplace_back(id, type, !seasons[i].second.empty() ? seasons[i].second[((QComboBox*)ui.seasonsTable->cellWidget(i, 6))->currentIndex()].id : -1);
        else data.emplace_back(id, type, -1);
    }
    library.clear();
    seasons.clear();
    next_window->show();
    // path name title seasontitle id type seasonid
    next_window->set_library(std::move(pth), std::move(names), std::move(data));
    destroy();
}