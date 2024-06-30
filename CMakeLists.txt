cmake_minimum_required(VERSION 3.16)
project(windowsys)

set(CMAKE_CXX_STANDARD 17)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Quick WaylandCompositor)
find_package(KF6CoreAddons REQUIRED)

set(SOURCES
    src/main.cpp
)

qt_add_executable(windowsys ${SOURCES})

qt_add_qml_module(windowsys
    URI windowsys
    VERSION 1.0
    QML_FILES main.qml
)

target_link_libraries(windowsys
    Qt6::Core
    Qt6::Gui
    Qt6::Quick
    Qt6::WaylandCompositor
    KF6::CoreAddons
)
