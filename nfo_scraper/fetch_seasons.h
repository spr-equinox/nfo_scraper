#pragma once

#include <QMainWindow>
#include <QMenu>
#include <QThreadPool>
#include <filesystem>

#include "config.h"
#include "ui_fetch_seasons.h"

class fetch_seasons : public window_init_with_data {
    Q_OBJECT

public:
    fetch_seasons(config* cfg, window_init_with_data* next_window, QWidget* parent = nullptr);
    void init(void* pointer) override;
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
        search_thread(std::string&& name, int row, config* cfg, QObject* object);

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
        update_thread(int id, int row, bool type, config* cfg, QObject* object);

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
        write_thread(int id, int season_number, bool type, fs_path path, config* cfg, QObject* object, shows_directories* shows, seasons_directories* seasons);

    private:
        int id, season_number;
        bool type;
        fs_path path;
        QObject* obj;
        config* cfg;
        shows_directories* shows;
        seasons_directories* seasons;
        std::mutex lock;

    protected:
        void run();
        void write_nfo();
        void write_strm();
    };
    std::vector<std::pair<fs_path, vec_data> > seasons;  // number name overview
    std::map<void*, int> cell_pos;
    library_directories library;
    shows_directories shows_path;
    seasons_directories seasons_path;
    Ui::fetch_seasonsClass ui;
    config* cfg;
    QMenu* menu;
    window_init_with_data* next_window;
    int running;
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
