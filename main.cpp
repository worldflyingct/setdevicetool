#include "mainwindow/mainwindow.h"
#include <QApplication>
#include <QFontDatabase>
#include <QTextCodec>

int main (int argc, char *argv[]) {
    QApplication a(argc, argv);
    QFont font;
    int fontId = QFontDatabase::addApplicationFont(":/fonts.otf");
    QString fontname = QFontDatabase::applicationFontFamilies(fontId).at(0);
    font.setFamily(fontname);
    font.setPixelSize(13);
    font.setWeight(QFont::Normal);
    font.setItalic(false);
    a.setFont(font);
    QTextCodec *codec = QTextCodec::codecForName("UTF-8");
    QTextCodec::setCodecForLocale(codec);

    MainWindow w;
    w.show();

    return a.exec();
}
