R"zzz(
#version 330 core
uniform sampler3D vox_tex;
uniform float tex3d_layer;

in vec4 face_normal;
in vec4 vertex_normal;
in vec4 light_direction;
in vec4 camera_direction;
in vec2 uv_coords;
uniform vec4 diffuse;
uniform vec4 ambient;
uniform vec4 specular;
uniform float shininess;
uniform float alpha;
out vec4 fragment_color;

void main() {
    // vec3 color = vec3(uv_coords.x, tex3d_layer/2.0 + 0.5, uv_coords.y);
    // fragment_color = vec4(color, 1.0);

    fragment_color = texture(vox_tex, vec3(uv_coords.x, tex3d_layer/2.0 + 0.5, uv_coords.y));
}
)zzz"
