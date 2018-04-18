R"zzz(
#version 330 core
// skinning calculation modes
// const int LIN_BLINN = 0
// const int BI_QUAT = 1
// uniform int mode;

uniform vec4 light_position;
uniform vec3 camera_position;

uniform vec3 joint_tran[128];
uniform vec4 joint_rota[128];

in int jid0;
in int jid1;
in float w0;
in vec3 vector_from_joint0;
in vec3 vector_from_joint1;
in vec4 normal;
in vec2 uv;
in vec4 vert;

out vec4 vs_light_direction;
out vec4 vs_normal;
out vec2 vs_uv;
out vec4 vs_camera_direction;

vec3 qtransform(vec4 q, vec3 v) {
	return v + 2.0 * cross(cross(v, q.xyz) - q.w*v, q.xyz);
}

// fully in model space
void main() {
	// rotate about joint then translate by joint to get back to model space
	vec3 pbl0 = qtransform(joint_rota[jid0], vector_from_joint0) + joint_tran[jid0];
	vec3 pbl1 = qtransform(joint_rota[jid1], vector_from_joint1) + joint_tran[jid1];
	// then linearly blend the positions with given weight
	vec3 bl_vert = mix(pbl1, pbl0, w0);

	gl_Position = vec4(bl_vert, 1);
	vs_normal = normal;
	vs_light_direction = light_position - gl_Position;
	vs_camera_direction = vec4(camera_position, 1.0) - gl_Position;
	vs_uv = uv;
}
)zzz"
