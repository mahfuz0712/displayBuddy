cmake_minimum_required(VERSION 3.20)
project(DisplayBuddy VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

option(DISPLAYBUDDY_FAKE_DISPLAYS "Use three fake displays (UI development without hardware, e.g. on WSL)" OFF)

find_package(Qt6 6.2 REQUIRED COMPONENTS Widgets)

set(DISPLAYBUDDY_SOURCES
    src/main.cpp
    src/mainwindow.cpp
    src/mainwindow.h
    src/brightnessbackend.cpp
    src/brightnessbackend.h
    src/brightnessworker.cpp
    src/brightnessworker.h
    src/flowlayout.cpp
    src/flowlayout.h
    src/icons.cpp
    src/icons.h
    src/theme.cpp
    src/theme.h
    src/widgets.h
    src/autostart.cpp
    src/autostart.h
    resources/resources.qrc
)

if(WIN32)
    # Executable icon + version info (.rc). The icon path is made absolute so
    # windres/rc find it no matter where the build directory is.
    enable_language(RC)
    set(DISPLAYBUDDY_ICON_PATH "${CMAKE_CURRENT_SOURCE_DIR}/resources/icons/displaybuddy.ico")
    configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/resources/displaybuddy.rc.in"
        "${CMAKE_CURRENT_BINARY_DIR}/displaybuddy.rc"
        @ONLY
    )
    list(APPEND DISPLAYBUDDY_SOURCES "${CMAKE_CURRENT_BINARY_DIR}/displaybuddy.rc")
endif()

add_executable(displaybuddy ${DISPLAYBUDDY_SOURCES})

target_compile_definitions(displaybuddy PRIVATE
    DISPLAYBUDDY_VERSION="${PROJECT_VERSION}"
)
if(DISPLAYBUDDY_FAKE_DISPLAYS)
    target_compile_definitions(displaybuddy PRIVATE DISPLAYBUDDY_FAKE_DISPLAYS)
endif()

target_link_libraries(displaybuddy PRIVATE Qt6::Widgets)

if(WIN32)
    set_target_properties(displaybuddy PROPERTIES WIN32_EXECUTABLE ON)
    target_link_libraries(displaybuddy PRIVATE dxva2 user32 dwmapi)
endif()

if(UNIX AND NOT APPLE)
    target_link_libraries(displaybuddy PRIVATE pthread)
endif()

include(GNUInstallDirs)
install(TARGETS displaybuddy RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

if(UNIX AND NOT APPLE)
    install(FILES resources/displaybuddy.desktop
            DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)
    foreach(size 16 24 32 48 64 128 256 512)
        install(FILES resources/icons/displaybuddy-${size}.png
                DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/${size}x${size}/apps
                RENAME displaybuddy.png)
    endforeach()
    install(FILES resources/icons/displaybuddy.svg
            DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/scalable/apps)
endif()
