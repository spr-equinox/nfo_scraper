#pragma once

#include <QStringListModel>
#include <QtWidgets/QMainWindow>
#include <filesystem>

#include "ui_write_ignore.h"

class write_ignore : public QMainWindow {
    Q_OBJECT

public:
    using path = std::filesystem::path;
    using vec_paths = std::vector<path>;
    write_ignore(QWidget* parent = nullptr);
    ~write_ignore();
    void set_library(vec_paths&& data);

private:
    Ui::write_ignoreClass ui;

private slots:
    void Add_Clicked();
    void Remove_Clicked();
    void Finished_Clicked();
    void CreateIgnore_Clicked();
};
