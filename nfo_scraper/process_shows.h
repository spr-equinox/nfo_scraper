#pragma once

#include <QStringListModel>
#include <QThread>
#include <QtWidgets/QMainWindow>
#include <filesystem>
#include <unordered_map>

#include "config.h"
#include "fetch_shows.h"
#include "ui_process_shows.h"

class process_shows : public QMainWindow {
    Q_OBJECT

public:
    using path = std::filesystem::path;
    using vec_paths = std::vector<path>;
    process_shows(config* c, fetch_shows* w, QWidget* parent = nullptr);
    void set_search_paths(vec_paths&& data);
    void set_up();
    ~process_shows();

private:
    class search_thread : public QThread {
    public:
        search_thread(process_shows* that);

    private:
        process_shows* ths;

    protected:
        void run();
    }* search;
    Ui::process_showsClass ui;
    vec_paths search_paths, ignore_paths;
    std::unordered_map<path, vec_paths> library;
    config* cfg;
    fetch_shows* next_window;
    void dfs(const path& now);
private slots:
    void search_finished();
    void Next_Clicked();
    void CreateIgnore_Clicked();
};
