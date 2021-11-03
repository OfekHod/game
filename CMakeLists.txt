cmake_minimum_required(VERSION 3.10)

project(opengl_app)

add_subdirectory(SOIL)

add_executable(app main.cpp)

target_compile_features(app PRIVATE cxx_std_17)
target_include_directories(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

find_package(glm REQUIRED)

find_package(OpenGL REQUIRED COMPONENTS OpenGL)
find_package(GLEW REQUIRED)


find_package(PkgConfig REQUIRED)
pkg_search_module(GLFW REQUIRED glfw3)
target_include_directories(app PRIVATE ${GLFW_INCLUDE_DIRS})


target_link_libraries(app OpenGL::OpenGL GLEW::GLEW SOIL glm::glm ${GLFW_STATIC_LIBRARIES})
