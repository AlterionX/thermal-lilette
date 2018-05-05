#include "gas_stuff.h"

#include <random>

#include <iostream> // TODO: remove this
#include <glm/gtx/string_cast.hpp> // TODO: remove this

#define N_ITER 20
#define EPS 1e-6
#define NUM_THREADS 4
#define at(x, y, z) (x)*size[1]*size[2] + (y)*size[2] + (z) // 1d array coordinate
#define pat(x, y, z) (x)*size[1]*size[2]*4 + (y)*size[2]*4 + (z)*4 // RGBA coordinate

namespace {
    std::vector<glm::vec4> rand_vels(const glm::vec3& size) {
        std::vector<glm::vec4> output;
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
                    1e6 * (x - size.x/2),
                    1e8,
                    1e6 * (z - size.z/2),
                    1.0
                };

                for(int y=2; y<size.y-2; y++) {
                    output[at(x, y, z)] = glm::vec4{
                        1e6 * (x - size.x/2),
                        1e8,
                        1e6 * (z - size.z/2),
                        1.0
                    };
                }
            }
    }

    void four_side_push(const glm::vec3& size, std::vector<glm::vec4>& output) {
        for(int x=size.x*11/24; x<size.x*13/24; x++)
            for(int z=size.z*11/24; z<size.z*13/24; z++) {
                for(int y=1; y<size.y-2; y++) {
                    output[at(x, y, z)] = glm:::vec4{-1e6 * (x - size.x/2), 1e8, -1e6 * (z - size.z/2), 0};
                }
            }

        for(int x=size.x*11/24; x<size.x*13/24; x++)
            for(int y=1; y<size.y-2; y++) {
            // for(int y=size.y*11/24; y<size.y*13/24; y++) {
                output[at(x, y, 1)] = glm::vec3{
                    1e8,
                    0.0f,
                    0.0f,
                    1.0
                };
                output[at(x, y, size.z-2)] = glm::vec3{
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
}

GasModel::GasModel(glm::ivec3 sz) : size_(sz) {
    make_ellipse();
    std::vector<glm::vec4> vufs[3] = {rand_vels(size_), std::vector<glm::vec4>(size.x * size.y * size.z), std::vector<glm::vec4>(size.x * size.y * size.z)};
    up_jet_cloud(size_, sources);


    // Init opengl resources for optimization
    // compute shaders and programs
    for (int i = 0; i < 6; ++i) {
        shader_ids_[i] = glCreateShader(GL_COMPUTE_SHADER);
        glShaderSource(shader_ids_[i], 1, &shader_srcs_[i], 0);
        glCompileShader(shader_ids_[i]);

        prog_ids_[p] = glCreateProgram();
        glAttachShader(prog_ids_[i], shader_ids_[i]);
        glLinkProgram(prog_ids_[i]);
        CHECK_GL_PROGRAM_ERROR(prog_ids_[i]);
    }
    // uniforms
    adsrc_dt_uniloc_ = glGetUniformLocation(adsrc_prog_id_, "dt");
    adsrc_mask_uniloc_ = glGetUniformLocation(adsrc_prog_id_, "mask");

    advec_dt_uniloc_ = glGetUniformLocation(advec_prog_id_, "dt");
    advec_shift_channel_uniloc_ = glGetUniformLocation(advec_prog_id_, "shift_mask");

    diffu_a_uniloc_ = glGetUniformLocation(diffu_prog_id_, "a");
    diffu_dt_uniloc_ = glGetUniformLocation(diffu_prog_id_, "dt");
    diffu_shift_mask_uniloc_ = glGetUniformLocation(diffu_prog_id_, "shift_mask");
    
    proj0_shift_mask_uniloc_ = glGetUniformLocation(proj0_prog_id_, "shift_mask");
    proj0_reset_channel_uniloc_ = glGetUniformLocation(proj0_prog_id_, "reset_channel");
    
    proj1_stable_channel_uniloc_ = glGetUniformLocation(proj1_prog_id_, "stable_channel");
    proj1_shift_mask_uniloc_ = glGetUniformLocation(proj1_prog_id_, "shift_mask");
    
    proj2_shift_channel_mask_uniloc_ = glGetUniformLocation(proj 2_prog_id_, "shift_channel_mask");
    proj2_source_channel_mask_uniloc_ = glGetUniformLocation(proj 2_prog_id_, "source_channel_mask");
    
    bound_mode_uniloc_ = glGetUniformLocation(bound_prog_id_, "mode");
    bound_channel_uniloc_ = glGetUniformLocation(bound_prog_id_, "channel");
    // texs
    glGenTextures(3, &texs_);
    for (int i = 0; i < 3; ++i) {
        glBindTexture(GL_TEXTURE_3D, texs_[i]);
        glTexImage3D(GL_TEXTURE_3D, 0, GL_RGBA, size.x, size.y, size.z, 0, GL_RGBA, GL_FLOAT, vufs[i].data());
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    }
}

/********************************************************/
/** Navier-Stokes (v-rho) Gas Model Solver **************/

void GasModel::simulate_step(float dt) {
    vel_step(dt);
    den_step(diff, dt);
}
void GasModel::den_step(float dt) {
    static auto dmask = glm::ivec4{0, 0, 0, 1};
    add_source(dmask, dt);
    diffuse(dmask, dt);
    advect(dmask, dt);
}
void GasModel::vel_step(float dt) {
    static auto vmask = glm::ivec4{1, 1, 1, 0};
    add_source(vmask, dt);
    diffuse(vmask, dt);
    project(); // u0 -> p, v0 -> div
    advect(vmask, dt);
    project(); // u0 -> p, v0 -> div
}

void GasModel::add_source(glm::ivec4 mask, float dt) {
    // call source
}
void GasModel::diffuse(glm::ivec4 mask, float dt) {
    float a = dt * diff * size.x * size.y * size.z;
    for(int i = 0; i < N_ITER; ++i) {
        // call diffu
        glUseProgram(diffu_prog_id_);

        glUniform1f(advec_dt_uniloc_, dt);
        glBindImageTexture(0, tex___, 0, GL_LAYERED, 0, GL_READ_ONLY, GL_RGBA32F);
        glBindImageTexture(0, tex_0_, 0, GL_LAYERED, 0, GL_READ_ONLY, GL_RGBA32F);

        glDispatchCompute(size[0], size[1], size[2]);
        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
        set_bnd(mask);
    }
}
void GasModel::advect(glm::ivec4 mask, float dt) {
    // call advec
    glUseProgram(advec_prog_id_);

    glUniform1f(advec_dt_uniloc_, dt);
    glBindImageTexture(0, tex___, 0, GL_LAYERED, 0, GL_READ_ONLY, GL_RGBA32F);
    glBindImageTexture(0, tex_0_, 0, GL_LAYERED, 0, GL_READ_ONLY, GL_RGBA32F);

    glDispatchCompute(size[0], size[1], size[2]);
    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

    set_bnd(mask);
}
void GasModel::project(void) {
    // proj 0
    glUseProgram(proj0_prog_id_);


    for(l=0; l<N_ITER; l++) {
        // proj 1
        glUseProgram(proj1_prog_id_);



        glDispatchCompute(size[0], size[1], size[2]);
        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);    

        set_bnd(glm::ivec4{0, 0, 0, 1});
    }

    // proj 2
    glUseProgram(proj2_prog_id_);

    glDispatchCompute(size[0], size[1], size[2]);
    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);    
    
    set_bnd(glm::ivec4{1, 1, 1, 0});
}
void GasModel::set_bnd(const glm::ivec4& mask) {
    glUseProgram(bound_prog_id_);

    glDispatchCompute(size[0], size[1], size[2]);
    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);    
}

void GasModel::make_ellipse(std::vector<glm::vec4> source, std::vector<glm::vec4> vels) {
    glm::vec3 center = glm::vec3((float)size.x, (float)size.y, size.z/2.0f)/2.0f;
    for(int x=0; x<size.x; x++)
        for(int y=0; y<size.y; y++)
            for(int z=0; z<size.z; z++) {
                tex3d[pat(x, y, z)+0] = 255;
                tex3d[pat(x, y, z)+1] = 255;
                tex3d[pat(x, y, z)+2] = 255;
                glm::vec3 delta = glm::vec3((float)x, (float)y, z/2.0f) - center;
                std::cout << glm::to_string(delta) << ", length= " << glm::length(delta) << std::endl;
                if(glm::length(delta) < size.x/4.0) {
                    tex3d[pat(x, y, z)+3] = 255;
                }
                else {
                    tex3d[pat(x, y, z)+3] = 0;
                }
            }
}