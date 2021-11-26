cmake_minimum_required(VERSION 3.10)

project(opengl_app)

add_executable(app main.cpp math.cpp)

target_compile_features(app PRIVATE cxx_std_17)
target_include_directories(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})


#############################################
#LIBPNG
#############################################

#include_directories("${CMAKE_CURRENT_SOURCE_DIR}/external_libs/zlib-1.2.11/")
#add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/external_libs/zlib-1.2.11/")
#
#set(PNG_SHARED OFF CACHE BOOL "" FORCE)
#set(PNG_STATIC ON CACHE BOOL "" FORCE)
#
#set(PNG_EXECUTABLES OFF CACHE BOOL "" FORCE)
#set(PNG_TESTS OFF CACHE BOOL "" FORCE)
#set(SKIP_INSTALL_ALL OFF CACHE BOOL "" FORCE)
#set(PNG_BUILD_ZLIB ON CACHE BOOL "" FORCE)
#
#message("WAT ${CMAKE_CURRENT_SOURCE_DIR}/external_libs/zlib-1.2.11")
#include_directories("${CMAKE_CURRENT_SOURCE_DIR}/external_libs/zlib-1.2.11")
#include_directories("${CMAKE_CURRENT_BINARY_DIR}/external_libs/libpng-1.6.37")
#target_include_directories(app PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/external_libs/libpng-1.6.37")
#add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/external_libs/libpng-1.6.37")
##############################################

#--------------------------------------------------------------------------------

#############################################
#GLFW
#############################################
target_include_directories(app PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/external_libs/glfw-3.3.5")
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/external_libs/glfw-3.3.5")
#############################################

find_package(OpenGL REQUIRED COMPONENTS OpenGL)


set(GLEW_USE_STATIC ON CACHE BOOL "" FORCE)
set(CMAKE_PREFIX_PATH "${CMAKE_CURRENT_SOURCE_DIR}/external_libs/glew-2.1.0/")
find_package(GLEW REQUIRED)
target_include_directories(app PRIVATE ${GLEW_INCLUDE_DIRS})


#find_package(PkgConfig REQUIRED)
#pkg_search_module(GLFW REQUIRED glfw3)
#target_include_directories(app PRIVATE ${GLFW_INCLUDE_DIRS})


# Profile compile time
#SET(GCC_COVERAGE_COMPILE_FLAGS "-Q")
#add_definitions(${GCC_COVERAGE_COMPILE_FLAGS})
target_link_libraries(app ${OPENGL_LIBRARIES} ${GLEW_STATIC_LIBRARIES} glfw)
