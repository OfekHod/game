#include "math.hpp"
#include "stdio.h"
#include <cmath>

mat4f
identity() {
  mat4f res;
  for (int r = 0; r < 4; ++r) {
    for (int c = 0; c < 4; ++c) {
      if (r != c) {
        res.elements[r * 4 + c] = 0;
      } else {
        res.elements[r * 4 + c] = 1;
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
  for (int r = 0; r < 4; ++r) {
    for (int c = 0; c < 4; ++c) {
      if (r != c) {
        res.elements[r * 4 + c] = 0;
      } else {
        res.elements[r * 4 + c] = diag[diag_idx++];
      }
    }
  }

  return res;
}

void
print(mat4f m) {
  for (int row = 0; row < 4; ++row) {
    for (int col = 0; col < 3; ++col) {
      printf("%f ", m.elements[row * 4 + col]);
    }
    printf("%f\n", m.elements[row * 4 + 3]);
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
	  cos_t + x * x * m_cos_t, x * y * m_cos_t - z * sin_t, x * z * m_cos_t + y * sin_t, 0,
	  y * x * m_cos_t + z * sin_t, cos_t + y * y * m_cos_t, y * z * m_cos_t - x * sin_t, 0,
	  z * x * m_cos_t - y * sin_t, z * y * m_cos_t + x * sin_t, cos_t + z * z * m_cos_t, 0,
	  0, 0, 0, 1
  }};
  // clang-format on
}

mat4f
operator*(mat4f m1, mat4f m2) {
  mat4f res;
  for (int res_row = 0; res_row < 4; ++res_row) {
    for (int res_col = 0; res_col < 4; ++res_col) {
      float *pos = &res.elements[res_row * 4 + res_col];
      *pos = 0;
      for (int a = 0; a < 4; ++a) {
        *pos += m1.elements[res_row * 4 + a] * m2.elements[a * 4 + res_col];
      }
    }
  }
  return res;
}
