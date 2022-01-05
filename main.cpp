#include "mainwindow/mainwindow.h"
#include <QApplication>
#include <QFontDatabase>
#include <QTextCodec>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    int fontId = QFontDatabase::addApplicationFont(":/fonts.otf");
    QString fontname = QFontDatabase::applicationFontFamilies (fontId).at(0);
    a.setFont(QFont(fontname, 9, QFont::Normal, false));
    QTextCodec *codec = QTextCodec::codecForName("UTF-8");
    QTextCodec::setCodecForLocale(codec);

    MainWindow w;
    w.show();

    return a.exec();
}
