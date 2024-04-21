#include "fetch_shows.h"

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

fetch_shows::fetch_shows(config* c, fetch_seasons* w, QWidget* parent)
    : QMainWindow(parent) {
    cfg = c, next_window = w;
    ui.setupUi(this);
    ui.showsTable->horizontalHeader()->sectionResizeMode(QHeaderView::Fixed);
    ui.showsTable->verticalHeader()->sectionResizeMode(QHeaderView::Fixed);
    menu = new QMenu(this);
    auto act = new QAction(menu);
    act->setText("打开所选项的路径");
    connect(act, SIGNAL(triggered()), this, SLOT(RightMenuAction_Clicked()));
    menu->addAction(act);
    connect(ui.SearchSelected, SIGNAL(clicked()), this, SLOT(SearchSelected_Clicked()));
    connect(ui.UpdateSelected, SIGNAL(clicked()), this, SLOT(UpdateSelected_Clicked()));
    connect(ui.Next, SIGNAL(clicked()), this, SLOT(Next_Clicked()));
    connect(ui.WriteSelected, SIGNAL(clicked()), this, SLOT(WriteSelected_Clicked()));
    connect(ui.DialogSearchSelected, SIGNAL(clicked()), this, SLOT(DialogSearchSelected_Clicked()));
    connect(ui.showsTable, SIGNAL(customContextMenuRequested(const QPoint&)), this, SLOT(RightMenu_Clicked(const QPoint&)));
    connect(ui.showsTable, SIGNAL(cellDoubleClicked(int, int)), this, SLOT(Cell_DoubleClicked(int, int)));
}

void fetch_shows::set_library(std::unordered_map<path, vec_paths>&& source) {
    library = std::move(source);
    ui.showsTable->setRowCount(library.size());
    shows.reserve(library.size());
    int idx = 0;
    for (auto&& it : library) {
        auto item0 = new QTableWidgetItem(QString::fromStdString(it.first.generic_u8string()));
        item0->setFlags(item0->flags() ^ Qt::ItemIsEditable);
        ui.showsTable->setItem(idx, 0, item0);

        auto item1 = new QLineEdit(ui.showsTable);
        item1->setValidator(new QIntValidator());
        ui.showsTable->setCellWidget(idx, 2, item1);

        auto item2 = new QTableWidgetItem(QString::fromStdString(cfg->get_shows_name(it.first.filename().generic_u8string())));
        ui.showsTable->setItem(idx, 1, item2);

        auto item3 = new QTableWidgetItem();
        item3->setFlags(item3->flags() ^ Qt::ItemIsEditable);
        ui.showsTable->setItem(idx, 3, item3);

        auto item4 = new QComboBox();
        item4->addItem(QString("tv"));
        item4->addItem(QString("movie"));
        ui.showsTable->setCellWidget(idx, 4, item4);

        auto item5 = new QTableWidgetItem();
        item5->setFlags(item5->flags() ^ Qt::ItemIsEditable);
        ui.showsTable->setItem(idx, 5, item5);

        shows.emplace_back(it.first);
        ++idx;
    }
    // ui.showsTable->resizeColumnsToContents();
    // ui.showsTable->resizeRowsToContents();
}

Q_INVOKABLE void fetch_shows::thread_return(QString result, int row, QString title, bool type, QString overview) {
    ((QLineEdit*)ui.showsTable->cellWidget(row, 2))->setText(std::move(result));
    ui.showsTable->item(row, 3)->setText(std::move(title));
    ((QComboBox*)ui.showsTable->cellWidget(row, 4))->setCurrentIndex(type);
    ui.showsTable->item(row, 5)->setText(std::move(overview));
    if (!QThreadPool::globalInstance()->activeThreadCount()) {
        // ui.showsTable->resizeColumnsToContents();
        // ui.showsTable->resizeRowsToContents();
        all_setEnable(true);
        QMessageBox::information(ui.Widget1, "信息", "已完成抓取", QMessageBox::Ok, QMessageBox::Ok);
    }
}

Q_INVOKABLE void fetch_shows::write_return() {
    if (!QThreadPool::globalInstance()->activeThreadCount()) {
        // ui.showsTable->resizeColumnsToContents();
        // ui.showsTable->resizeRowsToContents();
        all_setEnable(true);
        QMessageBox::information(ui.Widget1, "信息", "已完成写入", QMessageBox::Ok, QMessageBox::Ok);
    }
}

Q_INVOKABLE int fetch_shows::cover_check(const QString& title, const QString& content) {
    return ui.CoverCheck->isChecked() ? QMessageBox::warning(this, title, content, QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes) : QMessageBox::Yes;
}

