QT       += core gui printsupport

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0
QMAKE_CXXFLAGS += -fopenmp
QMAKE_LFLAGS += -fopenmp
SOURCES += \
    src/filterplaintextedit.cpp \
    src/main.cpp \
    src/widget.cpp \
    src/qcustomplot.cpp

HEADERS += \
    src/filterplaintextedit.h \
    src/widget.h \
    src/qcustomplot.h

FORMS += \
    src/widget.ui

INCLUDEPATH += \
    src/

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

RESOURCES += \
    src/q.qrc

RC_ICONS += \
    src/icon.ico
