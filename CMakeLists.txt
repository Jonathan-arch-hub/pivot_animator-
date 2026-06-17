cmake_minimum_required(VERSION 3.16)
project(PivotAnimatorPro LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Widgets Gui)

set(SOURCES
    src/main.cpp
    src/animdata.cpp
    src/canvaswidget.cpp
    src/timelinewidget.cpp
    src/codeeditordialog.cpp
    src/mainwindow.cpp
)

set(HEADERS
    src/animdata.h
    src/canvaswidget.h
    src/timelinewidget.h
    src/codeeditordialog.h
    src/mainwindow.h
)

add_executable(pivot_animator ${SOURCES} ${HEADERS})

target_link_libraries(pivot_animator PRIVATE
    Qt6::Core
    Qt6::Widgets
    Qt6::Gui
)

target_include_directories(pivot_animator PRIVATE src)

# Install
install(TARGETS pivot_animator DESTINATION bin)
