cmake_minimum_required(VERSION 3.16)

project(daupi_server LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(Boost_USE_STATIC_LIBS OFF)

# Поиск библиотек
find_package(Boost COMPONENTS thread filesystem system program_options date_time REQUIRED)

# Сообщение об успехе/неудаче поиска Boost
if (${Boost_FOUND})
    message("Found Boost " ${Boost_VERSION})
else()
    message(FATAL_ERROR "Couldn't find Boost")
endif()

# Добавление поддиректорий для приложения и тестов
add_subdirectory(src)
add_subdirectory(tests)
add_subdirectory(src_client)
