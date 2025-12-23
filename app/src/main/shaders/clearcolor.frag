#version 450

// 纯色渲染片段着色器
// 输出固定的天空颜色

layout(location = 0) out vec4 outColor;

void main() {
    // 天空蓝颜色 (RGB: 0.53, 0.81, 0.92)
    outColor = vec4(0.53, 0.81, 0.92, 1.0);
}
