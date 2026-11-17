cmake_minimum_required(VERSION 3.16)

project(UCAVDesigner VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets)

set(UCAV_CORE_SOURCES
    src/core/WeightAnalysis.cpp
    src/core/ConstraintAnalysis.cpp
)

add_library(ucav_core STATIC
    ${UCAV_CORE_SOURCES}
    src/core/DesignTypes.h
    src/core/WeightAnalysis.h
    src/core/ConstraintAnalysis.h
)
target_include_directories(ucav_core PUBLIC src)

add_executable(UCAVDesigner
    src/main.cpp
    src/ui/MainWindow.cpp
    src/ui/MainWindow.h
    src/ui/ConstraintPlotWidget.cpp
    src/ui/ConstraintPlotWidget.h
)
target_link_libraries(UCAVDesigner PRIVATE ucav_core Qt${QT_VERSION_MAJOR}::Widgets)

add_executable(core_tests tests/core_tests.cpp)
target_link_libraries(core_tests PRIVATE ucav_core)

enable_testing()
add_test(NAME core_tests COMMAND core_tests)
