#pragma once

#include <QMainWindow>
#include <QMenu>
#include <QThreadPool>
#include <filesystem>

#include "config.h"
#include "fetch_episode.h"
#include "ui_fetch_seasons.h"

class fetch_seasons : public QMainWindow {
    Q_OBJECT

public:
    using path = std::filesystem::path;
    using vec_paths = std::vector<path>;
    fetch_seasons(config* c, fetch_episode* w, QWidget* parent = nullptr);
    void set_library(std::unordered_map<path, vec_paths>&& source, std::vector<std::tuple<QString, QString, bool> >&& vals);
    ~fetch_seasons();

    struct season_data {
        int id;
        std::string name, overview;
        season_data(int a, std::string b, std::string c) {
            id = a, name = std::move(b), overview = std::move(c);
        }
        season_data() = default;
    };
    using vec_data = std::vector<season_data>;

private:
    class search_thread : public QRunnable {
    public:
        search_thread(const std::string& n, int r, config* c, QObject* object);

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
        write_thread(int i, int s, bool t, path p, config* c, QObject* object);

    private:
        int id, season_num;
        bool type;
        path pth;
        QObject* obj;
        config* cfg;

    protected:
        void run();
    };
    std::vector<std::pair<path, vec_data> > seasons;  // number name overview
    std::map<void*, int> cell_pos;
    std::unordered_map<path, vec_paths> library;
    Ui::fetch_seasonsClass ui;
    config* cfg;
    QMenu* menu;
    fetch_episode* next_window;
    Q_INVOKABLE void thread_return(QString result, int row, QString title, int type, QString overview, vec_data data);
    Q_INVOKABLE void write_return();
    Q_INVOKABLE int cover_check(const QString& title, const QString& content);
    void all_setEnable(bool status);
private slots:
    void SearchSelected_Clicked();
    void UpdateSelected_Clicked();
    void WriteSelected_Clicked();
    void RightMenu_Clicked(const QPoint& point);
    void RightMenuAction_Clicked();
    void Next_Clicked();
    void ID_Changed(const QString& text);
    void Type_Changed(int index);
    void Season_Changed(int index);
    void DialogSearchSelected_Clicked();
    void Cell_DoubleClicked(int row, int column);
};
