cmake_minimum_required(VERSION 3.16)

project(boost_libdatachannel LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(Boost_USE_STATIC_LIBS OFF)

find_package(nlohmann_json CONFIG REQUIRED)
find_package(Boost COMPONENTS thread filesystem system program_options REQUIRED)
find_package(LibDataChannel CONFIG REQUIRED)


if (${Boost_FOUND})
    message("Found Boost " ${Boost_VERSION})
else()
    message("Couldn't find Boost")
endif()
add_executable(boost_libdatachannel main.cpp)


target_link_libraries(boost_libdatachannel
    Boost::thread
    Boost::filesystem
    Boost::system
    Boost::program_options
    nlohmann_json::nlohmann_json
    LibDataChannel::LibDataChannel
)

if(WIN32)
  target_link_libraries(boost_libdatachannel wsock32 ws2_32)
endif()

include(GNUInstallDirs)
install(TARGETS boost_libdatachannel
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
