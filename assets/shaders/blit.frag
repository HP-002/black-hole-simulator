#version 430 core
// Stretch an image over the screen
in vec2 vNdc;
out vec4 FragColor;

uniform sampler2D uImage;

void main() {
    FragColor = vec4(texture(uImage, vNdc * 0.5 + 0.5).rgb, 1.0);
}
