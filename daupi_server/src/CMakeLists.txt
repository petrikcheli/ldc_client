
add_executable(daupi_server
    main.cpp
    server.h
    server.cpp
)

# Линкуем библиотеки
target_link_libraries(daupi_server
    Boost::thread Boost::filesystem Boost::system Boost::program_options Boost::date_time
)

# Установка библиотек Boost
include_directories(${Boost_INCLUDE_DIRS})

# Линкуем для Windows
if(WIN32)
    target_link_libraries(daupi_server wsock32 ws2_32)
endif()

# Указываем пути для установки приложения
install(TARGETS daupi_server
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
