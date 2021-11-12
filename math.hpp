#pragma once

struct mat4f {
  float elements[16];
};
struct vec3f {
	float x;
	float y;
	float z;
};

mat4f
identity();

mat4f
diagonal(float a, float b, float c, float d);

void print(mat4f m);
mat4f rotation(vec3f u, float theta);

mat4f operator*(mat4f m1, mat4f m2);
constexpr float pi = 3.14159265358979323846F;
