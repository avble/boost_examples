cmake_minimum_required(VERSION 3.16)

# set the project name
project(WsClient CXX)

find_package(Threads REQUIRED)
find_package(Boost 1.73.0 REQUIRED COMPONENTS filesystem system)

set(CMAKE_CXX_STANDARD 17)

# Setting for boost
add_definitions(-DBOOST_BEAST_USE_STD_STRING_VIEW)

# add the executable
add_executable(ws_client
                ws_client.cpp
                main.cpp
            )

target_compile_options(ws_client PRIVATE -g -O0)

target_include_directories(ws_client PUBLIC   
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

target_link_libraries(ws_client Threads::Threads Boost::filesystem Boost::system)

# Test 
# add_subdirectory(test)
