cmake_minimum_required(VERSION 3.20)
project(BrightnessController VERSION 1.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 6.2 REQUIRED COMPONENTS Widgets)

add_executable(brightness-controller
    src/main.cpp
    src/mainwindow.cpp
    src/mainwindow.h
    src/brightnessbackend.cpp
    src/brightnessbackend.h
)

target_link_libraries(brightness-controller PRIVATE Qt6::Widgets)

if(WIN32)
    target_link_libraries(brightness-controller PRIVATE dxva2 user32)
endif()

if(UNIX AND NOT APPLE)
    target_link_libraries(brightness-controller PRIVATE pthread)
endif()

install(TARGETS brightness-controller RUNTIME DESTINATION bin)
