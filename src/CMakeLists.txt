set(PROJECT_SOURCES
        main.cpp
        ui/mainwindow.cpp
        ui/mainwindow.h
        ui/mainwindow.ui
        #p2p.h p2p.cpp

)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(boost_libdatachannel
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
        websocket_client/websocket_client.h websocket_client/websocket_client.cpp
        client/p2p_connection.h client/p2p_connection.cpp
        screen_share/screen_streamer.h screen_share/screen_streamer.cpp
        call_dialog/call_dialog.h call_dialog/call_dialog.cpp call_dialog/call_dialog.ui
        audio/audio.h audio/audio.cpp
        audio/Audio_parametrs.h
        enums/es_p2p.h
        video_camera/video_camera.h video_camera/video_camera.cpp
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
