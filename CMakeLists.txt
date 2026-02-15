cmake_minimum_required(VERSION 3.16)

project(KdePrimaryMonitorDetector LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets DBus)

add_executable(kde-primary-monitor
    src/main.cpp
)

target_link_libraries(kde-primary-monitor
    PRIVATE
        Qt6::Core
        Qt6::Gui
        Qt6::Widgets
        Qt6::DBus
)
