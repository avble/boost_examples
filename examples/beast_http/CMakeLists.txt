cmake_minimum_required(VERSION 3.18)

# set the project name
project(HttpServer)

find_package(Threads REQUIRED)
find_package(Boost 1.73.0 REQUIRED COMPONENTS filesystem system)

# add the executable
add_executable(http_srv
                src/main.cpp 
                include/beast_rest/session.hpp 
                include/beast_rest/config.hpp
            )

target_include_directories(http_srv PUBLIC   
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

target_link_libraries(http_srv Threads::Threads Boost::filesystem Boost::system)