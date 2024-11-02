#include "process_shows.h"

#include <spdlog/spdlog.h>

#include <QFileDialog>
#include <vector>

process_shows::process_shows(config* cfg, window_init_with_data* next_window, QWidget* parent)
    : window_init_with_data(parent), cfg(cfg), next_window(next_window) {
    ui.setupUi(this);
    connect(ui.Next, SIGNAL(clicked()), this, SLOT(Next_Clicked()));
    connect(ui.CreateIgnore, SIGNAL(clicked()), this, SLOT(CreateIgnore_Clicked()));
    connect(ui.Expand, SIGNAL(clicked()), ui.libraryTree, SLOT(expandAll()));
    connect(ui.Collapse, SIGNAL(clicked()), ui.libraryTree, SLOT(collapseAll()));
}

void process_shows::init(void *pointer) {
    search_paths = std::move(*(vec_paths*)pointer);
    delete (vec_paths*)pointer;
    show();
    search = new search_thread(this);
    connect(search, SIGNAL(finished()), this, SLOT(search_finished()));
    search->start();
}

process_shows::~process_shows() {
}

void process_shows::dfs(const fs_path& now) {
    if (std::filesystem::exists(now / ".ignore")) {
        spdlog::info("路径 {} 被忽略", now.generic_u8string());
        ignore_paths.emplace_back(now);
        return;
    }
    bool found = false;
    for (const auto& it : std::filesystem::directory_iterator(now)) {
        if (!it.is_directory() && cfg->check_ext(it.path().extension().generic_u8string())) {
            found = true;
            if (exist_path.count(it.path())) continue;
            const auto &parent = now.parent_path(), &filename = now.filename();
            spdlog::info("找到媒体文件夹：{}", now.generic_u8string());
            if (auto tmp = library.find(parent); tmp != library.end())
                tmp->second.emplace_back(filename);
            else library.emplace(parent, vec_paths{filename});
            return;
        }
    }
    if (found) {
        spdlog::info("媒体文件夹 {} 中的所有文件已链接，跳过", now.generic_u8string());
        return;
    }
    for (const auto& it : std::filesystem::directory_iterator(now)) {
        if (!it.is_directory()) continue;
        const auto& path = it.path();
        if (cfg->check_ignore(path.filename().generic_u8string())) {
            spdlog::info("路径 {} 被忽略", path.generic_u8string());
            ignore_paths.emplace_back(path);
            continue;
        }
        dfs(path);
    }
}

void process_shows::Next_Clicked() {
    spdlog::info("第二阶段结束");
    close();
    next_window->init(new library_directories(std::move(library)));
    destroy();
}

void process_shows::CreateIgnore_Clicked() {
    ui.Widget1->setEnabled(false);
    for (auto&& it : ui.ignoreList->selectedItems()) {
        fs_path p(it->text().toStdWString());
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
        ui.ignoreList->addItem(QString::fromStdWString(it.generic_wstring()));
    for (auto&& it : library) {
        QTreeWidgetItem* new_top_item = new QTreeWidgetItem(QStringList{QString::fromStdWString(it.first.generic_wstring())});
        ui.libraryTree->addTopLevelItem(new_top_item);
        for (auto&& sub : it.second)
            new_top_item->addChild(new QTreeWidgetItem(QStringList{QString::fromStdWString(sub.generic_wstring())}));
    }
    ui.Waiting->setText(QCoreApplication::translate("process_showsClass", "已完成", nullptr));
    ui.Next->setEnabled(true);
    if (cfg->get_save_type() == 1) ui.CreateIgnore->setEnabled(true);
    ui.libraryTree->expandAll();
}

process_shows::search_thread::search_thread(process_shows* self) : self(self) {}

void process_shows::search_thread::run() {
    if (self->cfg->get_save_type() == 2 && self->cfg->is_incremental_update()) {
        for (const auto& ch : std::filesystem::recursive_directory_iterator(self->cfg->get_save_path())) {
            if (ch.is_directory() || ch.path().extension() != L".strm") continue;
            std::ifstream f(ch.path());
            std::string str;
            str.resize(ch.file_size());
            f.read(str.data(), str.size());
            if (f.bad()) {
                spdlog::error("读取文件 {} 时发生了 I/O 错误", ch.path().generic_u8string());
                continue;
            }
            self->exist_path.emplace(utf8_to_wchar(str.c_str(), str.size()));
        }
    }
    for (auto&& it : self->search_paths) {
        it = it.make_preferred();
        for (const auto& ch : std::filesystem::directory_iterator(it)) {
            if (!ch.is_directory()) continue;
            self->dfs(ch.path());
        }
    }
}
