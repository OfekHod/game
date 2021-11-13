cmake_minimum_required(VERSION 3.10)

project(opengl_app)

add_executable(app main.cpp math.cpp read_images.cpp)

target_compile_features(app PRIVATE cxx_std_17)
target_include_directories(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

find_package(PNG REQUIRED)
include_directories(${PNG_INCLUDE_DIR})

find_package(OpenGL REQUIRED COMPONENTS OpenGL)
find_package(GLEW REQUIRED)


find_package(PkgConfig REQUIRED)
pkg_search_module(GLFW REQUIRED glfw3)
target_include_directories(app PRIVATE ${GLFW_INCLUDE_DIRS})

# Profile compile time
#SET(GCC_COVERAGE_COMPILE_FLAGS "-Q")
#add_definitions(${GCC_COVERAGE_COMPILE_FLAGS})

target_link_libraries(app OpenGL::OpenGL GLEW::GLEW ${PNG_LIBRARY} ${GLFW_STATIC_LIBRARIES})
