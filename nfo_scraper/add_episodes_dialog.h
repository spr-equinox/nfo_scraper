#pragma once
#include <QDialog>

#include "config.h"
#include "fetch_episode.h"
#include "fetch_seasons.h"
#include "tools.h"
#include "ui_add_episodes_dialog.h"

class add_episodes_dialog : public QDialog {
    Q_OBJECT

public:
    add_episodes_dialog(int id, config* cfg, QWidget* parent);
    ~add_episodes_dialog();

    // id type name overview
    static fetch_episode::vec_remotes add_episodes(config* cfg, QWidget* parent = nullptr, const char* str = nullptr);

private:
    int id;
    bool accepted;
    config* cfg;
    Ui_AddEpisodesDialog ui;
    fetch_episode::vec_remotes epis;
    std::vector<int> seasons;
private slots:
    void GetButton_Clicked();
    void ConfirmButton_Clicked();
};