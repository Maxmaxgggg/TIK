#include "widget.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    a.setApplicationName("AND_OR");
    a.setOrganizationName("Alpas");
    Widget w;
    w.show();
    return a.exec();
}
