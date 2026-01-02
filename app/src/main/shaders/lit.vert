#version 450

layout(location = 0) in vec4 inPosition;
layout(location = 1) in vec4 inColor;
layout(location = 2) in vec4 inUv;
layout(location = 3) in vec4 inNormal;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec2 fragTexCoord;
layout(location = 2) out vec3 fragNormal;
layout(location = 3) out vec3 fragPos;

layout(binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
} ubo;

void main() {
    // Project vertex position
    gl_Position = ubo.proj * ubo.view * ubo.model * vec4(inPosition.xyz, 1.0);

    // Pass world position to fragment shader
    fragPos = vec3(ubo.model * vec4(inPosition.xyz, 1.0));

    // Pass color to fragment shader
    fragColor = inColor.rgb;

    // Pass UV coordinates to fragment shader
    fragTexCoord = inUv.xy;

    // Transform normal to world space and pass to fragment shader
    // Using inverse transpose of model matrix to correctly handle non-uniform scaling
    fragNormal = normalize(mat3(transpose(inverse(ubo.model))) * inNormal.xyz);
}
