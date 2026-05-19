QT       += core gui widgets

CONFIG += c++11

TARGET = TodoList
DESTDIR = $$PWD/../bin

DEFINES += QT_DEPRECATED_WARNINGS

SOURCES += \
    main.cpp \
    Widget.cpp \
    TodoDelegate.cpp \
    TodoSortModel.cpp

HEADERS += \
    Widget.h \
    TodoData.h \
    TodoDelegate.h \
    TodoSortModel.h

FORMS += \
    Widget.ui
