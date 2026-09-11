#version 430 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aColor;

// The three transformations applied right-to-left:
//   model:      object space -> world space
//   view:       world space  -> camera space
//   projection: camera space -> clip space
uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProjection;

out vec3 vColor;
void main() {
    gl_Position = uProjection * uView * uModel * vec4(aPos, 1.0);
    vColor = aColor;
}
