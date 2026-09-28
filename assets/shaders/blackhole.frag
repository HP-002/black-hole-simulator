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
uniform float uEscapeRadius;
uniform samplerCube uSkybox;

// Mirrors physics/Schwarzschild.cpp
const float kStepFraction = 0.05; // step length / r
const int kMaxSteps = 500;

// Hovering observer's view -> coordinate direction
vec3 coordinateDirection(vec3 pos, vec3 dir) {
    vec3 radial = normalize(pos);
    float f = 1.0 - uRs / length(pos);
    return normalize(dir + (sqrt(f) - 1.0) * dot(dir, radial) * radial);
}

// d/dt (pos, vel) for x'' = -strength x / r^5
void derivative(vec3 pos, vec3 vel, float strength, out vec3 dPos,
                out vec3 dVel) {
    float r2 = dot(pos, pos);
    dPos = vel;
    dVel = -strength * pos / (r2 * r2 * sqrt(r2));
}

void rk4Step(inout vec3 pos, inout vec3 vel, float strength, float dt) {
    vec3 p1, v1, p2, v2, p3, v3, p4, v4;
    derivative(pos, vel, strength, p1, v1);
    derivative(pos + p1 * dt / 2.0, vel + v1 * dt / 2.0, strength, p2, v2);
    derivative(pos + p2 * dt / 2.0, vel + v2 * dt / 2.0, strength, p3, v3);
    derivative(pos + p3 * dt, vel + v3 * dt, strength, p4, v4);
    pos += (p1 + 2.0 * p2 + 2.0 * p3 + p4) * dt / 6.0;
    vel += (v1 + 2.0 * v2 + 2.0 * v3 + v4) * dt / 6.0;
}

vec3 trace(vec3 pos, vec3 dir) {
    if (length(pos) <= uRs) {
        return vec3(0.0);
    }
    vec3 vel = coordinateDirection(pos, dir);
    vec3 h = cross(pos, vel);
    float strength = 1.5 * uRs * dot(h, h);

    for (int i = 0; i < kMaxSteps; ++i) {
        float r = length(pos);
        if (r <= uRs) {
            return vec3(0.0);
        }
        if (r >= uEscapeRadius && dot(pos, vel) > 0.0) {
            // Level 0: no derivatives in divergent flow
            return textureLod(uSkybox, normalize(vel), 0.0).rgb;
        }
        rk4Step(pos, vel, strength, kStepFraction * r / length(vel));
    }
    return vec3(0.0); // still orbiting
}

void main() {
    vec3 dir = normalize(uCameraFront +
                         vNdc.x * uTanHalfFov * uAspect * uCameraRight +
                         vNdc.y * uTanHalfFov * uCameraUp);
    FragColor = vec4(trace(uCameraPos, dir), 1.0);
}
