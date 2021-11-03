#pragma once

#define GLEW_STATIC
#include <GL/glew.h>

#include <GLFW/glfw3.h>

#include <array>

#include <utility>

#include <iostream>

struct Window {
  GLFWwindow *ptr;
  Window(int width, int height) {
    constexpr std::array<std::pair<int, int>, 5> hints{
        {{GLFW_CONTEXT_VERSION_MAJOR, 3},
         {GLFW_CONTEXT_VERSION_MINOR, 2},
         {GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE},
         {GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE},
         {GLFW_RESIZABLE, GL_TRUE}}};

    for (const auto [key, val] : hints) {
      glfwWindowHint(key, val);
    }

    ptr = glfwCreateWindow(width, height, "opengl", nullptr, nullptr);
    glfwMakeContextCurrent(ptr);

    glewExperimental = GL_TRUE;
    glewInit();
  }

  Window(Window &&other) : ptr(std::exchange(other.ptr, nullptr)) {}

  ~Window() {
    if (ptr != nullptr) {
      glfwDestroyWindow(ptr);
    }
  }
};

class GlfwContext {
private:
  bool moved = false;

public:
  Window window;
  GlfwContext(int w, int h)
      : window([&] {
          glfwInit();
          return Window(w, h);
        }()) {}

  GlfwContext(const GlfwContext &) = delete;
  GlfwContext(GlfwContext &&other) : window(std::move(other.window)) {
    other.moved = true;
  }

  ~GlfwContext() {
    if (!moved) {
      glfwTerminate();
    }
  }
};
