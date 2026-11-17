QT += widgets
CONFIG += c++17 console
CONFIG -= app_bundle
TEMPLATE = app
TARGET = ui_smoke
INCLUDEPATH += ../src
SOURCES += ui_smoke.cpp \
    ../src/ui/MainWindow.cpp \
    ../src/ui/ConstraintPlotWidget.cpp \
    ../src/core/WeightAnalysis.cpp \
    ../src/core/ConstraintAnalysis.cpp
HEADERS += ../src/ui/MainWindow.h ../src/ui/ConstraintPlotWidget.h
