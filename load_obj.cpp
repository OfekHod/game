
#define GLEW_STATIC
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "math.hpp"

#include <cassert>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <ctime>

#include <array>
#include <chrono>

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

GLuint
compile_shader(const char *source, GLenum type) {
  GLuint gl_ptr = glCreateShader(type);
  glShaderSource(gl_ptr, 1, &source, nullptr);
  glCompileShader(gl_ptr);

  GLint status;
  glGetShaderiv(gl_ptr, GL_COMPILE_STATUS, &status);

  if (status != GL_TRUE) {
    fprintf(stderr, "Fail compile shader\n");
    exit(1);
  }
  char buffer[512];
  glGetShaderInfoLog(gl_ptr, 512, nullptr, buffer);
  printf("%s\n", buffer);
  return gl_ptr;
};

struct VertsContent {
  size_t size;
  vec3f *verts;
  vec3f *normals;
};

constexpr int screen_width = 800;
constexpr int screen_height = 800;

struct DrawContext {
  GLuint vao;
  GLuint shader_program;
};

void
render(GLFWwindow *window) {

  DrawContext cube_context;
  glGenVertexArrays(1, &(cube_context.vao));
  cube_context.shader_program = glCreateProgram();
  glBindVertexArray(cube_context.vao);

  //********************************************************************************
  // Cube Def

  // <<<========= Here we should define loaded OBJ file
  //
  // clang-format off
  vec3f cube_verts[8] = {
      	{-0.5F,  0.5F, -0.5F},
      	{ 0.5F,  0.5F, -0.5F},
      	{ 0.5F, -0.5F, -0.5F},
      	{-0.5F, -0.5F, -0.5F},
      	{-0.5F,  0.5F,  0.5F},
      	{ 0.5F,  0.5F,  0.5F},
      	{ 0.5F, -0.5F,  0.5F},
      	{-0.5F, -0.5F,  0.5F}
      };

  std::pair<std::array<int, 4>, vec3f> rects[6]
  {
	  {{0, 1, 2, 3}, vec3f{0, 0, -1}},
	  {{1, 5, 6, 2}, vec3f{1, 0, 0}},
	  {{3, 2, 6, 7}, vec3f{0, -1, 0}},
	  {{0, 3, 7, 4}, vec3f{-1, 0, 0}},
	  {{0, 4, 5, 1}, vec3f{0, 1, 0}},
	  {{5, 4, 7, 6}, vec3f{0, 0, 1}}
  };

  // clang-format on

  VertsContent verts_content;
  constexpr size_t n_verts = 6 * 4;
  vec3f c_verts[n_verts];
  vec3f c_normals[n_verts];

  verts_content.size = n_verts;
  verts_content.verts = c_verts;
  verts_content.normals = c_normals;

  GLuint cube_elements[6 * 6];
  {
    int ver_out = 0;
    int el_out = 0;
    for (auto &[rect, normal] : rects) {
      int a = ver_out;
      for (int j = 0; j < 4; j++) {
        verts_content.verts[ver_out] = cube_verts[rect[j]];
        verts_content.normals[ver_out] = normal;
        ver_out++;
      }

      int b = a + 1;
      int c = a + 2;
      int d = a + 3;
      for (int x : {a, b, c, c, d, a}) {
        cube_elements[el_out++] = x;
      }
    }
  }

  constexpr size_t vert_size = 3;
  constexpr size_t texcoord_size = 2;
  constexpr size_t normal_size = 3;
  constexpr size_t attr_size = vert_size + texcoord_size + normal_size;

  float attr[n_verts * attr_size];

  {
    size_t attr_idx = 0;
    for (size_t vert_idx = 0; vert_idx < verts_content.size; vert_idx++) {
      vec3f *vert = &verts_content.verts[vert_idx];
      vec3f *normal = &verts_content.normals[vert_idx];

      attr[attr_idx++] = vert->x;
      attr[attr_idx++] = vert->y;
      attr[attr_idx++] = vert->z;

      attr[attr_idx++] = normal->x;
      attr[attr_idx++] = normal->y;
      attr[attr_idx++] = normal->z;
    }
  };

  // Pass cube to opengl (attr for verts and elements for connections)
  [&attr, &cube_elements] {
    GLuint arr[2];
    glGenBuffers(2, arr);

    const GLuint vbo = arr[0];
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(attr), attr, GL_STATIC_DRAW);

    const GLuint ebo = arr[1];
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(cube_elements), cube_elements,
                 GL_STATIC_DRAW);
  }();

  //--------------------------------------------------------------------------------
  // Make Cube shader
  {
    const char *new_vertex_source = R"glsl(
        #version 150 core

        in vec3 position;
        in vec3 normal;

        out vec3 FragPos;
        out vec3 Normal;

        uniform mat4 trans;
        uniform mat4 view;
        uniform mat4 proj;

        void
        main() {
	  vec4 pos_t = trans * vec4(position, 1.0);
          gl_Position = proj * view * trans * vec4(position, 1.0);
          FragPos = vec3(trans * vec4(position, 1.0));
          Normal = mat3(trans) * normal;
        }
    )glsl";

    const char *new_fragment_source = R"glsl(
        #version 150 core

	in vec3 Normal;
	in vec3 FragPos;

	out vec4 outColor;

	void
	main() {
	  vec4 black = vec4(0.2, 0.1, 0.1, 1.0);
	  vec4 green = vec4(0.0, 1.0, 0.0, 1.0);
	  vec4 blue = vec4(0.0, 0.0, 1.0, 1.0);
	  vec4 red = vec4(1.0, 0.4, 0.4, 1.0);
          
	  outColor = blue;

	  vec3 lightPos = vec3(0, 500, 400);
	  vec3 lightDir = normalize(lightPos - FragPos);
	  vec3 norm = normalize(Normal);
	  float diff = max(dot(norm, lightDir), 0.0);

	  outColor *= min(0.1 + diff, 1.0);
	}
    )glsl";
    const GLuint vertex_shader =
        compile_shader(new_vertex_source, GL_VERTEX_SHADER);
    const GLuint fragment_shader =
        compile_shader(new_fragment_source, GL_FRAGMENT_SHADER);

    glAttachShader(cube_context.shader_program, vertex_shader);
    glAttachShader(cube_context.shader_program, fragment_shader);
    glBindFragDataLocation(cube_context.shader_program, 0, "outColor");
    glLinkProgram(cube_context.shader_program);

    std::pair<const char *, size_t> program_args[] = {{"position", vert_size},
                                                      {"normal", vert_size}};

    size_t sizeof_attr = 0;
    for (const auto &[name, el_size] : program_args) {
      sizeof_attr += sizeof(float) * el_size;
    }

    size_t offset = 0;
    for (const auto &[name, el_size] : program_args) {
      GLuint attr_location =
          glGetAttribLocation(cube_context.shader_program, name);
      glEnableVertexAttribArray(attr_location);
      glVertexAttribPointer(attr_location, el_size, GL_FLOAT, GL_FALSE,
                            sizeof_attr, reinterpret_cast<void *>(offset));
      offset += sizeof(float) * el_size;
    }
  }

  //--------------------------------------------------------------------------------

  {
    GLint uniTrans = glGetUniformLocation(cube_context.shader_program, "trans");
    GLuint uniView = glGetUniformLocation(cube_context.shader_program, "view");
    GLint uniProj = glGetUniformLocation(cube_context.shader_program, "proj");

    float scale_f = 5.0F;
    float rot_f = 0.1;



    while (!glfwWindowShouldClose(window)) {

      // User input

      double mouse_x, mouse_y;
      glfwGetCursorPos(window, &mouse_x, &mouse_y);
      mouse_x = mouse_x / (screen_width * 0.5F) - 1.0F;
      mouse_y = mouse_y / (screen_height * 0.5F) - 1.0F;

      // Zoom in out
      {
        if (glfwGetKey(window, GLFW_KEY_W)) {
          scale_f -= 0.3;
        } else if (glfwGetKey(window, GLFW_KEY_S)) {
          scale_f += 0.3;
        }
        scale_f = fmaxf(scale_f, 2);
        scale_f = fminf(scale_f, 40);
      }

      // rotation
      {
        if (glfwGetKey(window, GLFW_KEY_A)) {
          rot_f += 0.1;
        } else if (glfwGetKey(window, GLFW_KEY_D)) {
          rot_f -= 0.1;
        }
      }

      if (glfwGetKey(window, GLFW_KEY_Q)) {
        printf("Exit\n");
        exit(0);
      };

      //=========================================

      glUseProgram(cube_context.shader_program);

      //--------------------------------------------------
      // Camera setup
      //--------------------------------------------------
      mat4f view;
      mat4f proj;
      vec3f cam_pos{scale_f * sinf(rot_f), 0.7F * scale_f,
                    scale_f * cosf(rot_f)};
      //      cam_pos = vec3f{0, 1, 10};
      {

        // clang-format off
        view =  look_at(
        		cam_pos,
        		vec3f{0.0F, 0.0F, 0.0F},
        		vec3f{0.0F, 1.0F, 0.0F});
        // clang-format on
        glUniformMatrix4fv(uniView, 1, GL_FALSE, view.elements);

        proj = perspective(45.0F * deg2rad, float(screen_width) / screen_height,
                           0.5f, 100.F);

        glUniformMatrix4fv(uniProj, 1, GL_FALSE, proj.elements);
      };


      glClearColor(0.0F, 0.0F, 0.0F, 1.0F);
      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

      float scale = 6.0F;
      mat4f scale_mat = diagonal(scale, 1.0F, scale, 1);

      glEnable(GL_DEPTH_TEST);

      // Draw terrain
      glBindVertexArray(cube_context.vao);
      glUseProgram(cube_context.shader_program);

      glUniformMatrix4fv(uniTrans, 1, GL_FALSE, scale_mat.elements);

      glDrawElements(GL_TRIANGLES, std::size(cube_elements), GL_UNSIGNED_INT, 0);

      glfwSwapBuffers(window);
      glfwPollEvents();
    }
  };
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
  glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);

  GLFWwindow *ptr = glfwCreateWindow(width, height, "opengl", nullptr, nullptr);
  glfwMakeContextCurrent(ptr);

  glewExperimental = GL_TRUE;
  glewInit();
  return ptr;
}

int
main() {

  GLFWwindow *window = open_window(screen_width, screen_height);
  render(window);

  glfwDestroyWindow(window);
  glfwTerminate();
  return 0;
}
