cmake_minimum_required(VERSION 3.16)
project(c-mi VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

option(CMI_STATIC "Build a fully static executable" OFF)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# --- Qt6 (Widgets + OpenGLWidgets) ---
find_package(Qt6 REQUIRED COMPONENTS Widgets OpenGLWidgets Gui Core)

if(CMI_STATIC)
    foreach(QT_TARGET Qt6::Widgets Qt6::OpenGLWidgets Qt6::Gui Qt6::Core)
        get_target_property(QT_LOCATION ${QT_TARGET} IMPORTED_LOCATION_RELEASE)
        if(NOT QT_LOCATION)
            get_target_property(QT_LOCATION ${QT_TARGET} IMPORTED_LOCATION)
        endif()
        if(NOT QT_LOCATION OR NOT QT_LOCATION MATCHES "\\.a$")
            message(FATAL_ERROR
                "CMI_STATIC requires a static Qt6 installation; ${QT_TARGET} "
                "resolved to '${QT_LOCATION}'")
        endif()
    endforeach()

    set(PKG_CONFIG_USE_STATIC_LIBS TRUE)
    set(CMAKE_FIND_LIBRARY_SUFFIXES .a)
    set(CMAKE_LINK_SEARCH_START_STATIC TRUE)
    set(CMAKE_LINK_SEARCH_END_STATIC TRUE)
endif()

# --- pkg-config deps: FFmpeg ---
find_package(PkgConfig REQUIRED)
pkg_check_modules(FFMPEG REQUIRED libavcodec libavformat libavutil libswscale libswresample libavdevice)

add_executable(c-mi
    src/main.cpp
    src/v4l2/DeviceManager.h
    src/v4l2/DeviceManager.cpp
    src/v4l2/CaptureDevice.h
    src/v4l2/CaptureDevice.cpp
    src/v4l2/ControlPanel.h
    src/v4l2/ControlPanel.cpp
    src/encode/PhotoEncoder.h
    src/encode/PhotoEncoder.cpp
    src/encode/VideoEncoder.h
    src/encode/VideoEncoder.cpp
    src/ui/MainWindow.h
    src/ui/MainWindow.cpp
    src/ui/PreviewWidget.h
    src/ui/PreviewWidget.cpp
    src/ui/ControlSliders.h
    src/ui/ControlSliders.cpp
    src/ui/TrayIcon.h
    src/ui/TrayIcon.cpp
)

target_include_directories(c-mi PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${FFMPEG_INCLUDE_DIRS}
)

target_link_directories(c-mi PRIVATE
    ${FFMPEG_LIBRARY_DIRS}
)

target_compile_options(c-mi PRIVATE
    -Wall -Wextra
    ${FFMPEG_CFLAGS_OTHER}
)

target_link_libraries(c-mi PRIVATE
    Qt6::Widgets
    Qt6::OpenGLWidgets
    Qt6::Gui
    Qt6::Core
    ${FFMPEG_LIBRARIES}
    GL
    pthread
)

if(CMI_STATIC)
    target_link_options(c-mi PRIVATE
        -static
        -static-libgcc
        -static-libstdc++
    )
    if(COMMAND qt_import_plugins)
        qt_import_plugins(c-mi INCLUDE Qt6::QXcbIntegrationPlugin)
    else()
        message(FATAL_ERROR "CMI_STATIC requires Qt's qt_import_plugins command")
    endif()
endif()

# --- Install / XDG integration ---
include(GNUInstallDirs)

install(TARGETS c-mi RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(FILES data/c-mi.desktop DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)
install(FILES data/icons/c-mi.svg
        DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/scalable/apps)
