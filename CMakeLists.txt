cmake_minimum_required(VERSION 3.14)

project(SimpleDesktop)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets WaylandClient WaylandCompositor)
find_package(KF6 REQUIRED COMPONENTS KWin)

add_executable(SimpleDesktop 
    src/main.cpp
)

target_link_libraries(SimpleDesktop
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::WaylandClient
    Qt6::WaylandCompositor
    KF6::KWin
)

install(TARGETS SimpleDesktop RUNTIME DESTINATION bin)
