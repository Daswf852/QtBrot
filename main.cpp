#include "mainwindow.hpp"
#include "broth.hpp"

#include <iostream>

#include <QApplication>

#include <SFML/Graphics.hpp>

int main(int argc, char **argv) {
    QApplication a(argc, argv);

    //std::unique_ptr<Broth> broth = std::make_unique<Broth>(640, 480);
    Broth b(640, 480);

    MainWindow w(b, nullptr);
    w.show();

    int ret = a.exec();

    return ret;
}
