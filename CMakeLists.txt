cmake_minimum_required(VERSION 3.21)

project(UCAVDesigner LANGUAGES CXX)

option(UCAV_BUILD_GUI "Build the Qt Widgets application" ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(ucav_core
    src/core/WeightAnalysis.cpp
    src/core/ConstraintAnalysis.cpp
)

target_compile_features(ucav_core PUBLIC cxx_std_17)
target_include_directories(ucav_core
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

if(UCAV_BUILD_GUI)
    set(CMAKE_AUTOMOC ON)
    set(CMAKE_AUTOUIC ON)
    set(CMAKE_AUTORCC ON)

    find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets)
    find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets)

    add_executable(UCAVDesigner WIN32
        src/main.cpp
        src/ui/MainWindow.cpp
        src/ui/MainWindow.h
        src/ui/ConstraintPlotWidget.cpp
        src/ui/ConstraintPlotWidget.h
    )

    target_link_libraries(UCAVDesigner
        PRIVATE
            ucav_core
            Qt${QT_VERSION_MAJOR}::Widgets
    )

    if(QT_VERSION_MAJOR EQUAL 6)
        qt_finalize_executable(UCAVDesigner)
    endif()
endif()

include(CTest)
if(BUILD_TESTING)
    add_executable(core_tests
        tests/core_tests.cpp
    )

    target_link_libraries(core_tests PRIVATE ucav_core)

    add_test(NAME core_tests COMMAND core_tests)
endif()
