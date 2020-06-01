#include "mainwindow.hpp"
#include "broth.hpp"

#include <iostream>

#include <QApplication>

#include <SFML/Graphics.hpp>

int main(int argc, char **argv) {
    QApplication a(argc, argv);

    MainWindow w;
    w.show();

    int ret = a.exec();

    return ret;
}
