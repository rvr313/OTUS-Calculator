#include "widget.h"

#include <QApplication>
#include <locale>

int main(int argc, char *argv[])
{
    std::locale loc;
    loc.global(loc);
    QApplication a(argc, argv);
    qApp->setApplicationName("Simple Calculator");

    Widget w;
    w.show();
    return a.exec();
}
