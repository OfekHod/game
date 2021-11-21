#define GLEW_STATIC
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "math.hpp"
#include "read_images.hpp"

#include <cmath>
#include <cstdint>
#include <cstdio>

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <fstream>
#include <iostream>
#include <sstream>
#include <streambuf>
#include <string>
#include <thread>
#include <utility>

typedef std::chrono::high_resolution_clock::time_point time_point;

time_point
now() {
  return std::chrono::high_resolution_clock::now();
}

float
time_between(time_point start, time_point end) {
  return std::chrono::duration_cast<std::chrono::duration<float>>(end - start)
      .count();
}

enum class KeyState { KeyUp, KeyDown, KeyPressed };

KeyState
update_kstate(KeyState s, int glfw_state) {
  if (glfw_state != GLFW_PRESS) {
    return KeyState::KeyUp;
  } else if (s == KeyState::KeyUp) {
    return KeyState::KeyPressed;
  } else {
    return KeyState::KeyDown;
  }
}

bool
is_pressed(KeyState s) {
  return s == KeyState::KeyPressed;
}

struct Shader {
  GLuint gl_ptr;
  Shader(const char *source, GLenum type) {
    gl_ptr = glCreateShader(type);
    glShaderSource(gl_ptr, 1, &source, nullptr);
    glCompileShader(gl_ptr);

    GLint status;
    glGetShaderiv(gl_ptr, GL_COMPILE_STATUS, &status);

    if (status != GL_TRUE) {
      std::cout << "Fail compile\n";
      exit(1);
    }
    char buffer[512];
    glGetShaderInfoLog(gl_ptr, 512, nullptr, buffer);
    std::cout << buffer << '\n';
  }
};

struct VertsContent {
  size_t size;
  vec3f *verts;
  vec2f *texcoords;
  vec3f *normals;
};

constexpr int screen_width = 1000;
constexpr int screen_height = 1000;

