cmake_minimum_required(VERSION 2.8.9)

project(server)
set(CMAKE_BUILD_TYPE Release)

include_directories(./src/headers ./extern/finapi/Low/include)

file(GLOB_RECURSE SOURCES "src/source/*.cpp")

add_executable(server ${SOURCES})

target_link_libraries(server pthread dl ${CMAKE_SOURCE_DIR}/extern/finapi/Low/build/libfinapi.a)