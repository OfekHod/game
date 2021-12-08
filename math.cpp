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
  vec3f new_x = cross(normalized(new_y), vec3f{0, 1, 0});
  vec3f new_z = cross(new_x, new_y);
  new_y = cross(new_z, new_x);

  if (len(new_x) < 0.001) {
    new_x = vec3f{1, 0, 0};
    new_y = vec3f{0, 1, 0};
    new_z = vec3f{0, 0, 1};
  } else {
    new_y = normalized(new_y);
    new_x = normalized(new_x);
    new_z = normalized(new_z);
  }

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
  const float *me = m.elements;
  float div = me[0 * 4 + 3] + me[1 * 4 + 3] + me[2 * 4 + 3] + me[3 * 4 + 3];

  vec3f res{me[0 * 4 + 0] * v.x + me[1 * 4 + 0] * v.y + me[2 * 4 + 0] * v.z +
                me[3 * 4 + 0],
            me[0 * 4 + 1] * v.x + me[1 * 4 + 1] * v.y + me[2 * 4 + 1] * v.z +
                me[3 * 4 + 1],
            me[0 * 4 + 2] * v.x + me[1 * 4 + 2] * v.y + me[2 * 4 + 2] * v.z +
                me[3 * 4 + 2]};
  res = (1.0F / div) * res;
  return res;
}

mat4f
inverse(mat4f mat) {
  mat4f inv;

  float *m = mat.elements;
  float *inv_m = inv.elements;

  float m00 = m[0 + 4 * 0];
  float m01 = m[0 + 4 * 1];
  float m02 = m[0 + 4 * 2];
  float m03 = m[0 + 4 * 3];
  float m10 = m[1 + 4 * 0];
  float m11 = m[1 + 4 * 1];
  float m12 = m[1 + 4 * 2];
  float m13 = m[1 + 4 * 3];
  float m20 = m[2 + 4 * 0];
  float m21 = m[2 + 4 * 1];
  float m22 = m[2 + 4 * 2];
  float m23 = m[2 + 4 * 3];
  float m24 = m[2 + 4 * 4];
  float m30 = m[3 + 4 * 0];
  float m31 = m[3 + 4 * 1];
  float m32 = m[3 + 4 * 2];
  float m33 = m[3 + 4 * 3];

  float *inv_m00 = &inv_m[0 + 4 * 0];
  float *inv_m01 = &inv_m[0 + 4 * 1];
  float *inv_m02 = &inv_m[0 + 4 * 2];
  float *inv_m03 = &inv_m[0 + 4 * 3];
  float *inv_m10 = &inv_m[1 + 4 * 0];
  float *inv_m11 = &inv_m[1 + 4 * 1];
  float *inv_m12 = &inv_m[1 + 4 * 2];
  float *inv_m13 = &inv_m[1 + 4 * 3];
  float *inv_m20 = &inv_m[2 + 4 * 0];
  float *inv_m21 = &inv_m[2 + 4 * 1];
  float *inv_m22 = &inv_m[2 + 4 * 2];
  float *inv_m23 = &inv_m[2 + 4 * 3];
  float *inv_m24 = &inv_m[2 + 4 * 4];
  float *inv_m30 = &inv_m[3 + 4 * 0];
  float *inv_m31 = &inv_m[3 + 4 * 1];
  float *inv_m32 = &inv_m[3 + 4 * 2];
  float *inv_m33 = &inv_m[3 + 4 * 3];

  float A2323 = m22 * m33 - m23 * m32;
  float A1323 = m21 * m33 - m23 * m31;
  float A1223 = m21 * m32 - m22 * m31;
  float A0323 = m20 * m33 - m23 * m30;
  float A0223 = m20 * m32 - m22 * m30;
  float A0123 = m20 * m31 - m21 * m30;
  float A2313 = m12 * m33 - m13 * m32;
  float A1313 = m11 * m33 - m13 * m31;
  float A1213 = m11 * m32 - m12 * m31;
  float A2312 = m12 * m23 - m13 * m22;
  float A1312 = m11 * m23 - m13 * m21;
  float A1212 = m11 * m22 - m12 * m21;
  float A0313 = m10 * m33 - m13 * m30;
  float A0213 = m10 * m32 - m12 * m30;
  float A0312 = m10 * m23 - m13 * m20;
  float A0212 = m10 * m22 - m12 * m20;
  float A0113 = m10 * m31 - m11 * m30;
  float A0112 = m10 * m21 - m11 * m20;

  float det = m00 * (m11 * A2323 - m12 * A1323 + m13 * A1223) -
              m01 * (m10 * A2323 - m12 * A0323 + m13 * A0223) +
              m02 * (m10 * A1323 - m11 * A0323 + m13 * A0123) -
              m03 * (m10 * A1223 - m11 * A0223 + m12 * A0123);
  det = 1 / det;

  *inv_m00 = det * (m11 * A2323 - m12 * A1323 + m13 * A1223);
  *inv_m01 = det * -(m01 * A2323 - m02 * A1323 + m03 * A1223);
  *inv_m02 = det * (m01 * A2313 - m02 * A1313 + m03 * A1213);
  *inv_m03 = det * -(m01 * A2312 - m02 * A1312 + m03 * A1212);
  *inv_m10 = det * -(m10 * A2323 - m12 * A0323 + m13 * A0223);
  *inv_m11 = det * (m00 * A2323 - m02 * A0323 + m03 * A0223);
  *inv_m12 = det * -(m00 * A2313 - m02 * A0313 + m03 * A0213);
  *inv_m13 = det * (m00 * A2312 - m02 * A0312 + m03 * A0212);
  *inv_m20 = det * (m10 * A1323 - m11 * A0323 + m13 * A0123);
  *inv_m21 = det * -(m00 * A1323 - m01 * A0323 + m03 * A0123);
  *inv_m22 = det * (m00 * A1313 - m01 * A0313 + m03 * A0113);
  *inv_m23 = det * -(m00 * A1312 - m01 * A0312 + m03 * A0112);
  *inv_m30 = det * -(m10 * A1223 - m11 * A0223 + m12 * A0123);
  *inv_m31 = det * (m00 * A1223 - m01 * A0223 + m02 * A0123);
  *inv_m32 = det * -(m00 * A1213 - m01 * A0213 + m02 * A0113);
  *inv_m33 = det * (m00 * A1212 - m01 * A0212 + m02 * A0112);

  return inv;
}

void
transpose(mat4f *mat) {
  float *m = mat->elements;
  for (int col = 0; col < 4; ++col) {
    for (int row = 0; row < col; ++row) {
      float tmp = m[col * 4 + row];
      m[col * 4 + row] = m[row * 4 + col];
      m[row * 4 + col] = tmp;
    }
  }
}

mat4f
copy(mat4f *m) {
  mat4f res;
  for (int i = 0; i < 16; ++i) {
    res.elements[i] = m->elements[i];
  }

  return res;
}

float
clamp(float x, float min_v, float max_v) {
  if (x > max_v) {
    return max_v;
  }

  if (x < min_v) {
    return min_v;
  }

  return x;
}
