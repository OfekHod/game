#version 150 core
uniform float inter;

in vec2 Texcoord;
in vec3 Normal;
in vec3 FragPos;

out vec4 outColor;

uniform sampler2D tex1;
uniform sampler2D tex2;

void main() {
  vec4 color_tex1 = texture(tex1, Texcoord);
  vec4 color_tex2 = texture(tex2, Texcoord);
  float a = 0.5 * clamp(0.5 + 0.5 * sin(10 * Texcoord.x + 5 + 10 * inter) +
                            0.5 + 0.5 * cos(10 * Texcoord.y + 5 + 10 * inter),
                        0, 2);
  outColor = mix(color_tex1, color_tex2, a);
  outColor = color_tex2; //vec4(0.123, 0.4, 0.1, 1.0);


  vec3 lightPos = vec3(30, 50, 10);
  vec3 lightDir = normalize(lightPos - FragPos);
  vec3 norm = normalize(Normal);
  float diff = max(dot(norm, lightDir), 0.0);

  outColor *= min(0.3 + diff, 1.0);


}
