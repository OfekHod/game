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
enum class WaveState { Add, Adding, DoneAdding, Simulate };

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

struct Wave {
  float x;
  float y;
  float speed;
  float size;
  float time;
};

struct DrawContext {
  GLuint vao;
  GLuint shader_program;
};

void
switch_to_context(DrawContext *ctx) {
  glBindVertexArray(ctx->vao);
  glUseProgram(ctx->shader_program);
}

void
init_debug_draw(DrawContext *debug_context, mat4f view, mat4f proj) {
  switch_to_context(debug_context);
  GLuint uniView = glGetUniformLocation(debug_context->shader_program, "view");
  GLint uniProj = glGetUniformLocation(debug_context->shader_program, "proj");
  glUniformMatrix4fv(uniView, 1, GL_FALSE, view.elements);
  glUniformMatrix4fv(uniProj, 1, GL_FALSE, proj.elements);
}

void
draw_line(DrawContext *debug_context, vec3f p1, vec3f p2, vec3f color) {

  GLint uniTrans = glGetUniformLocation(debug_context->shader_program, "trans");
  GLint uniColor =
      glGetUniformLocation(debug_context->shader_program, "inColor");

  mat4f transD = streach_from_to(p1, p2, 0.01);
  glUniformMatrix4fv(uniTrans, 1, GL_FALSE, transD.elements);
  glUniform3f(uniColor, color.x, color.y, color.z);
  int el_size = 6 * 6;
  glDrawElements(GL_TRIANGLES, el_size, GL_UNSIGNED_INT, 0);
};

void
draw_star(DrawContext *debug_context, vec3f p, float scale, vec3f color) {
  vec3f adds[3 * 3 * 3 - 1];
  int idx = 0;
  for (int i = 0; i < 3; ++i) {
    for (int j = 0; j < 3; ++j) {
      for (int k = 0; k < 3; ++k) {
        float x = (float)(i - 1);
        float y = (float)(j - 1);
        float z = (float)(k - 1);
        if (!(x == 0 && y == 0 && z == 0)) {
          adds[idx++] = vec3f{x, y, z};
        }
      }
    }
  }
  for (vec3f add : adds) {
    draw_line(debug_context, p, p + scale * add, color);
  }
};

struct UserInput {
  KeyState mouse_state = KeyState::KeyUp;
  int keys[3] = {GLFW_KEY_G, GLFW_KEY_R, GLFW_KEY_O};
  KeyState key_state[3] = {KeyState::KeyUp, KeyState::KeyUp, KeyState::KeyUp};
};

void
update_user_input(UserInput *user_input, GLFWwindow *window) {
  for (int i = 0; i < std::size(user_input->keys); ++i) {
    user_input->key_state[i] = update_kstate(
        user_input->key_state[i], glfwGetKey(window, user_input->keys[i]));
  }
}

KeyState
key_state(UserInput *user_input, int key) {
  for (int i = 0; i < std::size(user_input->keys); ++i) {
    if (user_input->keys[i] == key) {
      return user_input->key_state[i];
    }
  }
  assert(false);
  return KeyState::KeyUp;
}

