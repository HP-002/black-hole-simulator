#version 430 core

out vec2 vNdc;
void main() {
    // (-1,-1), (3,-1), (-1,3)
    vNdc = vec2((gl_VertexID << 1) & 2, gl_VertexID & 2) * 2.0 - 1.0;
    gl_Position = vec4(vNdc, 0.0, 1.0);
}
