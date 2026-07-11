#include "mainwindow.h"
#include <QApplication>
#include <spdlog/async.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/spdlog.h>

int main(int argc, char *argv[]) {
    QApplication a(argc, argv);
    a.setApplicationVersion("0.8");


    MainWindow w;
    w.show();

    const int ret = QApplication::exec();

    return ret;
}