void
render(GLFWwindow *window) {

  DrawContext overlay_context;
  DrawContext debug_context;
  DrawContext cube_context;

  GLuint vaos[3];
  glGenVertexArrays(3, vaos);
  overlay_context.vao = vaos[0];
  cube_context.vao = vaos[1];
  debug_context.vao = vaos[2];

  debug_context.shader_program = glCreateProgram();
  overlay_context.shader_program = glCreateProgram();
  cube_context.shader_program = glCreateProgram();

  glBindVertexArray(overlay_context.vao);
  GLuint overlay_texture;

  //================================================================================
  //                                   Definitions
  //================================================================================

  // Define Overlay texture
  {
    glGenTextures(1, &overlay_texture);
    glBindTexture(GL_TEXTURE_2D, overlay_texture);
    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    // clang-format off
    float s = 0.7;
    float attrs[] = {
	//verts        //tex coord
        -1.0F , 1.0F ,     0.0F, 1.0F,
        -1 + s, 1.0F ,     1.0F, 1.0F,
        -1 + s, 1 - s,     1.0F, 0.0F,
        -1.0F , 1 - s,     0.0F, 0.0F
    };

    GLuint elements[] = {
	    0, 1, 2,
	    2, 3, 0
    };
    // clang-format on
    {
      GLuint arr[2];
      glGenBuffers(2, arr);
      GLuint vbo = arr[0];
      GLuint ebo = arr[1];

      glBindBuffer(GL_ARRAY_BUFFER, vbo);
      glBufferData(GL_ARRAY_BUFFER, sizeof(attrs), attrs, GL_STATIC_DRAW);

      glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
      glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(elements), elements,
                   GL_STATIC_DRAW);
    }
    GLuint vertex_shader = compile_shader(R"glsl(
        #version 150 core

        in vec2 position;
        in vec2 uv;

        out vec2 Texcoord;
        void
        main() {
          gl_Position = vec4(position, 0.0, 1.0);
          Texcoord = uv;
        }
    )glsl",
                                          GL_VERTEX_SHADER);

    GLuint fragment_shader = compile_shader(R"glsl(
        #version 150 core
        out vec4 outColor;
        in vec2 Texcoord;

        uniform sampler2D tex;

        void
        main() {
          vec4 c = texture(tex, Texcoord);
	  c.r /= 2.0;
          outColor = vec4(c.r, c.r, c.r, 1.0);
        }
    )glsl",
                                            GL_FRAGMENT_SHADER);

    glAttachShader(overlay_context.shader_program, vertex_shader);
    glAttachShader(overlay_context.shader_program, fragment_shader);
    glBindFragDataLocation(overlay_context.shader_program, 0, "outColor");
    glLinkProgram(overlay_context.shader_program);

    GLuint pos_attrib =
        glGetAttribLocation(overlay_context.shader_program, "position");
    GLuint coord_attrib =
        glGetAttribLocation(overlay_context.shader_program, "uv");
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

  // clang-format on

  VertsContent verts_content;
  constexpr size_t n_verts = 6 * 4;
  vec3f c_verts[n_verts];
  vec2f c_coords[n_verts];
  vec3f c_normals[n_verts];

  verts_content.size = n_verts;
  verts_content.verts = c_verts;
  verts_content.normals = c_normals;

  GLuint cube_elements[6 * 6];
  {
    int ver_out = 0;
    int el_out = 0;
    int rect_num = 0;
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
	out float Height;

        uniform mat4 trans;
        uniform mat4 view;
        uniform mat4 proj;

        void
        main() {
	  vec4 pos_t = trans * vec4(position, 1.0);
          gl_Position = proj * view * trans * vec4(position, 1.0);
          FragPos = vec3(trans * vec4(position, 1.0));
          Normal = mat3(trans) * normal;
	  Height = pos_t.y;
        }
    )glsl";

    const char *new_fragment_source = R"glsl(
        #version 150 core

	in vec3 Normal;
	in vec3 FragPos;
	in float Height;

	out vec4 outColor;

	uniform bool debug;

	void
	main() {
	  vec4 black = vec4(0.2, 0.1, 0.1, 1.0);
	  vec4 green = vec4(0.0, 1.0, 0.0, 1.0);
	  vec4 blue = vec4(0.0, 0.0, 1.0, 1.0);
	  vec4 red = vec4(1.0, 0.4, 0.4, 1.0);

	  float th = 0.5;
	  if (Height < 0) {
	    outColor = mix(blue, black, -Height * 10);
	  } else if (Height <= th) {
	    outColor = mix(blue, green, Height / th);
	  } else {
	    outColor = mix(green, red, (Height - th) / th);
	  }

	  vec3 lightPos = vec3(0, 500, 400);
	  vec3 lightDir = normalize(lightPos - FragPos);
	  vec3 norm = normalize(Normal);
	  float diff = max(dot(norm, lightDir), 0.0);

	  outColor *= min(0.1 + diff, 1.0);

	  if (debug) {
	    outColor *= 0.5;
	  }
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
  // Define debug lines
  {
    glUseProgram(debug_context.shader_program);
    glBindVertexArray(vaos[2]);

    // clang-format on
    GLuint arr[2];
    glGenBuffers(2, arr);

    {
      float attrs[n_verts * 3];
      for (int i = 0; i < n_verts; ++i) {
        attrs[i * 3 + 0] = verts_content.verts[i].x;
        attrs[i * 3 + 1] = verts_content.verts[i].y;
        attrs[i * 3 + 2] = verts_content.verts[i].z;
      }
      GLuint vbo = arr[0];
      glBindBuffer(GL_ARRAY_BUFFER, vbo);
      glBufferData(GL_ARRAY_BUFFER, sizeof(attrs), attrs, GL_STATIC_DRAW);
    }

    {
      GLuint ebo = arr[1];
      glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
      glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(cube_elements),
                   cube_elements, GL_STATIC_DRAW);
    }
    GLuint vertex_shader = compile_shader(R"glsl(
            #version 150 core
            in vec3 position;

            uniform mat4 trans;
            uniform mat4 view;
            uniform mat4 proj;

            void
            main() {
              gl_Position = proj * view * trans * vec4(position, 1.0);
            }
	)glsl",
                                          GL_VERTEX_SHADER);

    GLuint fragment_shader = compile_shader(R"glsl(
            #version 150 core

            uniform vec3 inColor;
	    out vec4 outColor;
            void
            main() {
              outColor = vec4(inColor, 1.0);
            }
        )glsl",
                                            GL_FRAGMENT_SHADER);

    glAttachShader(debug_context.shader_program, vertex_shader);
    glAttachShader(debug_context.shader_program, fragment_shader);
    glBindFragDataLocation(debug_context.shader_program, 0, "outColorי");
    glLinkProgram(debug_context.shader_program);

    switch_to_context(&debug_context);
    GLuint pos_attrib =
        glGetAttribLocation(debug_context.shader_program, "position");
    int attr_size = sizeof(float) * 3;
    glVertexAttribPointer(pos_attrib, 3, GL_FLOAT, GL_FALSE, attr_size, 0);
    glEnableVertexAttribArray(pos_attrib);
  }

  {
    size_t el_size = std::size(cube_elements);
    GLint uniTrans = glGetUniformLocation(cube_context.shader_program, "trans");
    GLuint uniView = glGetUniformLocation(cube_context.shader_program, "view");
    GLint uniProj = glGetUniformLocation(cube_context.shader_program, "proj");
    GLint uniDebug = glGetUniformLocation(cube_context.shader_program, "debug");

    time_point t_start = now();
    time_point t_prev = now();

    float sign = 1;
    float scale_f = 5.0F;
    float rot_f = 0.1;

    int terrain_width = 90;
    float *terrain_vals =
        (float *)malloc(sizeof(float *) * terrain_width * terrain_width);

    bool debug_overlay = false;
    bool overlay_texture = false;
    UserInput user_input;
    KeyState mouse_state = KeyState::KeyUp;

    WaveState wave_state = WaveState::Simulate;
    int wave_row = -1;
    int wave_col = -1;
    float wave_base_height = 0;
    Wave waves[100];
    int n_waves = 0;
    srand(time(nullptr));

    // Star collection demo params
    int n_pts = 15;
    bool collected[n_pts];
    for (int i = 0; i < n_pts; ++i) {
      collected[i] = false;
    }

    while (!glfwWindowShouldClose(window)) {

      // User input

      double mouse_x, mouse_y;
      glfwGetCursorPos(window, &mouse_x, &mouse_y);
      mouse_x = mouse_x / (screen_width * 0.5F) - 1.0F;
      mouse_y = mouse_y / (screen_height * 0.5F) - 1.0F;

      update_user_input(&user_input, window);

      mouse_state = update_kstate(
          mouse_state, glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_1));

      if (key_state(&user_input, GLFW_KEY_G) == KeyState::KeyPressed) {
        debug_overlay = !debug_overlay;
      }

      if (key_state(&user_input, GLFW_KEY_O) == KeyState::KeyPressed) {
        overlay_texture = !overlay_texture;
      }

      if (key_state(&user_input, GLFW_KEY_R) == KeyState::KeyPressed) {
        n_waves = 0;
        for (int i = 0; i < n_pts; ++i) {
          collected[i] = false;
        }
      }

      // Wave control
      {
        if (wave_state == WaveState::Add) {
          wave_state = WaveState::Adding;
        }
        if (wave_state == WaveState::DoneAdding) {
          wave_state = WaveState::Simulate;
        }
        if (mouse_state == KeyState::KeyPressed ||
            mouse_state == KeyState::KeyDown) {
          if (wave_state == WaveState::Simulate) {
            wave_state = WaveState::Add;
          }
        } else {
          if (wave_state == WaveState::Adding) {
            wave_state = WaveState::DoneAdding;
          }
        }
      }

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

      time_point t_now = now();
      float time = time_between(t_start, t_now);
      float time_delta = time_between(t_prev, t_now);
      t_prev = t_now;

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

      // Camera ray
      vec3f dir;
      vec3f ray_normal;
      {
        mat4f view2 = copy(&view);
        view2.elements[12] = 0;
        view2.elements[13] = 0;
        view2.elements[14] = 0;
        mat4f invVP = inverse(proj * view2);
        vec3f screen_pos{(float)mouse_x, -(float)mouse_y, 1.0F};
        dir = invVP * screen_pos;
        dir = normalized(dir);

        vec3f cam_x = {view2.elements[4 * 0 + 0], view2.elements[4 * 1 + 0],
                       view2.elements[4 * 2 + 0]};

        ray_normal = normalized(cross(dir, cam_x));
      }

      init_debug_draw(&debug_context, view, proj);

      glClearColor(0.0F, 0.0F, 0.0F, 1.0F);
      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

      float scale = 6.0F;
      mat4f scale_mat = diagonal(scale, 1.0F, scale, 1);

      float w_pix = 1.0F / terrain_width;

      // Define terrain
      for (int i = 0; i < terrain_width * terrain_width; ++i) {
        terrain_vals[i] = 0;
      }
      int mirror_row = (int)(0.90 * terrain_width);

      int hero_row = 10;
      int hero_col = 20;
      // mirror_row = terrain_width;
      for (int row = 0; row < mirror_row; ++row) {
        for (int col = 0; col < terrain_width; ++col) {
          float acc_val = 0;
          for (int step = 0; step < 2; ++step) {

            float row_norm = (float)row / terrain_width;
            float col_norm = (float)col / terrain_width;

            if (step == 1) {
              row_norm = (float)(mirror_row + mirror_row - row) / terrain_width;
              col_norm = (float)col / terrain_width;
            }

            for (int i = 0; i < n_waves; ++i) {
              auto wave = waves[i];
              float r = sqrtf(powf(col_norm - wave.x, 2) +
                              powf(row_norm - wave.y, 2));
              float tt = wave.time * wave.speed;

              float repititions = 4;

              float wave_place = r * pi * repititions - tt;
	      

              if (-0.5 * pi <= wave_place && wave_place <= 1.5 * pi) {

                float cos_w = cosf(wave_place);

                if (wave_place > pi || wave_place < 0) {
                  cos_w = powf(cos_w, 3);
                }
                if (cos_w < 0) {
                  cos_w /= 6;
                }

                float val_f = wave.size * (cos_w);
                acc_val += val_f;
              }
            }
          }
          if ((pow(hero_row - row, 2) + pow(hero_col - col, 2)) >= 25) {
            terrain_vals[row * terrain_width + col] = acc_val;
          } else {
            terrain_vals[row * terrain_width + col] = 1;
          }
        }
      }

      for (int i = 0; i < n_waves; ++i) {
        waves[i].time += time_delta;
      }

      glEnable(GL_DEPTH_TEST);

      // Find chosen terrain box

      // Mouse choose box
      int chosen_row = -1;
      int chosen_col = -1;
      float max_score = -1;
      for (int row = 0; row < terrain_width; ++row) {
        for (int col = 0; col < terrain_width; ++col) {
          float row_norm = (float)row / terrain_width;
          float col_norm = (float)col / terrain_width;
          float acc_val = terrain_vals[row * terrain_width + col];
          vec3f pppos{row_norm - 0.5F, acc_val, col_norm - 0.5F};
          pppos = scale_mat * pppos;
          vec3f cam2pos = normalized(pppos - cam_pos);
          float score = dot(cam2pos, dir);
          if (score > max_score) {
            max_score = score;
            chosen_row = row;
            chosen_col = col;
          }
          score = powf(score, 500);

          if (debug_overlay) {
            draw_line(&debug_context, pppos, pppos + vec3f{0, score, 0},
                      vec3f{0.8, 0.9, 0.6});
          }
        }
      }

      // Draw mirror wall
      {
        float row_norm = (float)mirror_row / terrain_width;
        for (int col = 0; col < terrain_width; ++col) {
          float col_norm = (float)col / terrain_width;
          vec3f pppos{row_norm - 0.5F, 0, col_norm - 0.5F};
          pppos = scale_mat * pppos;
          vec3f addy{0, 2, 0};
          draw_line(&debug_context, pppos, pppos + addy, vec3f{1, 0, 0});
        }
      }

      // Draw hero
      {
        float row_norm = (float)hero_row / terrain_width;
        float col_norm = (float)hero_col / terrain_width;
        vec3f pppos{row_norm - 0.5F, 0.3, col_norm - 0.5F};
        pppos = scale_mat * pppos;
        draw_star(&debug_context, pppos, 0.1, vec3f{1, 0.5, 0.5});
        draw_star(&debug_context, pppos + vec3f{0, -0.3, 0}, 0.1,
                  vec3f{1, 0.5, 0.5});
      }
      // Draw lines for demo
      {
        vec3f p0{0, 0.5, 0};
        vec3f addy{0, 0.3, 0};
        float radius = 2;
        for (int i = 0; i < n_pts; ++i) {
          float t = 2 * pi * (float)i / (float)n_pts;
          vec3f c_dir = vec3f{cosf(t), 0, sinf(t)};
          draw_line(&debug_context, p0, p0 + radius * c_dir, vec3f{1, 1, 1});
          draw_line(&debug_context, vec3f{0, 0, 0}, radius * c_dir,
                    vec3f{0, 0, 0});
          int m = 2;

          vec3f star_pos = p0 + addy + 0.7 * radius * c_dir;

          float x = star_pos.x;
          float z = star_pos.z;

          int row = (int)((x / scale + 0.5) * terrain_width);
          int col = (int)((z / scale + 0.5) * terrain_width);
          float acc_val = terrain_vals[row * terrain_width + col];

          if (acc_val > star_pos.y) {
            collected[i] = true;
          }

          if (!collected[i]) {
            vec3f star_color =
                collected[i] ? vec3f{0.7, 0.7, 0.7} : vec3f{0.8, 0.8, 0.0};
            draw_star(&debug_context, star_pos, 0.03, star_color);
            star_pos.y = 0;
            draw_star(&debug_context, star_pos, 0.1, vec3f{0, 0, 0});
          }
        }
      }

      // Draw terrain
      switch_to_context(&cube_context);
      for (int row = 0; row < terrain_width; ++row) {
        for (int col = 0; col < terrain_width; ++col) {
          float row_norm = (float)row / terrain_width;
          float col_norm = (float)col / terrain_width;
          float acc_val = terrain_vals[row * terrain_width + col];
          mat4f trans = diagonal(w_pix, 2, w_pix, 1);
          trans.elements[12] = row_norm - 0.5;
          trans.elements[13] = acc_val - 1;
          trans.elements[14] = col_norm - 0.5;

          trans = scale_mat * trans;
          glUniformMatrix4fv(uniTrans, 1, GL_FALSE, trans.elements);
          glUniform1i(uniDebug, debug_overlay);

          if (row == chosen_row && col == chosen_col) {
            if (wave_state == WaveState::Add) {
              waves[n_waves++] = Wave{.x = col_norm,
                                      .y = row_norm,
                                      .speed = 0,
                                      .size = 0,
                                      .time = 0};
              wave_row = row;
              wave_col = col;
              wave_base_height = acc_val;
            }
          }
          bool is_chosen = false;
          {
            int s = 2;
            for (int add_row = -s; add_row <= s; ++add_row) {
              for (int add_col = -s; add_col <= s; ++add_col) {
                if ((row + add_row) == chosen_row &&
                    (col + add_col) == chosen_col) {
                  is_chosen = true;
                }
              }
            }
          }

          glDrawElements(GL_TRIANGLES, el_size, GL_UNSIGNED_INT, 0);
        }
      }
      if (wave_state == WaveState::Adding) {
        Wave *last = &waves[n_waves - 1];
        int row = wave_row;
        int col = wave_col;
        float row_norm = (float)wave_row / terrain_width;
        float col_norm = (float)wave_col / terrain_width;

        vec3f pppos{row_norm - 0.5F, wave_base_height, col_norm - 0.5F};
        pppos = scale_mat * pppos;
        vec3f pos2cam = cam_pos - pppos;

        float b_length =
            dot(pos2cam, ray_normal) / dot(vec3f{0, 1, 0}, ray_normal);

        last->size = clamp(b_length, -0.8, 0.8);
      }

      if (wave_state == WaveState::DoneAdding) {
        float s = waves[n_waves - 1].size;
        float speed = 5 * powf(fabs(s), 1.5);
        if (s < 0) {
          speed *= -1;
        }
        waves[n_waves - 1].speed = speed;
        waves[n_waves - 1].time = 0;
      }

      // Draw overlay texture
      if (overlay_texture) {
        for (int i = 0; i < terrain_width * terrain_width; ++i) {
          terrain_vals[i] += 0.5;
          terrain_vals[i] /= 3;
        }

        glDisable(GL_DEPTH_TEST);
        switch_to_context(&overlay_context);
        glBindTexture(GL_TEXTURE_2D, overlay_texture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, terrain_width, terrain_width, 0,
                     GL_RED, GL_FLOAT, terrain_vals);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
      }

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
main(int argc, char **argv) {

  GLFWwindow *window = open_window(screen_width, screen_height);
  render(window);

  glfwDestroyWindow(window);
  glfwTerminate();
  return 0;
}
