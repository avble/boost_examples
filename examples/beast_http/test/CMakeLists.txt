cmake_minimum_required(VERSION 3.18)


project(TestAvble)

# include(FetchContent)
# FetchContent_Declare(catch2 
#     GIT_REPOSITORY git@github.com:catchorg/Catch2.git
#     GIT_TAG v2.13.4
# )

# FetchContent_MakeAvailable(catch2)

add_executable(test_avble_http
    main.cpp)

target_include_directories(test_avble_http PRIVATE ../include)
target_link_libraries(test_avble_http PRIVATE 
                        Catch2::Catch2
                        Threads::Threads Boost::filesystem Boost::system
                    )