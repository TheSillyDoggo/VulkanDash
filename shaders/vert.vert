#version 450
layout(location = 0) in vec3 inPos;
layout(location = 1) in vec4 inColor;
layout(location = 2) in vec2 inUV;

layout(push_constant) uniform PushConst {
    mat4 mvp;
} pc;

layout(location = 0) out vec4 fragColor;
layout(location = 1) out vec2 fragUV;

void main() {
    gl_Position = pc.mvp * vec4(inPos, 1.0);
    fragColor = inColor;
    fragUV = vec2(inUV.x, inUV.y);
}