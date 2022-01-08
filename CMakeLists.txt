cmake_minimum_required(VERSION 3.10)

project(opengl_app)
add_executable(game main.cpp math.cpp)
add_executable(load_bmp load_bmp.cpp math.cpp)
add_executable(load_obj load_obj.cpp math.cpp)


add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/external_libs/glfw-3.3.5")

FOREACH(exec_name IN ITEMS game load_bmp load_obj)
  target_compile_features(${exec_name} PRIVATE cxx_std_20)
  target_include_directories(${exec_name} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

  target_include_directories(${exec_name} PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/external_libs/glfw-3.3.5")

  find_package(OpenGL REQUIRED COMPONENTS OpenGL)

  IF (WIN32)
     set(GLEW_USE_STATIC ON CACHE BOOL "" FORCE)
     set(CMAKE_PREFIX_PATH "${CMAKE_CURRENT_SOURCE_DIR}/external_libs/glew-2.1.0/")
     find_package(GLEW REQUIRED)
     set(GLEWLIBS ${GLEW_STATIC_LIBRARIES})
  ELSEIF (APPLE)
     find_package(GLEW 2.0 REQUIRED)
     set(GLEWLIBS ${GLEW_LIBRARIES})
     target_include_directories(${exec_name} PRIVATE "/usr/local/include/")
  ELSE()
     find_package(GLEW REQUIRED)
     set(GLEWLIBS ${GLEW_LIBRARIES})
  ENDIF()

  IF (NOT APPLE)
     target_include_directories(${exec_name} PRIVATE ${GLEW_INCLUDE_DIRS})
  ENDIF()

  target_link_libraries(${exec_name} ${OPENGL_LIBRARIES} ${GLEWLIBS} glfw)
  target_compile_options(${exec_name} PRIVATE -Wall -Wextra -Wpedantic -Werror)
ENDFOREACH()
