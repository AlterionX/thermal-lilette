R"zzz(#version 330 core
out vec4 fragment_color;
in vec2 tex_coord;
uniform sampler2D sampler;
uniform bool show_border;
void main() {
	float d_x = min(tex_coord.x, 1.0 - tex_coord.x);
	float d_y = min(tex_coord.y, 1.0 - tex_coord.y);
	if (show_border && (d_x < 0.05 || d_y < 0.05) ) {
		fragment_color = vec4(0.0, 1.0, 0.0, 1.0);
	} else {
		// fragment_color = vec4(texture(sampler, tex_coord).xyz, 1.0);
		float dxy = 0.002;
		vec2 tex_xy = tex_coord * (1 - dxy);
		fragment_color = (vec4(texture(sampler, tex_xy).xyz, 1.0)
						+ vec4(texture(sampler, tex_xy + vec2(dxy/2.0, 0.0)).xyz, 1.0)
						+ vec4(texture(sampler, tex_xy + vec2(0.0, dxy/2.0)).xyz, 1.0)
						+ vec4(texture(sampler, tex_xy + vec2(dxy/2.0, dxy/2.0)).xyz, 1.0)
						+ vec4(texture(sampler, tex_xy + vec2(dxy, dxy/2.0)).xyz, 1.0)
						+ vec4(texture(sampler, tex_xy + vec2(dxy/2.0, dxy)).xyz, 1.0)
						+ vec4(texture(sampler, tex_xy + vec2(dxy, 0.0)).xyz, 1.0)
						+ vec4(texture(sampler, tex_xy + vec2(0.0, dxy)).xyz, 1.0)
						+ vec4(texture(sampler, tex_xy + vec2(dxy, dxy)).xyz, 1.0)) / 9.0;
	}
}
)zzz"
