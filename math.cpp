#include "math.hpp"
#include "stdio.h"
#include <cmath>

/*
   To conform with OpenGL conventions, matrices are saved column first
*/
mat4f
identity() {
  mat4f res;
  for (int col = 0; col < 4; ++col) {
    for (int row = 0; row < 4; ++row) {
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
  int diag_idx = 0;
  for (int col = 0; col < 4; ++col) {
    for (int row = 0; row < 4; ++row) {
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
  for (int col = 0; col < 4; ++col) {
    for (int row = 0; row < 3; ++row) {
      printf("%f\t", m.elements[col * 4 + row]);
    }
    printf("%f\n", m.elements[col * 4 + 3]);
  }
}

mat4f
rotation(vec3f u, float theta) {
  float cos_t = cos(theta);
  float sin_t = sin(theta);
  float m_cos_t = 1.0F - cos_t;
  float x = u.x;
  float y = u.y;
  float z = u.z;
  // clang-format off
  return mat4f{{
	  cos_t + x * x * m_cos_t,     y * x * m_cos_t + z * sin_t, z * x * m_cos_t - y * sin_t, 0,
	  x * y * m_cos_t - z * sin_t, cos_t + y * y * m_cos_t,     z * y * m_cos_t + x * sin_t     , 0,
	  x * z * m_cos_t + y * sin_t, y * z * m_cos_t - x * sin_t, cos_t + z * z * m_cos_t,     0,
	  0, 0, 0, 1
  }};
  // clang-format on
}

vec3f
operator-(vec3f v1, vec3f v2) {
  return vec3f{v1.x - v2.x, v1.y - v2.y, v1.z - v2.z};
}

vec3f
operator-(vec3f v) {
  return vec3f{-v.x, -v.y, -v.z};
}

float
dot(vec3f u, vec3f v) {
  return u.x * v.x + u.y * v.y + u.z * v.z;
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
  for (int res_col = 0; res_col < 4; ++res_col) {
    for (int res_row = 0; res_row < 4; ++res_row) {
      float *pos = &res.elements[res_col * 4 + res_row];
      *pos = 0;
      for (int a = 0; a < 4; ++a) {
        *pos += m1.elements[res_col * 4 + a] * m2.elements[a * 4 + res_row];
      }
    }
  }
  return res;
}

// https://www.khronos.org/opengl/wiki/GluLookAt_code
mat4f
look_at(vec3f eye, vec3f center, vec3f up) {
  vec3f forward = normalized(center - eye);
  vec3f side = normalized(cross(forward, up));
  up = cross(side, forward);

  // clang-format off
  return mat4f{{
    side.x,          up.x,          -forward.x,          0.0F,
    side.y,          up.y,          -forward.y,          0.0F,
    side.z,          up.z,          -forward.z,          0.0F,
    -dot(eye, side), -dot(eye, up), -dot(eye, -forward), 1.0F
  }};

  // clang-format on
}
