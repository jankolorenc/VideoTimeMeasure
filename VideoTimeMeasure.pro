#-------------------------------------------------
#
# Project created by QtCreator 2014-10-07T20:35:52
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = VideoMeasure
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp \
    timeintervalsmodel.cpp \
    timeinterval.cpp \
    aspectratiopixmaplabel.cpp \
    navigationeventfilter.cpp

HEADERS  += mainwindow.h \
    videoimage.h \
    timeintervalsmodel.h \
    timeinterval.h \
    intervaltimestamp.h \
    aspectratiopixmaplabel.h \
    navigationeventfilter.h

FORMS    += mainwindow.ui

*-g++* {
    QMAKE_CXXFLAGS += -D__STDC_CONSTANT_MACROS -fpermissive
}

unix|win32: LIBS += -lm -lz -lswscale -lavformat -lavcodec -lavutil

win32: LIBS += -lvfw32 -lbz2 -liconv -lmp3lame -lopencore-amrwb -lopencore-amrnb -lopus -lspeex -ltheora -lvorbis -lvorbisenc -lvorbisfile -lvpx -lx264 -lxvidcore -logg -lpthread

RESOURCES += \
    Images.qrc
