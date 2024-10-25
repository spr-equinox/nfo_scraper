#pragma once
#include <QDialog>

#include "config.h"
#include "tools.h"
#include "ui_search_dialog.h"
class search_dialog : public QDialog {
    Q_OBJECT

public:
    search_dialog(config* cfg, QWidget* parent = nullptr);
    ~search_dialog();

    // id type name overview
    static std::tuple<int, bool, QString, QString> search_tvshow(config* cfg, QWidget* parent = nullptr, const char* str = nullptr);

private:
    config* cfg;
    Ui_SearchDialog ui;
    std::vector<std::tuple<int, bool, QString, QString> > results;
    int result;
private slots:
    void SearchButton_Clicked();
    void ConfirmButton_Clicked();
    void Cell_DoubleClicked(int row, int column);
};
