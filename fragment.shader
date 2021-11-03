#version 150 core
uniform float inter;

in vec2 Texcoord;

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
}