fetch_shows::~fetch_shows() {}

void fetch_shows::SearchSelected_Clicked() {
    const auto& index = ui.showsTable->selectionModel()->selectedRows();
    if (index.empty()) return;
    all_setEnable(false);
    for (auto&& it : index) {
        std::string search = ui.showsTable->item(it.row(), 1)->text().toStdString();
        search_thread* t = new search_thread(std::move(search), it.row(), cfg, this);
        QThreadPool::globalInstance()->start(t);
    }
}

void fetch_shows::all_setEnable(bool status) {
    ui.Widget1->setEnabled(status);
}

void fetch_shows::UpdateSelected_Clicked() {
    const auto& index = ui.showsTable->selectionModel()->selectedRows();
    if (index.empty()) return;
    all_setEnable(false);
    for (auto&& it : index) {
        const std::string search = ui.showsTable->item(it.row(), 1)->text().toStdString();
        QLineEdit* edit = (QLineEdit*)ui.showsTable->cellWidget(it.row(), 2);
        QComboBox* type = (QComboBox*)ui.showsTable->cellWidget(it.row(), 4);
        update_thread* t = new update_thread(edit->text().toInt(), it.row(), type->currentIndex(), cfg, this);
        QThreadPool::globalInstance()->start(t);
    }
}

void fetch_shows::WriteSelected_Clicked() {
    all_setEnable(false);
    bool called = true;
    const auto& index = ui.showsTable->selectionModel()->selectedRows();
    for (auto&& it : index) {
        QString edit = ((QLineEdit*)ui.showsTable->cellWidget(it.row(), 2))->text();
        if (edit.isEmpty()) {
            spdlog::warn("缺少 TMDB ID，项目：{}", shows[it.row()].generic_u8string());
            continue;
        }
        called = false;
        QComboBox* type = (QComboBox*)ui.showsTable->cellWidget(it.row(), 4);
        write_thread* t = new write_thread(edit.toInt(), type->currentIndex(), shows[it.row()], cfg, this);
        QThreadPool::globalInstance()->start(t);
    }
    if (called) all_setEnable(true);
}

void fetch_shows::RightMenu_Clicked(const QPoint& point) {
    menu->exec(QCursor::pos());
}

void fetch_shows::RightMenuAction_Clicked() {
    const auto& index = ui.showsTable->selectionModel()->selectedRows();
    for (auto&& it : index) {
        QDesktopServices::openUrl(QUrl::fromLocalFile(ui.showsTable->item(it.row(), 0)->text()));
    }
}

void fetch_shows::DialogSearchSelected_Clicked() {
    const auto& index = ui.showsTable->selectionModel()->selectedRows();
    for (auto&& it : index) {
        const std::string search = ui.showsTable->item(it.row(), 1)->text().toStdString();
        const auto& [id, type, name, overview] = search_dialog::search_tvshow(cfg, ui.Widget1, search.c_str());
        ((QLineEdit*)ui.showsTable->cellWidget(it.row(), 2))->setText(id ? QString::number(id) : QString());
        ui.showsTable->item(it.row(), 3)->setText(name);
        ((QComboBox*)ui.showsTable->cellWidget(it.row(), 4))->setCurrentIndex(type);
        ui.showsTable->item(it.row(), 5)->setText(overview);
    }
}

void fetch_shows::Cell_DoubleClicked(int row, int column) {
    const std::string search = ui.showsTable->item(row, 1)->text().toStdString();
    const auto& [id, type, name, overview] = search_dialog::search_tvshow(cfg, ui.Widget1, search.c_str());
    ((QLineEdit*)ui.showsTable->cellWidget(row, 2))->setText(id ? QString::number(id) : QString());
    ui.showsTable->item(row, 3)->setText(name);
    ((QComboBox*)ui.showsTable->cellWidget(row, 4))->setCurrentIndex(type);
    ui.showsTable->item(row, 5)->setText(overview);
}

fetch_shows::search_thread::search_thread(std::string&& n, int r, config* c, QObject* object) {
    name = std::move(n), row = r, cfg = c, obj = object;
}

