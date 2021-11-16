#include "glfw_wrapper.hpp"
#include "math.hpp"
#include "read_images.hpp"

#include <cmath>
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
update_kstate(KeyState s, int curr_state) {
  if (curr_state != GLFW_PRESS) {
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

template <typename T, size_t N>
constexpr auto
value_size(const std::array<T, N> &arr) {
  return std::tuple_size<T>::value;
}

template <typename T, size_t N>
constexpr auto
sizeof_value(const std::array<T, N> &arr) {
  return sizeof(T);
}

template <size_t N_VERTS> struct VertsContent {
  std::array<std::array<float, 3>, N_VERTS> verts;
  std::array<std::array<float, 2>, N_VERTS> texcoords;
};

constexpr int screen_width = 1000;
constexpr int screen_height = 1000;

void
render(char *vertex_source, char *fragment_source) {

  GlfwContext ctx(screen_width, screen_height);

  glEnable(GL_DEPTH_TEST);
  GLuint vao;
  glGenVertexArrays(1, &vao);
  glBindVertexArray(vao);

  // clang-format off

  const std::array<std::array<float, 3>, 8>
	  cube_verts = {{
      	{-0.5F,  0.5F, -0.5F},
      	{ 0.5F,  0.5F, -0.5F},
      	{ 0.5F, -0.5F, -0.5F},
      	{-0.5F, -0.5F, -0.5F},
      	{-0.5F,  0.5F,  0.5F},
      	{ 0.5F,  0.5F,  0.5F},
      	{ 0.5F, -0.5F,  0.5F},
      	{-0.5F, -0.5F,  0.5F}
      }};

  const std::array<std::array<int, 4>, 6> rects{
  {
	{0, 1, 2, 3},
	{1, 5, 6, 2},
	{3, 2, 6, 7},
	{0, 3, 7, 4},
	{0, 4, 5, 1},
	{5, 4, 7, 6}
  }};

  const std::array<std::array<float, 2>, 4> tex_c {{
      	{0.0F, 0.0F},
      	{0.0F, 1.0F},
	{1.0F, 1.0F},
      	{1.0F, 0.0F}
  }};

  // clang-format on

  VertsContent<6 * 4> verts_content;
  std::array<GLuint, 6 * 6> elements{};
  {
    int ver_out = 0;
    int el_out = 0;
    int rect_num = 0;
    for (const auto rect : rects) {
      const int a = ver_out;
      for (int j = 0; j < 4; j++) {
        verts_content.verts[ver_out] = cube_verts[rect[j]];
        verts_content.texcoords[ver_out] = tex_c[j];
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

  constexpr size_t attr_size = vert_size + texcoord_size;

  std::array<float, verts_content.verts.size() * attr_size> attr;
  [&attr, &verts_content] {
    auto attr_it = std::begin(attr);
    for (auto [it_v, it_t] = std::tuple(begin(verts_content.verts),
                                        begin(verts_content.texcoords));
         it_v != end(verts_content.verts); ++it_v, ++it_t) {
      attr_it = std::copy(begin(*it_v), end(*it_v), attr_it);
      attr_it = std::copy(begin(*it_t), end(*it_t), attr_it);
    }
  }();

  [&attr, &elements] {
    std::array<GLuint, 2> arr;
    glGenBuffers(2, arr.data());

    const GLuint vbo = std::get<0>(arr);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(attr), attr.data(), GL_STATIC_DRAW);

    const GLuint ebo = std::get<1>(arr);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(elements), elements.data(),
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

    std::array<std::tuple<const char *, size_t>, 2> program_args{
        {{"position", vert_size}, {"texcoord", texcoord_size}}};

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

    const std::array<const char *, 2> shader_pnames{"tex1", "tex2"};

    const std::array<GLenum, 2> tex_enums{GL_TEXTURE0, GL_TEXTURE1};

    PngImage image_1 = read_png_file("../texture1.png");
    PngImage image_2;
    image_2.width = 2;
    image_2.height = 2;
    // clang-format off
    unsigned char img2_pixels[12] = {
	    0  , 255, 255,     255, 255, 0,
	    255, 0  , 255,     255, 0  , 0
    };
    // clang-format on
    image_2.pixels = img2_pixels;

    PngImage *images_p[2] = {&image_1, &image_2};

    for (int i = 0; i < 2; i++) {
      PngImage *curr = images_p[i];

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

  [&ctx, &shader_program, el_size = elements.size()] {
    const GLint uniTrans = glGetUniformLocation(shader_program, "trans");
    const GLuint uniView = glGetUniformLocation(shader_program, "view");
    const GLint uniProj = glGetUniformLocation(shader_program, "proj");
    const GLint inter = glGetUniformLocation(shader_program, "inter");

    const time_point t_start = now();

    KeyState space = KeyState::KeyUp;
    const auto window = ctx.window.ptr;
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
            rotation(vec3f{1.0F, 0.0F, 0.0F}, time * 50.F * pi / 180.0F);
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

      glfwSwapBuffers(ctx.window.ptr);
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
  long fsize = ftell(f);
  fseek(f, 0, SEEK_SET);
  char *content = (char *)malloc(sizeof(char) * fsize + 1);
  fread(content, sizeof(char), fsize, f);
  fclose(f);
  content[fsize] = 0;
  return content;
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

  render(vertex_source, fragment_source);
  return 0;
}
