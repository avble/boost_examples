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

# openssl configure
set(OPENSSL_ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/install)
set(OPENSSL_USE_STATIC_LIBS TRUE)
find_package(OpenSSL REQUIRED )

IF (OPENSSL_FOUND)
INCLUDE_DIRECTORIES(${OPENSSL_INCLUDE_DIR})
ENDIF(OPENSSL_FOUND)

# add the executable
add_subdirectory(examples/beast_http)

add_subdirectory(examples/beast_ws_client)