#version 450

// Skybox顶点着色器
// 注意：为了去除view矩阵的平移分量，我们需要将view矩阵转换为mat3再转回mat4

layout(binding = 0) uniform UniformBufferObject {
    mat4 view;
    mat4 proj;
} ubo;

layout(location = 0) in vec3 inPosition;

layout(location = 0) out vec3 fragTexCoord;

void main() {
    // 去除view矩阵的平移分量，使skybox始终围绕相机
    // 通过将mat4转换为mat3再转回mat4来实现
    mat4 viewNoTranslation = mat4(mat3(ubo.view));

    vec4 clipPos = ubo.proj * viewNoTranslation * vec4(inPosition, 1.0);

    // 将深度设置为最大值（z/w = 1.0），确保skybox渲染在最远处
    // 这样可以避免其他物体被skybox遮挡
    gl_Position = clipPos.xyww;

    // 将位置坐标直接作为纹理坐标传递给片段着色器
    fragTexCoord = inPosition;
}