void
render(GLFWwindow *window, char *vertex_source, char *fragment_source) {

  glEnable(GL_DEPTH_TEST);
  GLuint vao;
  glGenVertexArrays(1, &vao);
  glBindVertexArray(vao);

  // clang-format off

  const vec3f cube_verts[8] = {
      	{-0.5F,  0.5F, -0.5F},
      	{ 0.5F,  0.5F, -0.5F},
      	{ 0.5F, -0.5F, -0.5F},
      	{-0.5F, -0.5F, -0.5F},
      	{-0.5F,  0.5F,  0.5F},
      	{ 0.5F,  0.5F,  0.5F},
      	{ 0.5F, -0.5F,  0.5F},
      	{-0.5F, -0.5F,  0.5F}
      };

  const std::pair<std::array<int, 4>, vec3f> rects[6]
  {
	  {{0, 1, 2, 3}, vec3f{0, 0, -1}},
	  {{1, 5, 6, 2}, vec3f{1, 0, 0}},
	  {{3, 2, 6, 7}, vec3f{0, -1, 0}},
	  {{0, 3, 7, 4}, vec3f{-1, 0, 0}},
	  {{0, 4, 5, 1}, vec3f{0, 1, 0}},
	  {{5, 4, 7, 6}, vec3f{0, 0, 1}}
  };

  const vec2f tex_c[4] {
      	{0.0F, 0.0F},
      	{0.0F, 1.0F},
	{1.0F, 1.0F},
      	{1.0F, 0.0F}
  };

  // clang-format on

  VertsContent verts_content;
  const size_t n_verts = 6 * 4;
  vec3f c_verts[n_verts];
  vec2f c_coords[n_verts];
  vec3f c_normals[n_verts];

  verts_content.size = n_verts;
  verts_content.verts = c_verts;
  verts_content.texcoords = c_coords;
  verts_content.normals = c_normals;

  GLuint elements[6 * 6];
  {
    int ver_out = 0;
    int el_out = 0;
    int rect_num = 0;
    for (const auto [rect, normal] : rects) {
      const int a = ver_out;
      for (int j = 0; j < 4; j++) {
        verts_content.verts[ver_out] = cube_verts[rect[j]];
        verts_content.texcoords[ver_out] = tex_c[j];
        verts_content.normals[ver_out] = normal;
        ver_out++;
      }

      const int b = a + 1;
      const int c = a + 2;
      const int d = a + 3;
      for (int x : {a, b, c, c, d, a}) {
        elements[el_out++] = x;
      }
    }
  }

  constexpr size_t vert_size = 3;
  constexpr size_t texcoord_size = 2;
  constexpr size_t normal_size = 3;

  constexpr size_t attr_size = vert_size + texcoord_size + normal_size;

  float attr[verts_content.size * attr_size];

  [&attr, &verts_content] {
    size_t attr_idx = 0;
    for (size_t vert_idx = 0; vert_idx < verts_content.size; vert_idx++) {
      vec3f *vert = &verts_content.verts[vert_idx];
      vec2f *coord = &verts_content.texcoords[vert_idx];
      vec3f *normal = &verts_content.normals[vert_idx];


      attr[attr_idx++] = vert->x;
      attr[attr_idx++] = vert->y;
      attr[attr_idx++] = vert->z;

      attr[attr_idx++] = coord->x;
      attr[attr_idx++] = coord->y;

      attr[attr_idx++] = normal->x;
      attr[attr_idx++] = normal->y;
      attr[attr_idx++] = normal->z;
    }
  }();

  [&attr, &elements] {
    GLuint arr[2];
    glGenBuffers(2, arr);

    const GLuint vbo = arr[0];
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(attr), attr, GL_STATIC_DRAW);

    const GLuint ebo = arr[1];
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(elements), elements,
                 GL_STATIC_DRAW);
  }();

  const auto shader_program = [&vertex_source, &fragment_source, vert_size,
                               texcoord_size] {
    auto shader_program = glCreateProgram();

    const Shader vertex_shader(vertex_source, GL_VERTEX_SHADER);
    const Shader fragment_shader(fragment_source, GL_FRAGMENT_SHADER);

    glAttachShader(shader_program, vertex_shader.gl_ptr);
    glAttachShader(shader_program, fragment_shader.gl_ptr);
    glBindFragDataLocation(shader_program, 0, "outColor");
    glLinkProgram(shader_program);
    glUseProgram(shader_program);

    std::array<std::tuple<const char *, size_t>, 3> program_args{
        {{"position", vert_size},
         {"texcoord", texcoord_size},
         {"normal", vert_size}}};

    size_t sizeof_attr = 0;
    for (const auto &x : program_args) {
      sizeof_attr += sizeof(float) * std::get<1>(x);
    }

    size_t offset = 0;
    for (const auto &[name, el_size] : program_args) {
      GLuint attr_location = glGetAttribLocation(shader_program, name);
      glEnableVertexAttribArray(attr_location);
      glVertexAttribPointer(attr_location, el_size, GL_FLOAT, GL_FALSE,
                            sizeof_attr, reinterpret_cast<void *>(offset));
      offset += sizeof(float) * el_size;
    }

    return shader_program;
  }();

  [&shader_program] {
    std::array<GLuint, 2> textures;
    glGenTextures(2, textures.data());

    const char *shader_pnames[2] = {"tex1", "tex2"};
    const GLenum tex_enums[2] = {GL_TEXTURE0, GL_TEXTURE1};

    Image image_1 = read_png_file("../texture1.png");
    Image image_2;
    image_2.width = 2;
    image_2.height = 2;
    // clang-format off
    unsigned char img2_pixels[12] = {
	    0  , 255, 255,     255, 255, 0,
	    255, 0  , 255,     255, 0  , 0
    };
    // clang-format on
    image_2.pixels = img2_pixels;

    Image *images_p[2] = {&image_1, &image_2};

    for (int i = 0; i < 2; i++) {
      Image *curr = images_p[i];

      glActiveTexture(tex_enums[i]);

      glBindTexture(GL_TEXTURE_2D, textures[i]);
      {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, curr->width, curr->height, 0,
                     GL_RGB, GL_UNSIGNED_BYTE, curr->pixels);
      }

      glGenerateMipmap(GL_TEXTURE_2D);
      glUniform1i(glGetUniformLocation(shader_program, shader_pnames[i]), i);

      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    }
  }();

  [&window, &shader_program, el_size = sizeof(elements) / sizeof(GLuint)] {
    const GLint uniTrans = glGetUniformLocation(shader_program, "trans");
    const GLuint uniView = glGetUniformLocation(shader_program, "view");
    const GLint uniProj = glGetUniformLocation(shader_program, "proj");
    const GLint inter = glGetUniformLocation(shader_program, "inter");

    const time_point t_start = now();

    KeyState space = KeyState::KeyUp;
    float sign = 1;
    while (!glfwWindowShouldClose(window)) {

      if (glfwGetKey(window, GLFW_KEY_Q)) {
        std::cout << "Exit\n";
        exit(0);
      };

      space = update_kstate(space, glfwGetKey(window, GLFW_KEY_SPACE));
      if (space == KeyState::KeyPressed) {
        sign *= -1;
      }

      time_point t_now = now();
      float time = time_between(t_start, t_now);
      glUniform1f(inter, time / 2.0F);

      [uniTrans, time, &space, &sign] {
        const float scale = 0.5;

        mat4f scale_mat = diagonal(scale, scale, scale, 1);
        mat4f rot =
            rotation(vec3f{1.0F, 1.0F, 1.0F}, time * 50.F * pi / 180.0F);
        mat4f trans = rot * scale_mat;
        trans = trans * rotation(vec3f{1.0F, 0.0F, 0.0F},
                                 0.5F * (1.0F + sign) * (180.0F * pi / 180));
        glUniformMatrix4fv(uniTrans, 1, GL_FALSE, trans.elements);
      }();
      [time, shader_program, uniView, uniProj] {
        // clang-format off
	mat4f view =  look_at(
			vec3f{1.2F, 1.2F, 1.2F},
			vec3f{0.0F, 0.0F, 0.0F},
			vec3f{0.0F, 1.0F, 0.0F});
        // clang-format on

        glUniformMatrix4fv(uniView, 1, GL_FALSE, view.elements);

        mat4f proj =
            perspective(45.0F * pi / 180.0F,
                        float(screen_width) / screen_height, 1.0f, 10.F);

        glUniformMatrix4fv(uniProj, 1, GL_FALSE, proj.elements);
      }();

      glClearColor(0.0F, 0.0F, 0.0F, 1.0F);
      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
      glDrawElements(GL_TRIANGLES, el_size, GL_UNSIGNED_INT, 0);

      glfwSwapBuffers(window);
      glfwPollEvents();
    }
  }();
}

