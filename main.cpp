#include "mainwindow.h"

#include <QApplication>
#include <QQmlApplicationEngine>
#include <QFile>
#include <QWebEngineView>
#include <QVBoxLayout>


int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    QQmlApplicationEngine engine;
    engine.load(QUrl(QStringLiteral("qrc:/durus_new.qml")));
    engine.load(QUrl(QStringLiteral("qrc:/map_new.qml")));


    if (engine.rootObjects().isEmpty())
        return -1;

    MainWindow w;

   // w.showFullScreen();
    w.show();
    return a.exec();
}
