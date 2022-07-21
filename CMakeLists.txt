cmake_minimum_required(VERSION 3.16)

# set the project name
project(CpplangByEx CXX)

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
add_subdirectory(examples/beast_http)

add_subdirectory(examples/beast_ws_client)