#include "gas_stuff.h"

#include <random>

#include <iostream> // TODO: remove this
#include <glm/gtx/string_cast.hpp> // TODO: remove this

#define N_ITER 50
#define EPS 1e-6
#define NUM_THREADS 4
#define at(x, y, z) (x)*size[1]*size[2] + (y)*size[2] + (z) // 1d array coordinate
#define pat(x, y, z) (x)*size[1]*size[2]*4 + (y)*size[2]*4 + (z)*4 // RGBA coordinate

namespace {
    std::vector<glm::vec4> rand_vels(const glm::vec3& size) {
        std::vector<glm::vec4> output(size[0]*size[1]*size[2]);
        // randomly initialize velocities
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<> dis(-1.0, 1.0);
        for(int i=0; i<size.x*size.y*size.z; i++) {
            output[i] = glm::vec4{
                dis(gen),
                dis(gen),
                dis(gen),
                0.0
            };
        }
        return output;
    }

    void up_jet_cloud(const glm::vec3& size, std::vector<glm::vec4>& output) {
        // up-jet of cloud
        for(int x=size.x*11/24; x<size.x*13/24; x++)
            for(int z=size.z*11/24; z<size.z*13/24; z++) {
                output[at(x, 1, z)] = glm::vec4{
                    1e6f * (x - size.x/2),
                    1e8f,
                    1e6f * (z - size.z/2),
                    1.0f
                };

                for(int y=2; y<size.y-2; y++) {
                    output[at(x, y, z)] = glm::vec4{
                        1e6f * (x - size.x/2),
                        1e8f,
                        1e6f * (z - size.z/2),
                        1.0f
                    };
                }
            }
    }

    void four_side_push(const glm::vec3& size, std::vector<glm::vec4>& output) {
        for(int x=size.x*11/24; x<size.x*13/24; x++)
            for(int z=size.z*11/24; z<size.z*13/24; z++) {
                for(int y=1; y<size.y-2; y++) {
                    output[at(x, y, z)] = glm::vec4{
                        -1e6f * (x - size.x/2),
                        1e8f,
                        -1e6f * (z - size.z/2),
                        0.0f
                    };
                }
            }

        for(int x=size.x*11/24; x<size.x*13/24; x++)
            for(int y=1; y<size.y-2; y++) {
            // for(int y=size.y*11/24; y<size.y*13/24; y++) {
                output[at(x, y, 1)] = glm::vec4{
                    1e8,
                    0.0f,
                    0.0f,
                    1.0
                };
                output[at(x, y, size.z-2)] = glm::vec4{
                    -1e8,
                    0.0f,
                    0.0f,
                    1.0
                };
            }
        for(int z=size.z*11/24; z<size.z*13/24; z++)
            for(int y=1; y<size.y-2; y++) {
            // for(int y=size.y*11/24; y<size.y*13/24; y++) {
                output[at(1, y, z)] = glm::vec4{
                    0.0f,
                    0.0f,
                    1e8,
                    1.0
                };
                output[at(size.z-2, y, z)] = glm::vec4{
                    0.0f,
                    0.0f,
                    -1e8,
                    1.0
                };
            }
    }


    const char* adsrc_comp_shader =
    #include "compute/adsrc.comp"
    ;
    const char* advec_comp_shader =
    #include "compute/advec.comp"
    ;
    const char* bound_comp_shader =
    #include "compute/bound.comp"
    ;
    const char* diffu_comp_shader =
    #include "compute/diffu.comp"
    ;
    const char* proj0_comp_shader =
    #include "compute/proj0.comp"
    ;
    const char* proj1_comp_shader =
    #include "compute/proj1.comp"
    ;
    const char* proj2_comp_shader =
    #include "compute/proj2.comp"
    ;
    // program bindings
    const char* shader_srcs_[] = {
        adsrc_comp_shader,
        advec_comp_shader,
        bound_comp_shader,
        diffu_comp_shader,
        proj0_comp_shader,
        proj1_comp_shader,
        proj2_comp_shader
    };
}

