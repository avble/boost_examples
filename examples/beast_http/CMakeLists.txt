cmake_minimum_required(VERSION 3.18)

# set the project name
project(HttpServer)

find_package(Threads REQUIRED)
find_package(Boost 1.73.0 REQUIRED COMPONENTS filesystem system)

# add the executable
add_executable(http_srv
                src/main.cpp src/session.hpp src src/config.hpp
)

target_link_libraries(http_srv Threads::Threads Boost::filesystem Boost::system)