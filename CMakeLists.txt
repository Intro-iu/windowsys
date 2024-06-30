cmake_minimum_required(VERSION 3.15)
project(windowsys)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(QT Core Gui Widgets Quick QuickControls2 DBus LinguistTools)
find_package(Qt6 REQUIRED ${QT})
find_package(KF6WindowSystem REQUIRED)
find_package(PkgConfig REQUIRED)
find_package(ECM REQUIRED NO_MODULE)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${ECM_MODULE_PATH})

add_subdirectory(session)