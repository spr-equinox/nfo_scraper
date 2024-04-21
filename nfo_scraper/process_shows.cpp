#include "process_shows.h"

#include <spdlog/spdlog.h>

#include <QFileDialog>
#include <vector>

process_shows::process_shows(config* c, fetch_shows* w, QWidget* parent)
    : QMainWindow(parent) {
    cfg = c, next_window = w;
    ui.setupUi(this);
    connect(ui.Next, SIGNAL(clicked()), this, SLOT(Next_Clicked()));
    connect(ui.CreateIgnore, SIGNAL(clicked()), this, SLOT(CreateIgnore_Clicked()));
    connect(ui.Expand, SIGNAL(clicked()), ui.libraryTree, SLOT(expandAll()));
    connect(ui.Collapse, SIGNAL(clicked()), ui.libraryTree, SLOT(collapseAll()));
}

void process_shows::set_search_paths(vec_paths&& data) {
    search_paths = std::move(data);
}

void process_shows::set_up() {
    search = new search_thread(this);
    connect(search, SIGNAL(finished()), this, SLOT(search_finished()));
    search->start();
}

process_shows::~process_shows() {
}

void process_shows::dfs(const path& now) {
    if (std::filesystem::exists(now / ".ignore")) {
        spdlog::info("路径 {} 被忽略", now.generic_u8string());
        ignore_paths.emplace_back(now);
        return;
    }
    for (const auto& it : std::filesystem::directory_iterator(now)) {
        if (it.is_regular_file() && cfg->check_ext(it.path().extension().generic_u8string())) {
            const auto &parent = now.parent_path(), &filename = now.filename();
            spdlog::info("找到媒体文件夹：{}", now.generic_u8string());
            if (auto tmp = library.find(parent); tmp != library.end())
                tmp->second.emplace_back(filename);
            else library.emplace(parent, vec_paths{filename});
            return;
        }
    }
    for (const auto& it : std::filesystem::directory_iterator(now)) {
        if (!it.is_directory()) continue;
        const auto& pth = it.path();
        if (cfg->check_ignore(pth.filename().generic_u8string())) {
            spdlog::info("路径 {} 被忽略", pth.generic_u8string());
            ignore_paths.emplace_back(pth);
            continue;
        }
        dfs(pth);
    }
}

void process_shows::Next_Clicked() {
    spdlog::info("第二阶段结束");
    close();
    next_window->set_library(std::move(library));
    next_window->show();
    destroy();
}

void process_shows::CreateIgnore_Clicked() {
    ui.Widget1->setEnabled(false);
    for (auto&& it : ui.ignoreList->selectedItems()) {
        path p(it->text().toStdWString());
        p /= ".ignore";
        if (std::filesystem::exists(p))
            spdlog::info("文件 {} 已存在，跳过", p.generic_u8string());
        else {
            std::FILE* f = nullptr;
            errno_t error_code = _wfopen_s(&f, p.generic_wstring().c_str(), L"w");
            if (error_code) {
                spdlog::error("文件 {} 创建失败 错误信息：{}", p.generic_u8string(), std::strerror(error_code));
                continue;
            }
            spdlog::info("已创建文件 {} ", p.generic_u8string());
            if (f) fclose(f);  // 如果没有 if 会 warning
        }
        QApplication::processEvents();
    }
    ui.Widget1->setEnabled(true);
}

void process_shows::search_finished() {
    for (auto&& it : ignore_paths)
        ui.ignoreList->addItem(QString::fromStdString(it.generic_u8string()));
    for (auto&& it : library) {
        QTreeWidgetItem* new_top_item = new QTreeWidgetItem(QStringList{QString::fromStdString(it.first.u8string())});
        ui.libraryTree->addTopLevelItem(new_top_item);
        for (auto&& c : it.second)
            new_top_item->addChild(new QTreeWidgetItem(QStringList{QString::fromStdString(c.u8string())}));
    }
    ui.Waiting->setText(QCoreApplication::translate("process_showsClass", "已完成", nullptr));
    ui.Next->setEnabled(true);
    ui.CreateIgnore->setEnabled(true);
    ui.libraryTree->expandAll();
}

process_shows::search_thread::search_thread(process_shows* that) {
    ths = that;
}

void process_shows::search_thread::run() {
    for (auto&& it : ths->search_paths) {
        it = it.make_preferred();
        for (const auto& ch : std::filesystem::directory_iterator(it)) {
            if (!ch.is_directory()) continue;
            ths->dfs(ch.path());
        }
    }
}
