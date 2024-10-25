#include "write_ignore.h"

#include <spdlog/spdlog.h>

#include <QFileDialog>
#include <filesystem>
#include <vector>

write_ignore::write_ignore(QWidget* parent)
    : window_init_with_data(parent) {
    ui.setupUi(this);
    connect(ui.Add, SIGNAL(clicked()), this, SLOT(Add_Clicked()));
    connect(ui.Remove, SIGNAL(clicked()), this, SLOT(Remove_Clicked()));
    connect(ui.Finished, SIGNAL(clicked()), this, SLOT(Finished_Clicked()));
    connect(ui.CreateIgnore, SIGNAL(clicked()), this, SLOT(CreateIgnore_Clicked()));
}

write_ignore::~write_ignore() {}

void write_ignore::init(void* pointer) {
    show();
    QApplication::processEvents();
    for (auto&& it : *(vec_paths*)pointer) ui.folderList->addItem(QString::fromStdWString(it.generic_wstring()));
    delete (vec_paths*)pointer;
}

void write_ignore::CreateIgnore_Clicked() {
    ui.Widget1->setEnabled(false);
    for (auto&& it : ui.folderList->selectedItems()) {
        fs_path p(it->text().toStdWString());
        p /= ".ignore";
        if (std::filesystem::exists(p)) spdlog::info("文件 {} 已存在，跳过", p.generic_u8string());
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

void write_ignore::Add_Clicked() {
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

void write_ignore::Remove_Clicked() {
    for (auto&& it : ui.folderList->selectedItems()) {
        spdlog::info("移除文件夹：{}", it->text().toStdString());
        delete it;
    }
}

void write_ignore::Finished_Clicked() {
    close();
    spdlog::info("处理完毕 您辛苦了~");
    destroy();
}
