#version 450

layout(location = 0) in vec3 in_pos;
layout(location = 1) in vec3 in_color;

layout(location = 0) out vec3 frag_color;

layout(binding = 0) uniform ubo_global {
	mat4 view;
	mat4 proj;
} global;

layout(binding = 1) uniform ubo_inst {
	mat4 model;
} inst;

void main() {
    gl_Position = global.proj * global.view * inst.model * vec4(in_pos, 1.0);
    frag_color = in_color;
}
