
#include "MainWindow.h"
#include <QtWidgets/QApplication>
#include <QTextCodec>

int main(int argc, char *argv[])
{
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QApplication a(argc, argv);
    MainWindow w;
    a.setFont(QFont("Microsoft JhengHei UI", 10));
    w.show();
    w.setFixedSize(1060, 450);

    return a.exec();
}
