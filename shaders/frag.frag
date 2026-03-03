#version 450
layout(location = 0) in vec4 fragColor;
layout(location = 1) in vec2 fragUV;


layout(location = 0) out vec4 outColor;
layout(set = 0, binding = 0) uniform sampler2D texSampler;

void main() {
    vec4 texColor = texture(texSampler, fragUV);
    outColor = texColor * fragColor;
    // outColor = vec4(1.0 - texColor.x, 1.0 - texColor.y, 1.0 - texColor.z, 1.0) * fragColor;
}