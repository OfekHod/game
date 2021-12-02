#define GLEW_STATIC
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "math.hpp"

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

constexpr int screen_width = 800;
constexpr int screen_height = 800;

void
render(GLFWwindow *window) {

  GLuint debug_line_program = glCreateProgram();

  GLuint vaos[3];
  glGenVertexArrays(3, vaos);
  glBindVertexArray(vaos[0]);
  GLuint overlay_shader_program = glCreateProgram();
  GLuint overlay_texture;
  glGenTextures(1, &overlay_texture);

  // Define Overlay texture
  {
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

    Shader fragment_shader = Shader(R"glsl(
        #version 150 core
        out vec4 outColor;
        in vec2 Texcoord;

        uniform sampler2D tex;

        void
        main() {
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
  constexpr size_t n_verts = 6 * 4;
  vec3f c_verts[n_verts];
  vec2f c_coords[n_verts];
  vec3f c_normals[n_verts];

  verts_content.size = n_verts;
  verts_content.verts = c_verts;
  verts_content.texcoords = c_coords;
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
        verts_content.texcoords[ver_out] = tex_c[j];
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
  GLuint cube_shader_program = glCreateProgram();
  {
    const char *new_vertex_source = R"glsl(
        #version 150 core

        in vec3 position;
        in vec2 texcoord;
        in vec3 normal;

        out vec2 Texcoord;
        out vec3 FragPos;
        out vec3 Normal;

        uniform mat4 trans;
        uniform mat4 view;
        uniform mat4 proj;

        void
        main() {
          Texcoord = texcoord;
          gl_Position = proj * view * trans * vec4(position, 1.0);
          FragPos = vec3(trans * vec4(position, 1.0));
          Normal = mat3(trans) * normal;
        }
    )glsl";

    const char *new_fragment_source = R"glsl(
        #version 150 core

        in vec2 Texcoord;
        in vec3 Normal;
        in vec3 FragPos;

        out vec4 outColor;

        uniform sampler2D tex;
	uniform bool debug;
	uniform bool chosen;

        void
        main() {
          vec4 color_tex = texture(tex, Texcoord);
          outColor = color_tex;

	  if (chosen) {
	    outColor = vec4(0.0, 0.0, 1.0, 1.0);
	  }

          vec3 lightPos = vec3(30, 50, 10);
          vec3 lightDir = normalize(lightPos - FragPos);
          vec3 norm = normalize(Normal);
          float diff = max(dot(norm, lightDir), 0.0);

          outColor *= min(0.3 + diff, 1.0);

	  if (debug) {
	    outColor *= 0.5;
	  }
        }
    )glsl";
    const Shader vertex_shader(new_vertex_source, GL_VERTEX_SHADER);
    const Shader fragment_shader(new_fragment_source, GL_FRAGMENT_SHADER);

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
  GLuint terrain_texture;
  glGenTextures(1, &terrain_texture);
  {

    size_t width = 2;
    size_t height = 2;
    // clang-format off
    uint8_t pixels[12] = {
	    0  , 255, 255,     255, 255, 0,
	    255, 0  , 255,     0, 0  , 255
    };
    // clang-format on

    glBindTexture(GL_TEXTURE_2D, terrain_texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB,
                 GL_UNSIGNED_BYTE, pixels);

    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  };
  // Debug lines
  {
    glUseProgram(debug_line_program);
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
    Shader vertex_shader = Shader(R"glsl(
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

    Shader fragment_shader = Shader(R"glsl(
            #version 150 core

            uniform vec3 inColor;
	    out vec4 outColor;
            void
            main() {
              outColor = vec4(inColor, 1.0);
            }
        )glsl",
                                    GL_FRAGMENT_SHADER);

    glAttachShader(debug_line_program, vertex_shader.gl_ptr);
    glAttachShader(debug_line_program, fragment_shader.gl_ptr);
    glBindFragDataLocation(debug_line_program, 0, "outColorי");
    glLinkProgram(debug_line_program);
    glUseProgram(debug_line_program);

    glBindVertexArray(vaos[2]);
    GLuint pos_attrib = glGetAttribLocation(debug_line_program, "position");
    int attr_size = sizeof(float) * 3;
    glVertexAttribPointer(pos_attrib, 3, GL_FLOAT, GL_FALSE, attr_size, 0);
    glEnableVertexAttribArray(pos_attrib);
  }

  {
    size_t el_size = std::size(cube_elements);
    GLint uniTrans = glGetUniformLocation(cube_shader_program, "trans");
    GLuint uniView = glGetUniformLocation(cube_shader_program, "view");
    GLint uniProj = glGetUniformLocation(cube_shader_program, "proj");
    GLint uniDebug = glGetUniformLocation(cube_shader_program, "debug");
    GLint uniChosen = glGetUniformLocation(cube_shader_program, "chosen");

    time_point t_start = now();

    float sign = 1;
    float scale_f = 1.2F;
    float rot_f = 0;

    int terrain_width = 50;
    float *terrain_vals =
        (float *)malloc(sizeof(float *) * terrain_width * terrain_width);

    bool debug_overlay = true;
    KeyState g_state = KeyState::KeyUp;

    while (!glfwWindowShouldClose(window)) {

      double mouse_x, mouse_y;
      glfwGetCursorPos(window, &mouse_x, &mouse_y);
      mouse_x = mouse_x / (screen_width * 0.5F) - 1.0F;
      mouse_y = mouse_y / (screen_height * 0.5F) - 1.0F;

      g_state = update_kstate(g_state, glfwGetKey(window, GLFW_KEY_G));

      if (g_state == KeyState::KeyPressed) {
        debug_overlay = !debug_overlay;
      }

      glBindTexture(GL_TEXTURE_2D, terrain_texture);
      glUseProgram(cube_shader_program);

      // Zoom in out
      {
        if (glfwGetKey(window, GLFW_KEY_W)) {
          scale_f -= 0.3;
        } else if (glfwGetKey(window, GLFW_KEY_S)) {
          scale_f += 0.3;
        }
        scale_f = fmaxf(scale_f, 2);
        scale_f = fminf(scale_f, 10);
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

      time_point t_now = now();
      float time = time_between(t_start, t_now);

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
                           0.5f, 50.F);

        glUniformMatrix4fv(uniProj, 1, GL_FALSE, proj.elements);
      };

      // Camera ray
      vec3f dir;
      {
        mat4f view2 = copy(&view);
        view2.elements[12] = 0;
        view2.elements[13] = 0;
        view2.elements[14] = 0;
        mat4f invVP = inverse(proj * view2);
        vec3f screen_pos{(float)mouse_x, -(float)mouse_y, 1.0F};
        dir = invVP * screen_pos;
        dir = normalized(dir);
      }

      // Inner draw fucntions
      auto draw_line = [&](vec3f p1, vec3f p2, vec3f color) {
        glUseProgram(debug_line_program);
        glBindVertexArray(vaos[2]);

        GLint uniTrans = glGetUniformLocation(debug_line_program, "trans");
        GLuint uniView = glGetUniformLocation(debug_line_program, "view");
        GLint uniProj = glGetUniformLocation(debug_line_program, "proj");
        GLint uniColor = glGetUniformLocation(debug_line_program, "inColor");

        mat4f transD = streach_from_to(p1, p2, 0.01);

        glUniformMatrix4fv(uniTrans, 1, GL_FALSE, transD.elements);
        glUniformMatrix4fv(uniView, 1, GL_FALSE, view.elements);
        glUniformMatrix4fv(uniProj, 1, GL_FALSE, proj.elements);
        glUniform3f(uniColor, color.x, color.y, color.z);

        glDrawElements(GL_TRIANGLES, el_size, GL_UNSIGNED_INT, 0);
      };

      auto draw_star = [&](vec3f p, float scale, vec3f color) {
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
          draw_line(p, p + scale * add, color);
        }
      };
      glClearColor(0.0F, 0.0F, 0.0F, 1.0F);
      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

      float scale = 6.0F;
      mat4f scale_mat = diagonal(scale, 1.0F, scale, 1);

      float w_pix = 1.0F / terrain_width;

      // Define terrain
      for (int row = 0; row < terrain_width; ++row) {
        for (int col = 0; col < terrain_width; ++col) {
          {
            float row_norm = (float)row / terrain_width;
            float col_norm = (float)col / terrain_width;

            vec3f centers[] = {{0.5, 0.5, 1.5},
                               {0.3, 0.7, 1.4},
                               {0.2, 0.3, 0.8},
                               {0.8, 0.2, 2.4},
                               {0.7, 0.8, 0.8}};

            float acc_val = 0;
            for (auto center : centers) {
              float r = sqrtf(powf(col_norm - center.x, 2) +
                              powf(row_norm - center.y, 2));
              float tt = time * center.z;

              float val_f = powf((1 - r), 7) *
                            (0.5F + 0.5F * cosf(r * pi * 20 - tt * 2) + 0.1F +
                             0.1F * sinf(r * pi * 60 + 10 + tt * 5));
              val_f = val_f * 0.8F;
              acc_val += val_f;
            }
            acc_val *= 0.5;
            terrain_vals[row * terrain_width + col] = acc_val;
          }
        }
      }

      glEnable(GL_DEPTH_TEST);
      glDepthRange(0, 0.01); // For overlay drawings
      {
	      // Here comes overlay drawings;
      }

      // Find chosen terrain box



      // Mouse shoose box
      glDepthRange(0.01, 1.0);
      int chosen_row = -1;
      int chosen_col = -1;
      float max_score = -1;
      for (int row = 0; row < terrain_width; ++row) {
        for (int col = 0; col < terrain_width; ++col) {
          float row_norm = (float)row / terrain_width;
          float col_norm = (float)col / terrain_width;
          float acc_val = terrain_vals[row * terrain_width + col];
          mat4f trans = diagonal(w_pix, acc_val, w_pix, 1);
	  
	  vec3f pppos{ row_norm - 0.5F, acc_val, col_norm - 0.5F };
	  pppos = scale_mat * pppos;

	  vec3f cam2pos = normalized(pppos - cam_pos);

	  float score = dot(cam2pos, dir);
	  if (score > max_score) {
		  max_score = score;
		  chosen_row = row;
		  chosen_col = col;
	  }
	  score = powf(score, 20);

	  if (debug_overlay){
	  draw_line(pppos, pppos + vec3f{0, score, 0}, vec3f{0.8, 0.9, 0.6});
	  }

        }
      }

      // Draw terrain
      glBindTexture(GL_TEXTURE_2D, terrain_texture);
      glUseProgram(cube_shader_program);
      glBindVertexArray(vaos[1]);
      for (int row = 0; row < terrain_width; ++row) {
        for (int col = 0; col < terrain_width; ++col) {
          float row_norm = (float)row / terrain_width;
          float col_norm = (float)col / terrain_width;
          float acc_val = terrain_vals[row * terrain_width + col];
          mat4f trans = diagonal(w_pix, acc_val, w_pix, 1);
          trans.elements[12] = row_norm - 0.5;
          trans.elements[13] = acc_val * 0.5F;
          trans.elements[14] = col_norm - 0.5;

          trans = scale_mat * trans;
          glUniformMatrix4fv(uniTrans, 1, GL_FALSE, trans.elements);
          glUniform1i(uniDebug, debug_overlay);

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
          // bool is_chosen = (row == chosen_row && col == chosen_col);
          glUniform1i(uniChosen, is_chosen);

          glDrawElements(GL_TRIANGLES, el_size, GL_UNSIGNED_INT, 0);
        }
      }

      // Draw overlay texture
      {
        glDisable(GL_DEPTH_TEST);
        glBindVertexArray(vaos[0]);
        glBindTexture(GL_TEXTURE_2D, overlay_texture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, terrain_width, terrain_width, 0,
                     GL_RED, GL_FLOAT, terrain_vals);
        glUseProgram(overlay_shader_program);
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
