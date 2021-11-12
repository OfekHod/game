cmake_minimum_required(VERSION 3.10)

project(png_app)

add_executable(app read_images.cpp)

target_compile_features(app PRIVATE cxx_std_17)
target_include_directories(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

find_package(PNG REQUIRED)
include_directories(${PNG_INCLUDE_DIR})

target_link_libraries(app ${PNG_LIBRARY})
