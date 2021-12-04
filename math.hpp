#pragma once

struct mat4f {
  float elements[16];
};

struct vec4f {
  float x;
  float y;
  float z;
  float w;
};

struct vec3f {
  float x;
  float y;
  float z;
};

struct vec2f {
  float x;
  float y;
};


mat4f
identity();

mat4f
diagonal(float a, float b, float c, float d);

void
print(mat4f m);

mat4f
rotation(vec3f u, float theta);

mat4f
operator*(const mat4f &m1, const mat4f &m2);

vec3f
operator*(const mat4f &m, const vec3f &v);

constexpr float pi = 3.14159265358979323846F;
constexpr float deg2rad = pi / 180.0F;
constexpr float rad2deg = 180.0F / pi;

mat4f
look_at(vec3f eye, vec3f center, vec3f up);

mat4f
perspective(float fov, float aspect, float near, float far);

mat4f
streach_from_to(vec3f p1, vec3f p2, float thikness);

mat4f
inverse(mat4f mat);

vec3f
operator-(vec3f v1, vec3f v2);

vec3f
operator+(vec3f v1, vec3f v2);

vec3f
operator*(float scalar, vec3f v);

vec3f
normalized(vec3f v);

void transpose(mat4f *mat);

mat4f
copy(mat4f *m);

vec3f
cross(vec3f v, vec3f u);

float
len(vec3f v);

float
dot(vec3f u, vec3f v);

vec3f
operator-(vec3f v);
