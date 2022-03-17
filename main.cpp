#include "mainwindow.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    qRegisterMetaType<std::vector<unsigned char>>("std::vector<unsigned char>");

    MainWindow w;
    w.show();

    return a.exec();
}
