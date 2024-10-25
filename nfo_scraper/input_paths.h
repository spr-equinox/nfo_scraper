#pragma once

#include <QStringListModel>
#include <QtWidgets/QMainWindow>

#include "tools.h"
#include "ui_input_paths.h"

class input_paths : public QMainWindow {
    Q_OBJECT

public:
    input_paths(window_init_with_data *next_window, QWidget *parent = nullptr);
    ~input_paths();

private:
    Ui::input_pathsClass ui;
    window_init_with_data *next_window;

private slots:
    void Add_Clicked();
    void Remove_Clicked();
    void Next_Clicked();
    void Setting_Clicked();
};
