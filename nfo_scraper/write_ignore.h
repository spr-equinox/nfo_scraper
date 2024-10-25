#pragma once

#include <QStringListModel>
#include <QMainWindow>
#include <filesystem>

#include "ui_write_ignore.h"
#include "tools.h"

class write_ignore : public window_init_with_data {
    Q_OBJECT

public:
    write_ignore(QWidget* parent = nullptr);
    ~write_ignore();
    void init(void* pointer) override;
    private:
    Ui::write_ignoreClass ui;

private slots:
    void Add_Clicked();
    void Remove_Clicked();
    void Finished_Clicked();
    void CreateIgnore_Clicked();
};
