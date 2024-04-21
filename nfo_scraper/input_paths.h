#pragma once

#include <QStringListModel>
#include <QtWidgets/QMainWindow>

#include "process_shows.h"
#include "ui_input_paths.h"

class input_paths : public QMainWindow {
    Q_OBJECT

public:
    input_paths(process_shows *w, QWidget *parent = nullptr);
    ~input_paths();

private:
    Ui::input_pathsClass ui;
    process_shows *next_window;

private slots:
    void Add_Clicked();
    void Remove_Clicked();
    void Next_Clicked();
    void Setting_Clicked();
};
