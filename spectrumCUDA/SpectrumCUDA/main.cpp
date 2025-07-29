#include "widget.h"
#include <QtWidgets/QApplication>
#include "kernel.cuh"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("SpectrumCUDA");
    app.setOrganizationName("Alpas");
    Widget window;
    window.show();
    return app.exec();
}
