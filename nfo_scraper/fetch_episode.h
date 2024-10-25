#pragma once

#include <QMainWindow>
#include <QThreadPool>
#include <filesystem>
#include <fstream>
#include <iostream>

#include "config.h"
#include "ui_fetch_episode.h"

class fetch_episode : public window_init_with_data {
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
    fetch_episode(config* cfg, window_init_with_data* next_window, QWidget* parent = nullptr);
    ~fetch_episode();
    void init(void* pointer) override;

private:
    class update_thread : public QRunnable {
    public:
        update_thread(int id, bool type, int season, vec_remotes* remotes, int row, config* cfg, QObject* object);

    private:
        int id, season;
        bool type;
        int row;
        vec_remotes* remotes;
        QObject* obj;
        config* cfg;

    protected:
        void run();
    };
    class write_thread : public QRunnable {
    public:
        write_thread(int id, bool type, int season, int episode, fs_path path, config* cfg, QObject* object, seasons_directories* seasons);

    private:
        int id, season, episode;
        bool type;
        int row;
        fs_path path;
        vec_remotes* vec;
        QObject* obj;
        config* cfg;
        std::mutex lock;
        seasons_directories* seasons_path;

    protected:
        void run();
        void write_nfo();
        void write_strm();
    };
    vec_paths path;
    std::vector<vec_paths> local_video;
    std::vector<vec_remotes> remote_data;
    vec_paths addition_folder;
    std::vector<std::tuple<QString, QString, QString> > names;
    seasons_directories seasons_path;
    library_directories duplicate_path;
    config* cfg;
    window_init_with_data* next_window;
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
