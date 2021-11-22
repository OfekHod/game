#version 150 core
uniform float inter;

in vec2 Texcoord;
in vec3 Normal;
in vec3 FragPos;

out vec4 outColor;

uniform sampler2D tex;

void main() {
  vec4 color_tex = texture(tex, Texcoord);
  outColor = color_tex;


  vec3 lightPos = vec3(30, 50, 10);
  vec3 lightDir = normalize(lightPos - FragPos);
  vec3 norm = normalize(Normal);
  float diff = max(dot(norm, lightDir), 0.0);

  outColor *= min(0.3 + diff, 1.0);


}
