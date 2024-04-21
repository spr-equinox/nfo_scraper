#pragma once

#include <QMainWindow>
#include <QMenu>
#include <QThreadPool>
#include <filesystem>

#include "config.h"
#include "fetch_seasons.h"
#include "ui_fetch_shows.h"

class fetch_shows : public QMainWindow {
    Q_OBJECT

public:
    using path = std::filesystem::path;
    using vec_paths = std::vector<path>;
    fetch_shows(config* c, fetch_seasons* w, QWidget* parent = nullptr);
    void set_library(std::unordered_map<path, vec_paths>&& source);
    ~fetch_shows();

private:
    class search_thread : public QRunnable {
    public:
        search_thread(std::string&& n, int r, config* c, QObject* object);

    private:
        std::string name;
        int row;
        QObject* obj;
        config* cfg;

    protected:
        void run();
    };
    class update_thread : public QRunnable {
    public:
        update_thread(int i, int r, bool t, config* c, QObject* object);

    private:
        int id, row;
        bool type;
        QObject* obj;
        config* cfg;

    protected:
        void run();
    };
    class write_thread : public QRunnable {
    public:
        write_thread(int i, bool t, path p, config* c, QObject* object);

    private:
        int id;
        bool type;
        path pth;
        QObject* obj;
        config* cfg;

    protected:
        void run();
    };
    void all_setEnable(bool status);
    std::unordered_map<path, vec_paths> library;
    vec_paths shows;
    Ui::fetch_showsClass ui;
    config* cfg;
    QMenu* menu;
    fetch_seasons* next_window;
    Q_INVOKABLE void thread_return(QString result, int r, QString title, bool type, QString overview);
    Q_INVOKABLE void write_return();
    Q_INVOKABLE int cover_check(const QString& title, const QString& content);
private slots:
    void SearchSelected_Clicked();
    void UpdateSelected_Clicked();
    void WriteSelected_Clicked();
    void Next_Clicked();
    void RightMenu_Clicked(const QPoint& point);
    void RightMenuAction_Clicked();
    void DialogSearchSelected_Clicked();
    void Cell_DoubleClicked(int row, int column);
};
