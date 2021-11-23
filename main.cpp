#define GLEW_STATIC
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "math.hpp"
#include "read_images.hpp"

#include <cmath>
#include <cstdint>
#include <cstdio>

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

struct Shader {
  GLuint gl_ptr;
  Shader(const char *source, GLenum type) {
    gl_ptr = glCreateShader(type);
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

  GLuint vaos[2];
  glGenVertexArrays(2, vaos);
  glBindVertexArray(vaos[0]);
  GLuint overlay_shader_program = glCreateProgram();
  GLuint overlay_texture;
  glGenTextures(1, &overlay_texture);

  // Terrain def
  Image terrain;
  {
    size_t width = 100;
    size_t height = 100;
    uint8_t *pixels = (uint8_t *)malloc(sizeof(uint8_t *) * width * height);
    for (int row = 0; row < height; ++row) {
      for (int col = 0; col < width; ++col) {

        float col_norm = (float)col / (float)width;
        float row_norm = (float)row / (float)height;

        float val_f = 0.5F + 0.5F * sinf(row_norm * pi * 10);
        val_f += 0.5F + 0.5F * cosf(col_norm * pi * 10);
	val_f /= 2;

	val_f *= powf(1.0F - fabs(row_norm - 0.5), 4);
	val_f *= powf(1.0F - fabs(col_norm - 0.5), 4);
        uint8_t val = (int)(255.0F * val_f);
        pixels[row * width + col] = val;
      }
    }
    terrain.width = width;
    terrain.height = height;
    terrain.pixels = pixels;
  }
  // Overlay texture
  {
    glBindTexture(GL_TEXTURE_2D, overlay_texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, terrain.width, terrain.height, 0,
                 GL_RED, GL_UNSIGNED_BYTE, terrain.pixels);

    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    // clang-format off
    float attrs[] = {
	//verts        //tex coord
        0.6F, 1.0F,     0.0F, 1.0F,
        1.0F, 1.0F,     1.0F, 1.0F,
        1.0F, 0.6F,     1.0F, 0.0F,
        0.6F, 0.6F,     0.0F, 0.0F
    };

    GLuint elements[] = {
	    0, 1, 2,
	    2, 3, 0
    };
    // clang-format on
    GLuint arr[2];
    glGenBuffers(2, arr);

    {
      GLuint vbo = arr[0];
      glBindBuffer(GL_ARRAY_BUFFER, vbo);
      glBufferData(GL_ARRAY_BUFFER, sizeof(attrs), attrs, GL_STATIC_DRAW);
    }

    {
      GLuint ebo = arr[1];
      glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
      glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(elements), elements,
                   GL_STATIC_DRAW);
    }
    Shader vertex_shader = Shader(R"glsl(
    # version 150 core

    in vec2 position;
    in vec2 uv;

    out vec2 Texcoord;
    void main()
    {
        gl_Position = vec4(position, 0.0, 1.0);
	Texcoord = uv;
    }
    )glsl",
                                  GL_VERTEX_SHADER);

