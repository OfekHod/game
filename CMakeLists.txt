cmake_minimum_required(VERSION 3.10)

project(opengl_app)

add_executable(app main.cpp math.cpp)

target_compile_features(app PRIVATE cxx_std_20)
target_include_directories(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})



#############################################
#GLFW
#############################################
target_include_directories(app PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/external_libs/glfw-3.3.5")
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/external_libs/glfw-3.3.5")
#############################################

find_package(OpenGL REQUIRED COMPONENTS OpenGL)

IF (WIN32)
   set(GLEW_USE_STATIC ON CACHE BOOL "" FORCE)
   set(CMAKE_PREFIX_PATH "${CMAKE_CURRENT_SOURCE_DIR}/external_libs/glew-2.1.0/")
   find_package(GLEW REQUIRED)
   set(GLEWLIBS ${GLEW_STATIC_LIBRARIES})
ELSEIF (APPLE)
   find_package(GLEW 2.0 REQUIRED)
   set(GLEWLIBS ${GLEW_LIBRARIES})
   target_include_directories(app PRIVATE "/usr/local/include/")
ELSE()
   find_package(GLEW REQUIRED)
   set(GLEWLIBS ${GLEW_LIBRARIES})
ENDIF()

IF (NOT APPLE)
   target_include_directories(app PRIVATE ${GLEW_INCLUDE_DIRS})
ENDIF()

target_link_libraries(app ${OPENGL_LIBRARIES} ${GLEWLIBS} glfw)
