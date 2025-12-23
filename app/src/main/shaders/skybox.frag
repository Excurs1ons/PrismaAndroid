#version 450

// Skybox片段着色器
// 使用samplerCube采样立方体贴图

layout(binding = 1) uniform samplerCube cubemap;

layout(location = 0) in vec3 fragTexCoord;

layout(location = 0) out vec4 outColor;

void main() {
    // 使用插值后的方向向量直接采样cubemap
    outColor = texture(cubemap, fragTexCoord);
}
