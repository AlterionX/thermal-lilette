R"zzz(
#version 330 core
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

	float r_x = 16*uv_coords.x - int(16*uv_coords.x);
	float r_y = 16*uv_coords.y - int(16*uv_coords.y);
	float d_x = min(r_x, 1.0 - r_x);
	float d_y = min(r_y, 1.0 - r_y);
	if (d_x < 0.01 || d_y < 0.01) {
		fragment_color = vec4(0.0, 1.0, 0.0, 1.0);
	}
	else {
	 //    // vec3 color = vec3(1.0, 1.0, 1.0); // solid color
	 //    vec3 color = vec3(uv_coords.x, 0.5*(uv_coords.x+uv_coords.y), uv_coords.y); // contour
		// // vec3 color = mod((int(10*uv_coords.x) + int(10*uv_coords.y)), 2) * vec3(1.0, 1.0, 1.0); // by uv_coords

	 //    fragment_color = vec4(color, 1.0);
		fragment_color = vec4(0.0);
	}
}
)zzz"
