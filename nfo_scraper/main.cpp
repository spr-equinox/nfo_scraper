#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
#    include <Windows.h>
#endif
#include <curl/curl.h>
#include <spdlog/spdlog.h>

#include <QtWidgets/QApplication>

#include "config.h"
#include "fetch_episode.h"
#include "fetch_seasons.h"
#include "fetch_shows.h"
#include "input_paths.h"
#include "process_shows.h"
#include "search_dialog.h"
#include "write_ignore.h"

const char version[] = "0.1";

int main(int argc, char *argv[]) {
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
    SetConsoleCP(65001);
    SetConsoleOutputCP(65001);
#endif
    curl_global_init(CURL_GLOBAL_ALL);
    spdlog::info("版本号：{}", version);
    spdlog::info("{}", curl_version());

    QApplication a(argc, argv);
    config cfg("config.json");
    write_ignore w6;
    fetch_episode w5(&cfg, &w6);
    fetch_seasons w4(&cfg, &w5);
    fetch_shows w3(&cfg, &w4);
    process_shows w2(&cfg, &w3);
    input_paths w1(&w2);
    w1.show();
    a.exec();
    curl_global_cleanup();
}
