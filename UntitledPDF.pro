QT += core gui widgets pdf pdfwidgets

CONFIG += c++17

msvc:QMAKE_CXXFLAGS += /utf-8

DEFINES += QT_DEPRECATED_WARNINGS

# 3rdparty
INCLUDEPATH += \
    3rdparty/PoDoFo/include

LIBS += \
    -L"$${PWD}/3rdparty/PoDoFo/lib" -lpodofo

INCLUDEPATH += \
    headers/

SOURCES += \
    sources/main.cpp \
    sources/mainwindow.cpp \
    sources/pageselector.cpp \
    sources/tools.cpp \
    sources/zoomselector.cpp

HEADERS += \
    headers/mainwindow.h \
    headers/pageselector.h \
    headers/tools.h \
    headers/zoomselector.h

FORMS += \
    forms/mainwindow.ui

RESOURCES += \
    resources.qrc

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
