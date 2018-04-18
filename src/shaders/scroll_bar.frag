R"zzz(#version 330 core
out vec4 fragment_color;
in vec2 tex_coord;
void main() {
	float d_x = min(tex_coord.x, 1.0 - tex_coord.x);
	float d_y = min(tex_coord.y, 1.0 - tex_coord.y);
	if (d_x < 0.1 || d_y < 0.01) {
		fragment_color = vec4(0.0, 0.0, 0.0, 1.0);
	} else if(d_y < 0.025) {
		fragment_color = vec4(1.0, 1.0, 1.0, 1.0);
	} else {
		float kx = d_x*1.5 + 0.25;
		fragment_color = vec4(kx*0.2, kx*0.7, kx, 0.5);
	}
}
)zzz"