char *
read_whole_file(char *path) {
  FILE *f = fopen(path, "r");
  if (f == nullptr) {
    return nullptr;
  }
  fseek(f, 0, SEEK_END);
  size_t fsize = ftell(f);
  fseek(f, 0, SEEK_SET);
  char *content = (char *)malloc(sizeof(char) * fsize + 1);
  fread(content, sizeof(char), fsize, f);
  fclose(f);
  content[fsize] = 0;
  return content;
}

GLFWwindow *
open_window(int width, int height) {
  glfwInit();
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
  glfwWindowHint(GLFW_RESIZABLE, GL_TRUE);

  GLFWwindow *ptr = glfwCreateWindow(width, height, "opengl", nullptr, nullptr);
  glfwMakeContextCurrent(ptr);

  glewExperimental = GL_TRUE;
  glewInit();
  return ptr;
}

int
main(int argc, char **argv) {
  if (argc < 3) {
    std::cerr << "Usage: " << argv[0] << " <vertex shader>"
              << " <fragment shader>\n";
    exit(1);
  }

  char *vertex_source(read_whole_file(argv[1]));
  char *fragment_source(read_whole_file(argv[2]));

  GLFWwindow *window = open_window(screen_width, screen_height);
  render(window, vertex_source, fragment_source);

  glfwDestroyWindow(window);
  glfwTerminate();
  free(vertex_source);
  free(fragment_source);
  return 0;
}
