R"zzz(
#version 330 core

uniform float sky_time;
uniform float cloud_freq;

in vec2 uv_coords;
in vec4 face_normal;
in vec4 vertex_normal;
in vec4 light_direction;
in vec4 world_position;
out vec4 fragment_color;

#define M_PI 3.14159265358979323846

#define H1 10007
#define H2 10009
#define H3 10103

#define sky_vx 1.0 / sqrt(2)
#define sky_vy 1.0 / sqrt(2)

float rand(int i, int j) {
	i %= 10;
	j %= 10;
	return (cos(1.72 * i * i * j + 2.34 * j * j + 1.32*i)+1) * M_PI;
}

float sigmoid(float x) {
	return 1.0 / (1.0 + exp(-x));
}

vec2 get_gvec(int i, int j) {
	float deg = rand(i, j);
	return vec2(cos(2 * M_PI * deg), sin(2 * M_PI * deg));
}

float get_perlin(vec2 coord, int freq) {
	vec2 cpos = vec2(coord.x * freq, coord.y * freq);
	int lx = int(cpos.x);
	int ly = int(cpos.y);

	float subvals[2];
	int i, j;
	for(i=0; i<2; i++) {
		float subsubvals[2];	
		for(j=0; j<2; j++) {
			subsubvals[j] = dot(get_gvec(lx+i, ly+j), cpos - vec2(lx+i, ly+j));
		}
		subvals[i] = mix(subsubvals[0], subsubvals[1], cpos.y - ly);
	}
	float pval = mix(subvals[0], subvals[1], cpos.x - lx);
	return (pval / sqrt(2) + 1) / 1.5;
}

void main() {
	// sky background color
	vec3 color = vec3(0.5, 0.5, 1.0);
	if(int(sky_time*2)%2 == 1) {
		color = vec3(0.02, 0.0, 0.1);
	}

	// get clouds
	vec2 coord = uv_coords + vec2(sky_vx*sky_time, sky_vy*sky_time);
	float pval = (get_perlin(coord, int(cloud_freq)) 
				+ 2*get_perlin(coord, int(cloud_freq/2))
				+ 4*get_perlin(coord, int(cloud_freq/4))) / 7.0;
	pval = 2*exp(cloud_freq/100.0*pval) / exp(cloud_freq/100.0);
	float alpha = pval;
	if(int(sky_time*2)%2 == 1) {
		pval *= -0.5;
		vec2 cpos = vec2(1000*uv_coords.x, 1000*uv_coords.y);
		if(cos(rand(int(cpos.x), int(cpos.y))) >= 1.0
			&& cpos.x - int(cpos.x) <= 0.1 
			&& cpos.y - int(cpos.y) <= 0.1
			&& pval > -0.25) {
				color = vec3(1.0, 1.0, 1.0);
				alpha = 1.0;
			}
	}

	color += vec3(pval, pval, pval);
	color = clamp(color, 0.0, 1.0);

	fragment_color = vec4(color, alpha);
}
)zzz"
