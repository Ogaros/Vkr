/*#define WAIT_FOR_CONNECT true
#define OVERRIDE_NEW_DELETE
#define UNICODE
#define _UNICODE
#define NOMINMAX
#include "D:\Programs\MemPro\MemProLib\src_combined\MemPro.cpp"*/

#include "mainwindow.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;
    w.show();

    return a.exec();
}
