cmake_minimum_required(VERSION 3.18)

# set the project name
project(HttpServer CXX)

find_package(Threads REQUIRED)
find_package(Boost 1.73.0 REQUIRED COMPONENTS filesystem system)

set(CMAKE_CXX_STANDARD 17)

# Setting for boost
add_definitions(-DBOOST_BEAST_USE_STD_STRING_VIEW)

# add the executable
add_executable(http_srv
                examples/main.cpp
                include/beast_rest/impl/session.cpp
            )


target_include_directories(http_srv PUBLIC   
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

target_link_libraries(http_srv Threads::Threads Boost::filesystem Boost::system)


# Test 
add_subdirectory(test)