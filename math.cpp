#include "math.hpp"
#include "stdio.h"
#include <cmath>
#include <cstdint>

/*
   To conform with OpenGL conventions, matrices are saved column first
*/
mat4f
identity() {
  mat4f res;
  for (int8_t col = 0; col < 4; ++col) {
    for (uint8_t row = 0; row < 4; ++row) {
      if (col != row) {
        res.elements[col * 4 + row] = 0;
      } else {
        res.elements[col * 4 + row] = 1;
      }
    }
  }
  return res;
}

mat4f
diagonal(float a, float b, float c, float d) {
  const float diag[4] = {a, b, c, d};
  mat4f res;
  uint8_t diag_idx = 0;
  for (uint8_t col = 0; col < 4; ++col) {
    for (uint8_t row = 0; row < 4; ++row) {
      if (col != row) {
        res.elements[col * 4 + row] = 0;
      } else {
        res.elements[col * 4 + row] = diag[diag_idx++];
      }
    }
  }

  return res;
}

void
print(mat4f m) {
  for (uint8_t row = 0; row < 4; ++row) {
    uint8_t col = 0;
    for (col = 0; col < 3; ++col) {
      printf("%f\t", m.elements[col * 4 + row]);
    }
    printf("%f\n", m.elements[col * 4 + row]);
  }
}

float
len(vec3f v) {
  return sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
}

vec3f
normalized(vec3f v) {
  float v_len = len(v);
  return vec3f{v.x / v_len, v.y / v_len, v.z / v_len};
}

mat4f
rotation(vec3f u, float theta) {
  u = normalized(u);
  float cos_t = cos(theta);
  float sin_t = sin(theta);
  float m_cos_t = 1.0F - cos_t;
  float x = u.x;
  float y = u.y;
  float z = u.z;
  // clang-format off
  return mat4f{{
    cos_t + x * x * m_cos_t    , y * x * m_cos_t + z * sin_t, z * x * m_cos_t - y * sin_t, 0.0F,
    x * y * m_cos_t - z * sin_t, cos_t + y * y * m_cos_t    , z * y * m_cos_t + x * sin_t, 0.0F,
    x * z * m_cos_t + y * sin_t, y * z * m_cos_t - x * sin_t, cos_t + z * z * m_cos_t    , 0.0F,
    0.0F                       , 0.0F                       , 0.0F                       , 1.0F
  }};
  // clang-format on
}

vec3f
operator-(vec3f v1, vec3f v2) {
  return vec3f{v1.x - v2.x, v1.y - v2.y, v1.z - v2.z};
}

vec3f
operator+(vec3f v1, vec3f v2) {
  return vec3f{v1.x + v2.x, v1.y + v2.y, v1.z + v2.z};
}

vec3f
operator-(vec3f v) {
  return vec3f{-v.x, -v.y, -v.z};
}

vec3f
operator*(float scalar, vec3f v) {
  return vec3f{scalar * v.x, scalar * v.y, scalar * v.z};
}

float
dot(vec3f u, vec3f v) {
  return u.x * v.x + u.y * v.y + u.z * v.z;
}

// vx vy vz
// ux uy uz
vec3f
cross(vec3f v, vec3f u) {
  return vec3f{v.y * u.z - v.z * u.y, v.z * u.x - v.x * u.z,
               v.x * u.y - v.y * u.x};
}

mat4f
operator*(const mat4f &m1, const mat4f &m2) {
  mat4f res;
  for (uint8_t res_col = 0; res_col < 4; ++res_col) {
    for (uint8_t res_row = 0; res_row < 4; ++res_row) {
      float *pos = &res.elements[res_col * 4 + res_row];
      *pos = 0;
      for (uint8_t a = 0; a < 4; ++a) {
        *pos += m1.elements[a * 4 + res_row] * m2.elements[res_col * 4 + a];
      }
    }
  }
  return res;
}

mat4f
look_at(vec3f eye, vec3f center, vec3f up) {
  vec3f forward = normalized(center - eye);
  vec3f side = normalized(cross(forward, up));
  up = cross(side, forward);

  float t_x = -dot(eye, side);
  float t_y = -dot(eye, up);
  float t_z = -dot(eye, -forward);

  // clang-format off
  return mat4f{{
    side.x, up.x, -forward.x, 0.0F,
    side.y, up.y, -forward.y, 0.0F,
    side.z, up.z, -forward.z, 0.0F,
    t_x   , t_y , t_z       , 1.0F
  }};

  // clang-format on
}

mat4f
perspective(float fov, float aspect, float near, float far) {
  float tan_half = tanf(fov / 2.0F);

  // clang-format off
  return mat4f{{
    1 / (aspect * tan_half), 0           , 0                               , 0,
    0                      , 1 / tan_half, 0                               , 0,
    0                      , 0           , -(far + near) / (far - near)    , -1,
    0                      , 0           , -(2 * far * near) / (far - near), 0
  }};
  // clang-format on
}

mat4f
streach_from_to(vec3f p1, vec3f p2, float thikness) {
  vec3f new_y = p2 - p1;
  float new_len = len(new_y);
  vec3f new_x = cross(new_y, vec3f{0, 1, 0});
  vec3f new_z = cross(new_x, new_y);
  new_y = cross(new_z, new_x);

  new_y = normalized(new_y);
  new_x = normalized(new_x);
  new_z = normalized(new_z);

  // clang-format off
  mat4f rot = mat4f{{
	  new_x.x, new_x.y, new_x.z, 0.0,
	  new_y.x, new_y.y, new_y.z, 0.0,
	  new_z.x, new_z.y, new_z.z, 0.0,
	  0.0    , 0.0    , 0.0    , 1.0
  }};
  // clang-format on

  vec3f middle = p1 + 0.5 * (p2 - p1);
  mat4f trans = diagonal(1.0, 1.0, 1.0, 1.0);
  trans.elements[12] = middle.x;
  trans.elements[13] = middle.y;
  trans.elements[14] = middle.z;

  return trans * rot * diagonal(thikness, new_len, thikness, 1.0);
}

vec3f
operator*(const mat4f &m, const vec3f &v) {
  const float* me = m.elements;
  return vec3f{me[0 * 4 + 0] * v.x + me[1 * 4 + 0] * v.y + me[2 * 4 + 0] * v.z +
                   me[3 * 4 + 0],
               me[0 * 4 + 1] * v.x + me[1 * 4 + 1] * v.y + me[2 * 4 + 1] * v.z +
                   me[3 * 4 + 1],
               me[0 * 4 + 2] * v.x + me[1 * 4 + 2] * v.y + me[2 * 4 + 2] * v.z +
                   me[3 * 4 + 2]};
}