GasModel::GasModel(glm::ivec3 sz) : size_(sz) {
    std::vector<glm::vec4> vufs[3] = {
        rand_vels(size_),
        std::vector<glm::vec4>(size_.x * size_.y * size_.z),
        std::vector<glm::vec4>(size_.x * size_.y * size_.z)
    };
    // make_ellipse(vufs[0]);
    up_jet_cloud(size_, vufs[2]);

    // Init opengl resources for optimization
    gdm::qd([&] GDM_OP {
        // compute shaders and programs
        for (int i = 0; i < 7; ++i) {
            CHECK_GL_ERROR(shader_ids_[i] = glCreateShader(GL_COMPUTE_SHADER));
            CHECK_GL_ERROR(glShaderSource(shader_ids_[i], 1, &shader_srcs_[i], 0));
            glCompileShader(shader_ids_[i]);
            CHECK_GL_SHADER_ERROR(shader_ids_[i]);

            CHECK_GL_ERROR(prog_ids_[i] = glCreateProgram());
            glAttachShader(prog_ids_[i], shader_ids_[i]);
            glLinkProgram(prog_ids_[i]);
            CHECK_GL_PROGRAM_ERROR(prog_ids_[i]);
        }
        // uniforms
        adsrc_dt_uniloc_ = glGetUniformLocation(adsrc_prog_id_, "dt");
        adsrc_mask_uniloc_ = glGetUniformLocation(adsrc_prog_id_, "mask");

        advec_dt_uniloc_ = glGetUniformLocation(advec_prog_id_, "dt");
        advec_mask_uniloc_ = glGetUniformLocation(advec_prog_id_, "mask");

        diffu_a_uniloc_ = glGetUniformLocation(diffu_prog_id_, "a");
        diffu_dt_uniloc_ = glGetUniformLocation(diffu_prog_id_, "dt");
        diffu_mask_uniloc_ = glGetUniformLocation(diffu_prog_id_, "mask");
        
        proj1_ff_uniloc_ = glGetUniformLocation(proj1_prog_id_, "ff");

        proj2_ff_uniloc_ = glGetUniformLocation(proj2_prog_id_, "ff");
        
        bound_mode_uniloc_ = glGetUniformLocation(bound_prog_id_, "mode");
        bound_mask_uniloc_ = glGetUniformLocation(bound_prog_id_, "mask");

        // texs
        glGenTextures(3, texs_);
        for (int i = 0; i < 3; ++i) {
            glBindTexture(GL_TEXTURE_3D, texs_[i]);
            glTexImage3D(GL_TEXTURE_3D, 0, GL_RGBA32F,
                size_.x, size_.y, size_.z,
                0, GL_RGBA, GL_FLOAT,
                vufs[i].data()
            );
            CHECK_GL_ERROR(glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER));
            CHECK_GL_ERROR(glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER));
            CHECK_GL_ERROR(glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_BORDER));
            CHECK_GL_ERROR(glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
            CHECK_GL_ERROR(glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
        }
    });
}

/********************************************************/
/** Navier-Stokes (v-rho) Gas Model Solver **************/

void GasModel::simulate_step(const float& dt) {
    vel_step(dt);
    den_step(dt);
}

void GasModel::vel_step(const float& dt) {
    static auto vmask = glm::ivec4{1, 1, 1, 0};
    add_source(vmask, dt);
    diffuse(vmask, visc_, dt);
    project(); // u0 -> p, v0 -> div
    advect(vmask, dt);
    project(); // u0 -> p, v0 -> div
}
void GasModel::den_step(const float& dt) {
    static auto dmask = glm::ivec4{0, 0, 0, 1};
    add_source(dmask, dt);
    diffuse(dmask, diff_, dt);
    advect(dmask, dt);
}