void fetch_shows::search_thread::run() {
    char* url = curl_escape(name.c_str(), name.size());
    std::string requrl = std::string("https://api.themoviedb.org/3/search/multi?query=") + url + "&include_adult=false&language=zh-CN&page=1";
    curl_free(url);
    std::string reqdata = request(requrl.c_str(), cfg);
    if (reqdata.empty()) {
        QMetaObject::invokeMethod(obj, "thread_return", Q_ARG(QString, QString()), Q_ARG(int, row), Q_ARG(QString, QString()), Q_ARG(bool, false), Q_ARG(QString, QString()));
        return;
    }

    using namespace rapidjson;
    Document json;
    if (json.Parse(reqdata.data()).HasParseError()) {
        spdlog::error("获取到的 JSON 解析错误 数据：{}", reqdata);
        QMetaObject::invokeMethod(obj, "thread_return", Q_ARG(QString, QString()), Q_ARG(int, row), Q_ARG(QString, QString()), Q_ARG(bool, false), Q_ARG(QString, QString()));
        return;
    }

    QString id("-1"), title, overview;
    bool type = false;

    if (!json.HasMember("results") || json["results"].Empty()) {
        spdlog::warn("什么也没搜到 标题：{}", name);
        QMetaObject::invokeMethod(obj, "thread_return", Q_ARG(QString, QString()), Q_ARG(int, row), Q_ARG(QString, QString()), Q_ARG(bool, false), Q_ARG(QString, QString()));
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

    QMetaObject::invokeMethod(obj, "thread_return", Q_ARG(QString, id), Q_ARG(int, row), Q_ARG(QString, std::move(title)), Q_ARG(bool, type), Q_ARG(QString, std::move(overview)));
    return;
}

fetch_shows::update_thread::update_thread(int i, int r, bool t, config* c, QObject* object) {
    id = i, row = r, type = t, cfg = c, obj = object;
}

void fetch_shows::update_thread::run() {
    if (!id) {
        spdlog::warn("第 {} 行 ID 为空", row);
        QMetaObject::invokeMethod(obj, "thread_return", Q_ARG(QString, QString()), Q_ARG(int, row), Q_ARG(QString, QString()), Q_ARG(bool, false), Q_ARG(QString, QString()));
        return;
    }
    const std::string requrl = std::string("https://api.themoviedb.org/3/") + (type ? "movie/" : "tv/") + std::to_string(id) + "?language=zh-CN";
    std::string reqdata = request(requrl.c_str(), cfg);
    if (reqdata.empty()) {
        QMetaObject::invokeMethod(obj, "thread_return", Q_ARG(QString, QString()), Q_ARG(int, row), Q_ARG(QString, QString()), Q_ARG(bool, false), Q_ARG(QString, QString()));
        return;
    }

    using namespace rapidjson;
    Document json;
    if (json.Parse(reqdata.data()).HasParseError()) {
        spdlog::error("获取到的 JSON 解析错误 数据：{}", reqdata);
        QMetaObject::invokeMethod(obj, "thread_return", Q_ARG(QString, QString()), Q_ARG(int, row), Q_ARG(QString, QString()), Q_ARG(bool, false), Q_ARG(QString, QString()));
        return;
    }

    QString i = QString::number(id), title, overview;
    if (type) title = json["title"].GetString();
    else title = json["name"].GetString();
    overview = json["overview"].GetString();
    QMetaObject::invokeMethod(obj, "thread_return", Q_ARG(QString, i), Q_ARG(int, row), Q_ARG(QString, std::move(title)), Q_ARG(bool, type), Q_ARG(QString, std::move(overview)));
    return;
}

fetch_shows::write_thread::write_thread(int i, bool t, path p, config* c, QObject* object) {
    id = i, pth = p, type = t, cfg = c, obj = object;
}

void fetch_shows::write_thread::run() {
    using namespace rapidjson;
    const std::string id_str = std::to_string(id);
    std::string requrl = std::string("https://api.themoviedb.org/3/") + (type ? "movie/" : "tv/") + id_str + "?language=zh-CN";
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
    requrl = std::string("https://api.themoviedb.org/3/") + (type ? "movie/" : "tv/") + id_str + "/keywords";
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
    requrl = std::string("https://api.themoviedb.org/3/") + (type ? "movie/" : "tv/") + id_str + (type ? "/credits?language=zh-CN" : "/aggregate_credits?language=zh-CN");
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
    xml_node tvshow = doc.append_child("tvshow");
    tvshow.append_child("title").append_child(node_pcdata).set_value(details[type ? "title" : "name"].GetString());
    tvshow.append_child("originaltitle").append_child(node_pcdata).set_value(details[type ? "original_title" : "original_name"].GetString());
    tvshow.append_child("showtitle").append_child(node_pcdata).set_value(details[type ? "original_title" : "original_name"].GetString());
    tvshow.append_child("plot").append_child(node_pcdata).set_value(details["overview"].GetString());
    tvshow.append_child("premiered").append_child(node_pcdata).set_value(details[type ? "release_date" : "first_air_date"].GetString());
    tvshow.append_child("status").append_child(node_pcdata).set_value(details["status"].GetString());
    tvshow.append_child("episodeguide").append_child(node_pcdata).set_value(("{\"tmdb\": \"" + id_str + "\"}").c_str());
    xml_node ratings = tvshow.append_child("ratings").append_child("rating");
    ratings.append_attribute("name").set_value("themoviedb");
    ratings.append_attribute("max").set_value("10");
    ratings.append_attribute("default").set_value("yes");
    ratings.append_child("value").append_child(node_pcdata).set_value(std::to_string(details["vote_average"].GetFloat()).c_str());
    ratings.append_child("votes").append_child(node_pcdata).set_value(std::to_string(details["vote_count"].GetInt64()).c_str());

    xml_node uniqueid = tvshow.append_child("uniqueid");
    uniqueid.append_child(node_pcdata).set_value(id_str.c_str());
    uniqueid.append_attribute("type").set_value("tmdb");
    uniqueid.append_attribute("default").set_value("true");

    for (auto&& it : details["genres"].GetArray())
        tvshow.append_child("genre").append_child(node_pcdata).set_value(it.GetObj()["name"].GetString());
    for (auto&& it : keywords[type ? "keywords" : "results"].GetArray())
        tvshow.append_child("tag").append_child(node_pcdata).set_value(it.GetObj()["name"].GetString());
    for (auto&& it : details["production_companies"].GetArray())
        tvshow.append_child("studio").append_child(node_pcdata).set_value(it.GetObj()["name"].GetString());
    if (!type) {
        for (auto&& it : details["networks"].GetArray())
            tvshow.append_child("studio").append_child(node_pcdata).set_value(it.GetObj()["name"].GetString());
    }

    if (!type) {
        int idx = 0;
        for (auto&& it : details["seasons"].GetArray()) {
            xml_node namedseason = tvshow.append_child("namedseason");
            namedseason.append_child(node_pcdata).set_value(it.GetObj()["name"].GetString());
            namedseason.append_attribute("number").set_value(std::to_string(++idx).c_str());
        }
    }

    xml_node art = tvshow.append_child("art");
    const std::string img_site("https://image.tmdb.org/t/p/original");

    if (details["poster_path"].IsString()) {
        xml_node thumb1 = tvshow.append_child("thumb");
        thumb1.append_child(node_pcdata).set_value((img_site + details["poster_path"].GetString()).c_str());
        thumb1.append_attribute("aspect").set_value("poster");
        art.append_child("poster").append_child(node_pcdata).set_value((img_site + details["poster_path"].GetString()).c_str());
    }

    if (details["backdrop_path"].IsString()) {
        xml_node thumb2 = tvshow.append_child("thumb");
        thumb2.append_child(node_pcdata).set_value((img_site + details["backdrop_path"].GetString()).c_str());
        thumb2.append_attribute("aspect").set_value("banner");
        art.append_child("poster").append_child(node_pcdata).set_value((img_site + details["backdrop_path"].GetString()).c_str());
    }

    for (auto&& it : credits["cast"].GetArray()) {
        const auto&& obj = it.GetObj();
        xml_node actor = tvshow.append_child("actor");

        actor.append_child("name").append_child(node_pcdata).set_value(obj["name"].GetString());
        if (obj["profile_path"].IsString())
            actor.append_child("thumb").append_child(node_pcdata).set_value((img_site + obj["profile_path"].GetString()).c_str());
        actor.append_child("order").append_child(node_pcdata).set_value(std::to_string(obj["order"].GetInt()).c_str());
        if (type) actor.append_child("role").append_child(node_pcdata).set_value(obj["character"].GetString());
        else actor.append_child("role").append_child(node_pcdata).set_value(obj["roles"][0].GetObj()["character"].GetString());
    }

    const path write = pth / "tvshow.nfo";

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

void fetch_shows::Next_Clicked() {
    spdlog::info("第三阶段结束");
    close();
    std::vector<std::tuple<QString, QString, bool> > ids;
    ids.reserve(library.size());
    for (size_t i = 0; i < library.size(); ++i)
        ids.emplace_back(((QLineEdit*)(ui.showsTable->cellWidget(i, 2)))->text(), ui.showsTable->item(i, 3)->text(), ((QComboBox*)(ui.showsTable->cellWidget(i, 4)))->currentIndex());
    next_window->show();
    next_window->set_library(std::move(library), std::move(ids));
    destroy();
}