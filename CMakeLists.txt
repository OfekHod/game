cmake_minimum_required(VERSION 3.10)

project(opengl_app)

add_executable(app main.cpp math.cpp read_images.cpp)

target_compile_features(app PRIVATE cxx_std_17)
target_include_directories(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})


#############################################
#LIBPNG
#############################################

add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/external_libs/zlib-1.2.11/")

set(PNG_SHARED OFF CACHE BOOL "" FORCE)
set(PNG_STATIC ON CACHE BOOL "" FORCE)

set(PNG_EXECUTABLES OFF CACHE BOOL "" FORCE)
set(PNG_TESTS OFF CACHE BOOL "" FORCE)
set(SKIP_INSTALL_ALL OFF CACHE BOOL "" FORCE)
set(PNG_BUILD_ZLIB ON CACHE BOOL "" FORCE)

target_include_directories(app PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/external_libs/libpng-1.6.37")
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/external_libs/libpng-1.6.37")
#############################################

find_package(OpenGL REQUIRED COMPONENTS OpenGL)
find_package(GLEW REQUIRED)
find_package(PkgConfig REQUIRED)

pkg_search_module(GLFW REQUIRED glfw3)
target_include_directories(app PRIVATE ${GLFW_INCLUDE_DIRS})


# Profile compile time
#SET(GCC_COVERAGE_COMPILE_FLAGS "-Q")
#add_definitions(${GCC_COVERAGE_COMPILE_FLAGS})
message("OY VEY")
target_link_libraries(app OpenGL::OpenGL GLEW::GLEW zlib png_static ${GLFW_STATIC_LIBRARIES})
