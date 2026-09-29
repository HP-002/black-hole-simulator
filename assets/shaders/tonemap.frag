#version 430 core
// HDR image -> display: exposure, then ACES filmic curve
in vec2 vNdc;
out vec4 FragColor;

uniform sampler2D uImage;
uniform float uExposure;

// Narkowicz's fit of the ACES reference curve
vec3 aces(vec3 x) {
    return clamp(x * (2.51 * x + 0.03) / (x * (2.43 * x + 0.59) + 0.14), 0.0,
                 1.0);
}

void main() {
    vec3 hdr = texture(uImage, vNdc * 0.5 + 0.5).rgb;
    FragColor = vec4(aces(hdr * uExposure), 1.0);
}
