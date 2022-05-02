cmake_minimum_required(VERSION 3.18)

# set the project name
project(CpplangByEx)

## Boost library 
# set(Boost_DEBUG 1)
# set(Boost_NO_BOOST_CMAKE ON)
set(Boost_NO_SYSTEM_PATHS ON)
set(BOOST_ROOT ./git-sub/boost)

find_package(Threads REQUIRED)
find_package(Boost 1.73.0 REQUIRED 
  COMPONENTS filesystem system)
IF(Boost_FOUND)
  INCLUDE_DIRECTORIES(${Boost_INCLUDE_DIR})
ENDIF(Boost_FOUND)


# Catch2 deps
set(CATCH_BUILD_STATIC_LIBRARY ON)
add_subdirectory(git-sub/catch2)

# add the executable
# asio example
add_executable(async_timer ./examples/boost_asio/async_timer.cpp)

target_link_libraries(async_timer PUBLIC Threads::Threads)

add_subdirectory(examples/boost_asio/http)

# 
add_subdirectory(examples/beast_http)

# add_subdirectory(examples/sample_with_catch2)
# add_executable(catch2_test ./examples/sample_with_catch2/test.cpp)
# target_link_libraries(catch2_test Catch2::Catch2)