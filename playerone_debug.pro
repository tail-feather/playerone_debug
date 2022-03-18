QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    ccdplayerone.cpp \
    main.cpp \
    mainwindow.cpp

HEADERS += \
    ccdplayerone.h \
    logging.hpp \
    mainwindow.h

FORMS += \
    mainwindow.ui

INCLUDEPATH += ./include/

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

defineTest(copyToDestDir) {
    sources = $$1
    dest = $$2

    for(FILE, sources) {
        DDIR = $$dest

        # Replace slashes in paths with backslashes for Windows
        win32:FILE ~= s,/,\\,g
        win32:DDIR ~= s,/,\\,g

        QMAKE_POST_LINK += $$QMAKE_COPY $$quote($$FILE) $$quote($$DDIR) $$escape_expand(\\n\\t)
    }

    export(QMAKE_POST_LINK)
}

unix {
    QMAKE_LIBDIR_FLAGS += -Wl,-rpath $$PWD/lib
    LIBS += -L$$PWD/lib/ -lPlayerOneCamera
}
win32 {
    LIBS += $$PWD/lib/PlayerOneCamera.lib

    CONFIG(debug, debug|release) {
        OUTDIR = $$OUT_PWD/debug
    } else {
        OUTDIR = $$OUT_PWD/release
    }
    copyToDestDir($$PWD/lib/PlayerOneCamera.dll, $$OUTDIR)
}
