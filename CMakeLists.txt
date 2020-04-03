cmake_minimum_required(VERSION 3.7)

project(dde_wldpms)

set(TARGET_NAME dde_wldpms)

add_executable(${TARGET_NAME} src/main.cpp)

find_package(KF5Wayland REQUIRED)
# Find the library
find_package(PkgConfig REQUIRED)
find_package(Qt5 COMPONENTS
    Core
    DBus
    Gui
REQUIRED)

target_link_libraries(${TARGET_NAME}  Qt5::Core)
target_link_libraries(${TARGET_NAME} KF5::WaylandClient KF5::WaylandServer)
