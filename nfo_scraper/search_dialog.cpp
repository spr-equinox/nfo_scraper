#include "search_dialog.h"

#include <curl/curl.h>
#include <rapidjson/document.h>
#include <spdlog/spdlog.h>

#include <QMessageBox>

search_dialog::search_dialog(config* cfg, QWidget* parent)
    : QDialog(parent), cfg(cfg), result(-1) {
    ui.setupUi(this);
    ui.tableWidget->horizontalHeader()->sectionResizeMode(QHeaderView::Fixed);
    ui.tableWidget->verticalHeader()->sectionResizeMode(QHeaderView::Fixed);
    connect(ui.SearchButton, SIGNAL(clicked()), this, SLOT(SearchButton_Clicked()));
    connect(ui.ConfirmButton, SIGNAL(clicked()), this, SLOT(ConfirmButton_Clicked()));
    connect(ui.tableWidget, SIGNAL(cellDoubleClicked(int, int)), this, SLOT(Cell_DoubleClicked(int, int)));
}

search_dialog::~search_dialog() {
}

std::tuple<int, bool, QString, QString> search_dialog::search_tvshow(config* cfg, QWidget* parent, const char* str) {
    search_dialog dialog(cfg, parent);
    if (str) dialog.ui.TextEdit->setText(str);
    dialog.exec();
    return ~dialog.result ? dialog.results[dialog.result] : std::tuple<int, bool, QString, QString>();
}

void search_dialog::ConfirmButton_Clicked() {
    const int row = ui.tableWidget->currentRow();
    if (!~row) QMessageBox::warning(this, "警告", "请选择项目", QMessageBox::Ok, QMessageBox::Ok);
    else {
        result = row;
        close();
    }
}

void search_dialog::Cell_DoubleClicked(int row, int column) {
    result = row;
    close();
}

void search_dialog::SearchButton_Clicked() {
    ui.tableWidget->setRowCount(0);
    results.clear();
    const auto& name = ui.TextEdit->text().toStdString();
    char* url = curl_escape(name.c_str(), name.size());
    std::string requrl = std::string("https://api.themoviedb.org/3/search/multi?query=") + url + "&include_adult=false&language=zh-CN&page=1";
    curl_free(url);
    std::string reqdata = request(requrl.c_str(), cfg);
    if (reqdata.empty()) return;

    using namespace rapidjson;
    Document json;
    if (json.Parse(reqdata.data()).HasParseError()) {
        spdlog::error("获取到的 JSON 解析错误 数据：{}", reqdata);
        return;
    }

    if (!json.HasMember("results") || json["results"].Empty()) {
        spdlog::warn("什么也没搜到 标题：{}", name.data());
        return;
    } else
        for (auto&& it : json["results"].GetArray()) {
            auto&& tmp = it.GetObj();
            if (!std::strcmp(tmp["media_type"].GetString(), "tv"))
                results.emplace_back(tmp["id"].GetInt(), false, tmp["name"].GetString(), tmp["overview"].GetString());
            else if (!std::strcmp(tmp["media_type"].GetString(), "movie"))
                results.emplace_back(tmp["id"].GetInt(), true, tmp["title"].GetString(), tmp["overview"].GetString());
            else spdlog::warn("未知的类型：{}", tmp["media_type"].GetString());
        }
    ui.tableWidget->setRowCount(results.size());
    int idx = 0;
    for (auto&& [id, type, name, overview] : results) {
        ui.tableWidget->setItem(idx, 0, new QTableWidgetItem(name));
        ui.tableWidget->setItem(idx, 1, new QTableWidgetItem(QString::number(id)));
        ui.tableWidget->setItem(idx, 2, new QTableWidgetItem(type ? "电影" : "剧集"));
        ui.tableWidget->setItem(idx, 3, new QTableWidgetItem(overview));
        ++idx;
    }
}
