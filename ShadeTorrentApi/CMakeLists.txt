cmake_minimum_required(VERSION 3.30)
project(ShadeTorrentApi)

set(CMAKE_CXX_STANDARD 20)

if(WIN32)
    add_definitions(-D_WIN32_WINNT=0x0601)
endif()

find_package(fmt CONFIG REQUIRED)
find_package(LibtorrentRasterbar CONFIG REQUIRED)
find_package(Crow CONFIG REQUIRED)
find_package(asio CONFIG REQUIRED)

add_executable(ShadeTorrentApi main.cpp
        torrent_manager.hpp
        torrent_manager.cpp
        http_server.hpp
        http_server.cpp)

target_link_libraries(ShadeTorrentApi PRIVATE
        fmt::fmt-header-only
        LibtorrentRasterbar::torrent-rasterbar
        Crow::Crow
        asio::asio
)

# Static linking flags
target_link_options(ShadeTorrentApi PRIVATE
        -static
        -static-libgcc
        -static-libstdc++
)