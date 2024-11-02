#pragma once

#include <QStringListModel>
#include <QThread>
#include <QtWidgets/QMainWindow>
#include <filesystem>
#include <unordered_map>
#include <fstream>

#include "config.h"
#include "ui_process_shows.h"

class process_shows : public window_init_with_data {
    Q_OBJECT

public:
    process_shows(config* cfg, window_init_with_data* next_window, QWidget* parent = nullptr);
    void init(void* pointer) override;
    ~process_shows();

private:
    class search_thread : public QThread {
    public:
        search_thread(process_shows* self);

    private:
        process_shows* self;

    protected:
        void run();
    }* search;
    Ui::process_showsClass ui;
    vec_paths search_paths, ignore_paths;
    library_directories library;
    std::unordered_set<fs_path> exist_path;
    config* cfg;
    window_init_with_data* next_window;
    void dfs(const fs_path& now);
private slots:
    void search_finished();
    void Next_Clicked();
    void CreateIgnore_Clicked();
};
