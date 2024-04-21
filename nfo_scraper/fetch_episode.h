#pragma once

#include <QMainWindow>
#include <QThreadPool>
#include <filesystem>

#include "config.h"
#include "ui_fetch_episode.h"
#include "write_ignore.h"

class fetch_episode : public QMainWindow {
    Q_OBJECT

public:
    struct remote {
        int series_id;
        bool type;
        int season_number, episode_number;
        QString name;
        remote(int id, bool typ, int season, int epi, QString n) {
            series_id = id, type = typ, season_number = season, episode_number = epi, name = std::move(n);
        }
        remote() = default;
    };
    using vec_remotes = std::vector<remote>;
    using path = std::filesystem::path;
    using vec_paths = std::vector<path>;
    fetch_episode(config* c, write_ignore* w, QWidget* parent = nullptr);
    ~fetch_episode();
    void set_library(vec_paths&& p, std::vector<std::tuple<QString, QString, QString> >&& n, std::vector<std::tuple<int, bool, int> >&& data);

private:
    class update_thread : public QRunnable {
    public:
        update_thread(int i, int t, int s, vec_remotes* v, int r, config* c, QObject* object);

    private:
        int id, season;
        bool type;
        int row;
        vec_remotes* vec;
        QObject* obj;
        config* cfg;

    protected:
        void run();
    };
    class write_thread : public QRunnable {
    public:
        write_thread(int i, bool t, int s, int e, path p, config* c, QObject* object);

    private:
        int id, season, episode;
        bool type;
        int row;
        path pth;
        vec_remotes* vec;
        QObject* obj;
        config* cfg;

    protected:
        void run();
    };
    vec_paths pth;
    std::vector<vec_paths> local_video;
    std::vector<vec_remotes> remote_data;
    vec_paths addition_folder;
    std::vector<std::tuple<QString, QString, QString> > names;
    config* cfg;
    write_ignore* next_window;
    Ui::fetch_episodeClass ui;
    int running;

    int cur_row;
    vec_paths cur_local;
    vec_remotes cur_remote;

    Q_INVOKABLE void update_return(int row);
    Q_INVOKABLE void write_return();
    Q_INVOKABLE int cover_check(const QString& title, const QString& content);
private slots:
    void Cell_DoubleClicked(int row, int column);
    void LocalRemove_Clicked();
    void LocalUp_Clicked();
    void LocalDown_Clicked();
    void LocalAdd_Clicked();
    void RemoteRemove_Clicked();
    void RemoteUp_Clicked();
    void RemoteDown_Clicked();
    void RemoteAdd_Clicked();
    void SaveButton_Clicked();
    void WriteButton_Clicked();
    void NextButton_Clicked();
    void BrowseButton_Clicked();
};
