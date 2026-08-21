cmake_minimum_required(VERSION 3.16)
project(c-mi VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# --- Qt6 (Widgets + OpenGLWidgets) ---
find_package(Qt6 REQUIRED COMPONENTS Widgets OpenGLWidgets Gui Core)

# --- pkg-config deps: V4L2, FFmpeg, libudev ---
find_package(PkgConfig REQUIRED)
pkg_check_modules(V4L2 REQUIRED libv4l2)
pkg_check_modules(FFMPEG REQUIRED libavcodec libavformat libavutil libswscale libswresample libavdevice)
pkg_check_modules(LIBUDEV REQUIRED libudev)

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
    ${V4L2_INCLUDE_DIRS}
    ${FFMPEG_INCLUDE_DIRS}
    ${LIBUDEV_INCLUDE_DIRS}
)

target_link_directories(c-mi PRIVATE
    ${V4L2_LIBRARY_DIRS}
    ${FFMPEG_LIBRARY_DIRS}
    ${LIBUDEV_LIBRARY_DIRS}
)

target_compile_options(c-mi PRIVATE
    -Wall -Wextra
    ${V4L2_CFLAGS_OTHER}
    ${FFMPEG_CFLAGS_OTHER}
    ${LIBUDEV_CFLAGS_OTHER}
)

target_link_libraries(c-mi PRIVATE
    Qt6::Widgets
    Qt6::OpenGLWidgets
    Qt6::Gui
    Qt6::Core
    ${V4L2_LIBRARIES}
    ${FFMPEG_LIBRARIES}
    ${LIBUDEV_LIBRARIES}
    GL
    pthread
)

# --- Install / XDG integration ---
include(GNUInstallDirs)

install(TARGETS c-mi RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(FILES data/c-mi.desktop DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)
install(FILES data/icons/c-mi.svg
        DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/scalable/apps)
