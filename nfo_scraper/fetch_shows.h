#pragma once

#include <QMainWindow>
#include <QMenu>
#include <QThreadPool>
#include <filesystem>
#include <mutex>

#include "config.h"
#include "ui_fetch_shows.h"

class fetch_shows : public window_init_with_data {
    Q_OBJECT

public:
    fetch_shows(config* cfg, window_init_with_data* next_window, QWidget* parent = nullptr);
    void init(void* pointer) override;
    ~fetch_shows();

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
        write_thread(int id, bool type, fs_path path, config* cfg, QObject* object, shows_directories* shows);

    private:
        int id;
        bool type;
        fs_path path;
        QObject* obj;
        config* cfg;
        std::mutex lock;
        shows_directories* shows;

    protected:
        void run();
        void write_nfo();
        void write_strm();
    };
    void all_setEnable(bool status);
    library_directories library;
    shows_directories shows_path;
    vec_paths shows;
    Ui::fetch_showsClass ui;
    config* cfg;
    QMenu* menu;
    window_init_with_data* next_window;
    int running;
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
