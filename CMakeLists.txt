cmake_minimum_required(VERSION 3.16)

project(boost_libdatachannel LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(Boost_USE_STATIC_LIBS OFF)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(Boost COMPONENTS thread filesystem system program_options date_time REQUIRED)
find_package(portaudio REQUIRED)
find_package(Opus CONFIG REQUIRED)
find_package(LibDataChannel CONFIG REQUIRED)


if (${Boost_FOUND})
    message("Found Boost " ${Boost_VERSION})
else()
    message("Couldn't find Boost")
endif()

set(PROJECT_SOURCES
        main.cpp
        mainwindow.cpp
        mainwindow.h
        mainwindow.ui
        #p2p.h p2p.cpp

)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(boost_libdatachannel
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
        websocket_client.h websocket_client.cpp
        p2p_connection.h p2p_connection.cpp
        screen_streamer.h screen_streamer.cpp
        call_dialog.h call_dialog.cpp call_dialog.ui
        audio/audio.h audio/audio.cpp
        audio/Audio_parametrs.h
    )
else()
    if(ANDROID)
        add_library(boost_libdatachannel SHARED
            ${PROJECT_SOURCES}
        )
    else()
        add_executable(boost_libdatachannel
            ${PROJECT_SOURCES}
        )
    endif()
endif()


target_link_libraries(boost_libdatachannel
    PRIVATE
    Qt${QT_VERSION_MAJOR}::Widgets
    portaudio
    Opus::opus
    Boost::thread
    Boost::filesystem
    Boost::system
    Boost::program_options
    Boost::date_time
    nlohmann_json::nlohmann_json
    LibDataChannel::LibDataChannel
)

if(WIN32)
  target_link_libraries(boost_libdatachannel PRIVATE wsock32 ws2_32)
endif()

include(GNUInstallDirs)
install(TARGETS boost_libdatachannel
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