void GasModel::add_source(const glm::ivec4& mask, const float& dt) {
    // call source
    glUseProgram(adsrc_prog_id_);

    glUniform1f(adsrc_dt_uniloc_, dt);
    glUniform4iv(adsrc_mask_uniloc_, 1, &mask[0]);

    glBindImageTexture(0, tex___, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);
    glBindImageTexture(1, tex_s_, 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA32F);

    glDispatchCompute(size_[0], size_[1], size_[2]);
    glMemoryBarrier(GL_ALL_BARRIER_BITS);
}
void GasModel::advect(const glm::ivec4& mask, const float& dt) {
    // call advec
    glUseProgram(advec_prog_id_);

    glUniform1f(advec_dt_uniloc_, dt);
    glUniform4iv(advec_mask_uniloc_, 1, &mask[0]);

    glBindImageTexture(0, tex___, 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA32F);
    glBindImageTexture(1, tex_0_, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);

    glDispatchCompute(size_[0], size_[1], size_[2]);
    glMemoryBarrier(GL_ALL_BARRIER_BITS);

    std::swap(tex___, tex_0_);

    // set_bnd(mask);
}
void GasModel::diffuse(const glm::ivec4& mask, const float& diff, const float& dt) {
    float a = dt * diff * size_.x * size_.y * size_.z;
    for(int i = 0; i < N_ITER; ++i) {
        // call diffu
        glUseProgram(diffu_prog_id_);

        glUniform1f(diffu_a_uniloc_, a);
        glUniform1f(diffu_dt_uniloc_, dt);
        glUniform4iv(diffu_mask_uniloc_, 1, &mask[0]);

        glBindImageTexture(0, tex___, 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA32F);
        glBindImageTexture(1, tex_0_, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);

        glDispatchCompute(size_[0], size_[1], size_[2]);
        glMemoryBarrier(GL_ALL_BARRIER_BITS);

        std::swap(tex___, tex_0_);

        set_bnd(mask);
    }
}
void GasModel::project(void) {
    // proj 0
    glUseProgram(proj0_prog_id_);

    glBindImageTexture(0, tex_0_, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);
    glBindImageTexture(1, tex___, 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA32F);

    glDispatchCompute(size_[0], size_[1], size_[2]);
    glMemoryBarrier(GL_ALL_BARRIER_BITS);

    std::swap(tex___, tex_0_);

    int curr_val = 0;
    for(int i = 0; i < N_ITER; ++i) {
        // proj 1
        glUseProgram(proj1_prog_id_);

        glUniform1i(proj1_ff_uniloc_, curr_val);

        glBindImageTexture(0, tex_0_, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);

        glDispatchCompute(size_[0], size_[1], size_[2]);
        glMemoryBarrier(GL_ALL_BARRIER_BITS);

        curr_val = (curr_val + 1) % 2;

        set_bnd(glm::ivec4{int(curr_val == 0), int(curr_val == 1), 0, 0});
    }

    // proj 2
    glUseProgram(proj2_prog_id_);

    glUniform1i(proj2_ff_uniloc_, curr_val);

    glBindImageTexture(0, tex_0_, 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA32F);
    glBindImageTexture(1, tex___, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);

    glDispatchCompute(size_[0], size_[1], size_[2]);
    glMemoryBarrier(GL_ALL_BARRIER_BITS);
    
    set_bnd(glm::ivec4{1, 1, 1, 0});
}
void GasModel::set_bnd(const glm::ivec4& mask) {
    glUseProgram(bound_prog_id_);

    glUniform1i(bound_mode_uniloc_, bnd_type_);
    glUniform4iv(bound_mask_uniloc_, 1, &mask[0]);

    glBindImageTexture(0, tex___, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);

    glDispatchCompute(size_[0], size_[1], size_[2]);
    glMemoryBarrier(GL_ALL_BARRIER_BITS);
}

void GasModel::make_ellipse(std::vector<glm::vec4>& vels) {
    glm::vec3 center = glm::vec3((float)size_.x, (float)size_.y, size_.z/2.0f)/2.0f;
    auto size = size_;
    for(int x=0; x<size_.x; x++)
        for(int y=0; y<size_.y; y++)
            for(int z=0; z<size_.z; z++) {
                vels[at(x, y, z)] = glm::ivec4{255, 255, 255, 0};
                glm::vec3 delta = glm::vec3((float)x, (float)y, z/2.0f) - center;
                // std::cout << glm::to_string(delta) << ", length= " << glm::length(delta) << std::endl;
                if(glm::length(delta) < size_.x/4.0) {
                    vels[at(x, y, z)][3] = 255;
                }
                else {
                    vels[at(x, y, z)][3] = 0;
                }
            }
}