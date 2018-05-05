#pragma once

#ifndef __GAS_STUFF_H__
#define __GAS_STUFF_H__

#include <glm/glm.hpp>
#include <vector>

class GasModel {
public:
    GasModel(glm::ivec3 sz);

    char* get_tex3d(void) { return tex3d.data(); }
    glm::ivec3 get_size(void) const { return size; }
	void simulate_step(float dt);

private:

    std::vector<char> tex3d; // W x H x L x 4
    glm::ivec3 size; // (W, H, L)

	void update_tex3d(void);

    // simulation, from http://www.intpowertechcorp.com/GDC03.pdf
	void add_source(std::vector<float>& x, std::vector<float>& s, float dt);
	void diffuse(int b, std::vector<float>& x, std::vector<float>& x0, float diff, float dt);
	void advect(int b, std::vector<float>& d, std::vector<float>& d0,
            std::vector<float>& u, std::vector<float>& v, std::vector<float>& w,
            float dt);
	void den_step(std::vector<float>& x, std::vector<float>& den_s, std::vector<float>& x0,
            std::vector<float>& u, std::vector<float>& v, std::vector<float>& w,
            float diff, float dt);
	void vel_step(std::vector<float>& u, std::vector<float>& u0, std::vector<float>& u_s,
            std::vector<float>& v, std::vector<float>& v0, std::vector<float>& v_s,
            std::vector<float>& w, std::vector<float>& w0, std::vector<float>& w_s,
            float visc, float dt);
	void project(std::vector<float>& u, std::vector<float>& v, std::vector<float>& w,
            std::vector<float>& p, std::vector<float>& div);
	void set_bnd(int b, std::vector<float>& x);
    std::vector<float> vel_u, vel_v, vel_w, vel_u_0, vel_v_0, vel_w_0, vel_u_s, vel_v_s, vel_w_s;
    std::vector<float> den, den_0, den_s;
    std::vector<float> temp;
    float visc = 1.0;
    float diff = 0.01;
    int bnd_type = 1;

    // Then 4 separate buffers
    GLuint vao_ = 0;
    GLuint vbo_ = 0;

    // program bindings
    static constexpr shader_srcs_ = {
        #include "adsrc.comp"
        ,
        #include "advec.comp"
        ,
        #include "bound.comp"
        ,
        #include "diffu.comp"
        ,
        #include "proj0.comp"
        ,
        #include "proj1.comp"
        ,
        #include "proj2.comp"
    };
    union {
        GLuint shader_ids_[7];
        struct {
            GLuint adsrc_shader_id_;
            GLuint advec_shader_id_;
            GLuint bound_shader_id_;
            GLuint diffu_shader_id_;
            GLuint proj0_shader_id_;
            GLuint proj1_shader_id_;
            GLuint proj2_shader_id_;
        };
    };
    union {
        GLuint prog_ids_[7];
        struct {
            GLuint adsrc_prog_id_;
            GLuint advec_prog_id_;
            GLuint bound_prog_id_;
            GLuint diffu_prog_id_;
            GLuint proj0_prog_id_;
            GLuint proj1_prog_id_;
            GLuint proj2_prog_id_;
        };
    };
    
    // uniform location bindings
    GLuint advec_dt_uniloc_;
    GLuint advec_shift_channel_uniloc_;

    GLuint diffu_a_uniloc_;
    GLuint diffu_dt_uniloc_;
    GLuint diffu_shift_channel_uniloc_;

    GLuint adsrc_dt_uniloc_;
    GLuint adsrc_mask_uniloc_;

    GLuint proj0_shift_channel_uniloc_;
    GLuint proj0_reset_channel_uniloc_;

    GLuint proj1_stable_channel_uniloc_;
    GLuint proj1_shift_channel_uniloc_;

    GLuint proj2_shift_channel_mask_uniloc_;
    GLuint proj2_source_channel_mask_uniloc_;

    GLuint bound_mode_uniloc_;
    GLuint bound_channel_uniloc_;

    // dynamic texs, argb -> den, u, v, w
    union {
        GLuint texs_[6];
        struct {
            GLuint tex___;
            GLuint tex_0_;
            GLuint tex_s_;
        };
    };

	void make_ellipse(void); // for debugging
};

#endif