#pragma once

#include <QDragEnterEvent>
#include <QDropEvent>
#include <QListWidget>
#include <QMimeData>

class QListWidgetDragDrop : public QListWidget {
    Q_OBJECT

public:
    QListWidgetDragDrop(QWidget* parent);
    ~QListWidgetDragDrop() = default;

protected:
    void dragEnterEvent(QDragEnterEvent* event);
    void dropEvent(QDropEvent* event);
};