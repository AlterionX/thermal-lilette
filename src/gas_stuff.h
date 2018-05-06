#pragma once

#ifndef __GAS_STUFF_H__
#define __GAS_STUFF_H__

#include "glfw_dispatch.h"

#include <glm/glm.hpp>
#include <vector>

class GasModel {
public:
    GasModel(glm::ivec3 sz);

private:
    glm::ivec3 size_; // (W, H, L)

    // dynamic texs, rgba -> u, v, w, den
    union {
        GLuint texs_[3];
        struct {
            GLuint tex___;
            GLuint tex_0_;
            GLuint tex_s_;
        };
    };

public:
    GLuint get_tex3d(void) { return tex___; }
    glm::ivec3 get_size(void) const { return size_; }
    void simulate_step(const float& dt);

private:

    // simulation, from http://www.intpowertechcorp.com/GDC03.pdf
	void den_step(const float& dt);
	void vel_step(const float& dt);

    void add_source(const glm::ivec4& mask, const float& dt);
    void diffuse(const glm::ivec4& mask, const float& diff, const float& dt);
    void advect(const glm::ivec4& mask, const float& dt);
    void set_bnd(const glm::ivec4& mask);
	void project(void);
    
    float visc_ = 1.0;
    float diff_ = 0.01;
    int bnd_type_ = 1;

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
    GLuint adsrc_dt_uniloc_; // float
    GLuint adsrc_mask_uniloc_; // ivec4
    // tex img 0 = modify
    // tex img 1 = src_dict

    GLuint advec_dt_uniloc_; // float
    GLuint advec_mask_uniloc_; // ivec4
    // tex img 0 = src
    // tex img 1 = dst

    GLuint diffu_a_uniloc_; // float
    GLuint diffu_dt_uniloc_; // float
    GLuint diffu_mask_uniloc_; // ivec4
    // tex img 0 = src
    // tex img 1 = dst

    // proj0
    // tex img 0 = divp
    // tex img 1 = vel

    GLuint proj1_ff_uniloc_; // int, 0 or 1
    // tex img 0 = tex (divp)

    GLuint proj2_ff_uniloc_; // int, 0 or 1
    // tex img 0 = divp
    // tex img 1 = dst

    GLuint bound_mode_uniloc_; // int
    GLuint bound_mask_uniloc_; // ivec4
    // tex img 0 bounding_tex

	void make_ellipse(std::vector<glm::vec4>& vels); // for debugging
};

#endif