    Shader fragment_shader = Shader(R"glsl(
    # version 150 core
    out vec4 outColor;
    in vec2 Texcoord;

    uniform sampler2D tex;

    void main()
    {
       vec4 c = texture(tex, Texcoord); 
       outColor = vec4(c.r, c.r, c.r, 1.0);
    }
    )glsl",
                                    GL_FRAGMENT_SHADER);

    glAttachShader(overlay_shader_program, vertex_shader.gl_ptr);
    glAttachShader(overlay_shader_program, fragment_shader.gl_ptr);
    glBindFragDataLocation(overlay_shader_program, 0, "outColor");
    glLinkProgram(overlay_shader_program);

    GLuint pos_attrib = glGetAttribLocation(overlay_shader_program, "position");
    GLuint coord_attrib = glGetAttribLocation(overlay_shader_program, "uv");
    int attr_size = sizeof(float) * 4;
    glVertexAttribPointer(pos_attrib, 2, GL_FLOAT, GL_FALSE, attr_size, 0);
    glEnableVertexAttribArray(pos_attrib);
    glVertexAttribPointer(coord_attrib, 2, GL_FLOAT, GL_FALSE, attr_size,
                          (void *)(sizeof(float) * 2));
    glEnableVertexAttribArray(coord_attrib);
  }

  glBindVertexArray(vaos[1]);

  //********************************************************************************
  // Cube Def
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

  vec2f tex_c[4] {
      	{0.0F, 0.0F},
      	{0.0F, 1.0F},
	{1.0F, 1.0F},
      	{1.0F, 0.0F}
  };

  // clang-format on

  VertsContent verts_content;
  size_t n_verts = 6 * 4;
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
    for (auto &[rect, normal] : rects) {
      int a = ver_out;
      for (int j = 0; j < 4; j++) {
        verts_content.verts[ver_out] = cube_verts[rect[j]];
        verts_content.texcoords[ver_out] = tex_c[j];
        verts_content.normals[ver_out] = normal;
        ver_out++;
      }

      int b = a + 1;
      int c = a + 2;
      int d = a + 3;
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

  // Pass cube to opengl (attr for verts and elements for connections)
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

  //--------------------------------------------------------------------------------
  // Make Cube shader
  GLuint cube_shader_program = glCreateProgram();
  {

    const Shader vertex_shader(vertex_source, GL_VERTEX_SHADER);
    const Shader fragment_shader(fragment_source, GL_FRAGMENT_SHADER);

    glAttachShader(cube_shader_program, vertex_shader.gl_ptr);
    glAttachShader(cube_shader_program, fragment_shader.gl_ptr);
    glBindFragDataLocation(cube_shader_program, 0, "outColor");
    glLinkProgram(cube_shader_program);

    std::pair<const char *, size_t> program_args[3] = {
        {"position", vert_size},
        {"texcoord", texcoord_size},
        {"normal", vert_size}};

    size_t sizeof_attr = 0;
    for (const auto &[name, el_size] : program_args) {
      sizeof_attr += sizeof(float) * el_size;
    }

    size_t offset = 0;
    for (const auto &[name, el_size] : program_args) {
      GLuint attr_location = glGetAttribLocation(cube_shader_program, name);
      glEnableVertexAttribArray(attr_location);
      glVertexAttribPointer(attr_location, el_size, GL_FLOAT, GL_FALSE,
                            sizeof_attr, reinterpret_cast<void *>(offset));
      offset += sizeof(float) * el_size;
    }
  }

  //--------------------------------------------------------------------------------
  // Set cube texture
  GLuint texture;
  glGenTextures(1, &texture);
  {

    size_t width = 2;
    size_t height = 2;
    // clang-format off
    uint8_t pixels[12] = {
	    0  , 255, 255,     255, 255, 0,
	    255, 0  , 255,     0, 0  , 255
    };
    // clang-format on

    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB,
                 GL_UNSIGNED_BYTE, pixels);

    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  };

  {
    size_t el_size = sizeof(elements) / sizeof(elements[0]);
    GLint uniTrans = glGetUniformLocation(cube_shader_program, "trans");
    GLuint uniView = glGetUniformLocation(cube_shader_program, "view");
    GLint uniProj = glGetUniformLocation(cube_shader_program, "proj");
    GLint inter = glGetUniformLocation(cube_shader_program, "inter");

    time_point t_start = now();

    KeyState space = KeyState::KeyUp;
    float sign = 1;
    while (!glfwWindowShouldClose(window)) {

      glBindTexture(GL_TEXTURE_2D, texture);
      glUseProgram(cube_shader_program);
      if (glfwGetKey(window, GLFW_KEY_Q)) {
        printf("Exit\n");
        exit(0);
      };

      time_point t_now = now();
      float time = time_between(t_start, t_now);
      glUniform1f(inter, time);

      //--------------------------------------------------
      // Camera setup
      //--------------------------------------------------
      {
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
      };

      glClearColor(0.0F, 0.0F, 0.0F, 1.0F);
      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

      //--------------------------------------------------
      // Objects position
      //--------------------------------------------------
      glBindVertexArray(vaos[1]);
      for (int row = 0; row < terrain.height; ++row) {
        for (int col = 0; col < terrain.width; ++col) {
          uint8_t val_i = terrain.pixels[row * terrain.width + col];
          float val_f = (float)val_i / 255.0F;
          {
            float row_norm = (float)row / terrain.height;
            float col_norm = (float)col / terrain.width;
            const float scale = 0.5;

            //mat4f scale_mat = diagonal(0.1, val_f, 0.1, 1);
            mat4f rot = rotation(vec3f{0.0F, 1.0F, 0.0F}, time * 50.F * pi / 180.0F);

	    float w_pix = 1.0F / terrain.width;
	    float h_pix = 1.0F / terrain.height;

            mat4f trans =  diagonal(w_pix, val_f, h_pix, 1);
	    trans.elements[12] = row_norm - 0.5;
	    trans.elements[14] = col_norm - 0.5;

	    trans = trans * rot;
            glUniformMatrix4fv(uniTrans, 1, GL_FALSE, trans.elements);
          }

          glEnable(GL_DEPTH_TEST);
          glDrawElements(GL_TRIANGLES, el_size, GL_UNSIGNED_INT, 0);
        }
      }
      // Draw overlay texture
      glBindVertexArray(vaos[0]);
      glDisable(GL_DEPTH_TEST);
      glBindTexture(GL_TEXTURE_2D, overlay_texture);
      glUseProgram(overlay_shader_program);
      glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

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
    fprintf(stderr, "Usage: %s  <vertex shader>  <fragment shader>\n", argv[0]);
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
