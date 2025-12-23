#version 450

// 纯色渲染顶点着色器
// 渲染一个覆盖全屏的矩形

layout(location = 0) in vec2 inPosition;

void main() {
    // 直接输出位置，NDC坐标范围[-1, 1]
    gl_Position = vec4(inPosition, 0.0, 1.0);
}
