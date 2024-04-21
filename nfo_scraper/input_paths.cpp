#include "input_paths.h"

#include <spdlog/spdlog.h>

#include <QFileDialog>
#include <filesystem>
#include <vector>

input_paths::input_paths(process_shows* w, QWidget* parent)
    : QMainWindow(parent) {
    next_window = w;
    ui.setupUi(this);
    connect(ui.Add, SIGNAL(clicked()), this, SLOT(Add_Clicked()));
    connect(ui.Remove, SIGNAL(clicked()), this, SLOT(Remove_Clicked()));
    connect(ui.Next, SIGNAL(clicked()), this, SLOT(Next_Clicked()));
    // connect(ui.Setting, SIGNAL(clicked()), this, SLOT(Setting_Clicked()));
}

input_paths::~input_paths() {}

void input_paths::Add_Clicked() {
    QFileDialog* open = new QFileDialog(ui.Widget1);
    open->setAcceptMode(QFileDialog::AcceptOpen);
    open->setFileMode(QFileDialog::Directory);
    if (open->exec() != QDialog::Accepted) {
        spdlog::info("用户取消操作");
        return;
    }
    for (auto&& it : open->selectedFiles()) {
        spdlog::info("添加文件夹：{}", it.toStdString());
        ui.folderList->addItem(it);
    }
}

void input_paths::Remove_Clicked() {
    for (auto&& it : ui.folderList->selectedItems()) {
        spdlog::info("移除文件夹：{}", it->text().toStdString());
        delete it;
    }
}

void input_paths::Next_Clicked() {
    std::vector<std::filesystem::path> search_paths;
    const auto cnt = ui.folderList->count();
    search_paths.reserve(cnt);
    for (int i = 0; i < cnt; ++i)
        search_paths.emplace_back(ui.folderList->item(i)->text().toStdWString());
    spdlog::info("第一阶段结束");
    close();
    next_window->set_search_paths(std::move(search_paths));
    next_window->show();
    next_window->set_up();
    destroy();
}

void input_paths::Setting_Clicked() {
    // TODO
}
