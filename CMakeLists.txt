cmake_minimum_required(VERSION 3.16)

project(boost_libdatachannel LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(Boost_USE_STATIC_LIBS OFF)

find_package(PkgConfig REQUIRED)
pkg_check_modules(FFMPEG REQUIRED libavcodec libavformat libavutil libswscale libavdevice)


find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(Boost COMPONENTS thread filesystem system program_options date_time REQUIRED)
find_package(portaudio REQUIRED)
find_package(Opus CONFIG REQUIRED)
find_package(LibDataChannel CONFIG REQUIRED)
find_package(cpr CONFIG REQUIRED)


if (${Boost_FOUND})
    message("Found Boost " ${Boost_VERSION})
else()
    message("Couldn't find Boost")
endif()

add_subdirectory(src)

