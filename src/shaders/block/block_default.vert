R"zzz(#version 330 core
#version 330 core

uniform vec4 light_position;
uniform vec3 camera_position;

in vec4 v_pos;
in vec4 norm;
in vec2 uv;

out vec4 v_l_dir;
out vec4 v_norm;
out vec2 v_uv;
out vec4 v_c_dir;

void main() {
    gl_Position = vertex_position;
    vs_light_direction = light_position - gl_Position;
    vs_camera_direction = vec4(camera_position, 1.0) - gl_Position;
    vs_normal = normal;
    vs_uv = uv;
}
)zzz"
