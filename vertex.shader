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
void main()
{
  Texcoord = texcoord;
  gl_Position = proj * view * trans * vec4(position, 1.0);
  FragPos = vec3(trans * vec4(position, 1.0));
  Normal = mat3(trans) * normal;
}
