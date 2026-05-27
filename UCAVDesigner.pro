QT += widgets
CONFIG += c++17

TARGET = UCAVDesigner
TEMPLATE = app

SOURCES += \
    src/main.cpp \
    src/core/WeightAnalysis.cpp \
    src/core/ConstraintAnalysis.cpp \
    src/ui/MainWindow.cpp \
    src/ui/ConstraintPlotWidget.cpp

HEADERS += \
    src/core/DesignTypes.h \
    src/core/WeightAnalysis.h \
    src/core/ConstraintAnalysis.h \
    src/ui/MainWindow.h \
    src/ui/ConstraintPlotWidget.h

INCLUDEPATH += src
