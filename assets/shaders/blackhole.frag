#version 430 core
in vec2 vNdc;
out vec4 FragColor;

uniform vec3 uCameraPos;
uniform vec3 uCameraFront;
uniform vec3 uCameraRight;
uniform vec3 uCameraUp;
uniform float uTanHalfFov;
uniform float uAspect;
uniform float uRs;
uniform samplerCube uSkybox;

bool hitsHorizon(vec3 origin, vec3 dir) {
    float b = dot(origin, dir);
    float c = dot(origin, origin) - uRs * uRs;
    if (c <= 0.0) {
        return true; // inside
    }
    return b < 0.0 && b * b >= c;
}

void main() {
    vec3 dir = normalize(uCameraFront +
                         vNdc.x * uTanHalfFov * uAspect * uCameraRight +
                         vNdc.y * uTanHalfFov * uCameraUp);
    vec3 color = hitsHorizon(uCameraPos, dir) ? vec3(0.0)
                                              : texture(uSkybox, dir).rgb;
    FragColor = vec4(color, 1.0);
}
