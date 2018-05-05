#pragma once

#ifndef __GAS_STUFF_H__
#define __GAS_STUFF_H__

#include <glm/glm.hpp>
#include <vector>

class GasModel {
public:
    GasModel(glm::ivec3 sz);

    char* get_tex3d(void) { return tex___; }
    glm::ivec3 get_size(void) const { return size; }
	void simulate_step(float dt);

private:
    glm::ivec3 size; // (W, H, L)

    // simulation, from http://www.intpowertechcorp.com/GDC03.pdf
	void den_step(float dt);
	void vel_step(float dt);

    void add_source(std::vector<float>& x, std::vector<float>& s, float dt);
    void diffuse(const glm::ivec4& mask, const float& dt);
    void advect(const glm::ivec4& mask, const float& dt);
    void set_bnd(const glm::ivec4& mask, const float& dt);
	void project(void);
    
    float visc_ = 1.0;
    float diff_ = 0.01;
    int bnd_type_ = 1;

    // program bindings
    static constexpr shader_srcs_ = {
        #include "compute/adsrc.comp"
        ,
        #include "compute/advec.comp"
        ,
        #include "compute/bound.comp"
        ,
        #include "compute/diffu.comp"
        ,
        #include "compute/proj0.comp"
        ,
        #include "compute/proj1.comp"
        ,
        #include "compute/proj2.comp"
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
    GLuint advec_shift_mask_uniloc_;

    GLuint diffu_a_uniloc_;
    GLuint diffu_dt_uniloc_;
    GLuint diffu_shift_mask_uniloc_;

    GLuint adsrc_dt_uniloc_;
    GLuint adsrc_mask_uniloc_;

    GLuint proj0_shift_mask_uniloc_;
    GLuint proj0_reset_channel_uniloc_;

    GLuint proj1_stable_channel_uniloc_;
    GLuint proj1_shift_mask_uniloc_;

    GLuint proj2_shift_mask_mask_uniloc_;
    GLuint proj2_source_channel_mask_uniloc_;

    GLuint bound_mode_uniloc_;
    GLuint bound_channel_uniloc_;

    // dynamic texs, argb -> den, u, v, w
    union {
        GLuint texs_[3];
        struct {
            GLuint tex___;
            GLuint tex_0_;
            GLuint tex_s_;
        };
    };

	void make_ellipse(void); // for debugging
};

#endif