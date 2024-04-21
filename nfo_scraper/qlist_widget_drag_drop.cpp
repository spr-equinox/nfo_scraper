#include "qlist_widget_drag_drop.h"

#include <spdlog/spdlog.h>

#include <QFileInfo>
#include <QUrl>

QListWidgetDragDrop::QListWidgetDragDrop(QWidget* parent)
    : QListWidget(parent) {
    this->setAcceptDrops(true);
}

void QListWidgetDragDrop::dragEnterEvent(QDragEnterEvent* event) {
    if (event->mimeData()->hasUrls()) {
        QList<QUrl> files = event->mimeData()->urls();
        for (auto&& it : files) {
            auto tmp = it.toLocalFile();
            if (!QFileInfo(tmp).isDir()) continue;
            spdlog::info("拖入文件夹：{}", tmp.toStdString());
            this->addItem(tmp);
        }
    }
    event->acceptProposedAction();
}

void QListWidgetDragDrop::dropEvent(QDropEvent* event) {
    // 不知道为什么，这个函数没有用
    if (event->mimeData()->hasUrls()) {
        QList<QUrl> files = event->mimeData()->urls();
        for (auto&& it : files) {
            auto tmp = it.toString();
            spdlog::info("拖入文件夹：{}", tmp.toStdString());
            this->addItem(tmp);
        }
    }
}
