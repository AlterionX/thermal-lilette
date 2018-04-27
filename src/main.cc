#include "glfw_dispatch.h"
#include "render_pass.h"
#include "config.h"
#include "prim_mesh.h"
#include "gui.h"
#include "particle_sys.h"

#include <memory>
#include <algorithm>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include <cmath>

#include <glm/gtx/component_wise.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtx/string_cast.hpp>
#include <glm/gtx/io.hpp>

#include <debuggl.h>

int window_width = 1280;
int window_height = 720;
const std::string window_title = "Thermal Particle System";

const char* vertex_shader =
#include "shaders/default.vert"
;

const char* blending_shader =
#include "shaders/blending.vert"
;

const char* geometry_shader =
#include "shaders/default.geom"
;

const char* fragment_shader =
#include "shaders/default.frag"
;

const char* flat_vert_shader =
#include "shaders/flat.vert"
;

const char* flat_frag_shader =
#include "shaders/flat.frag"
;

const char* instanced_vert_shader = 
#include "shaders/instanced.vert"
;

const char* instanced_geom_shader = 
#include "shaders/instanced.geom"
;

const char* instanced_frag_shader = 
#include "shaders/instanced.frag"
;

int main(int argc, char* argv[]) {
    auto igdm = gdm::GDManager::inst(window_width, window_height, window_title);
    if (igdm.first) {
        std::cout << "GLFW manager started successfully." << std::endl;
    } else {
        std::cout << "GLFW could not initialize" << std::endl;
    }
    auto& gdman = igdm.second;
    GLFWwindow *window = gdman.getWindow();
    GUILite gui_lite(window);
    gui_lite.startup();

    // Meshes and instances
    auto floor_meshi = geom::c_floor();
    auto psys = psy::ParticleSystem();
    
    // Data binders
    auto floor_model_mat_data = [&floor_meshi](std::vector<glm::mat4>& data){ data[0] = floor_meshi.m_mat(); }; // This return model matrix for the floor.

    auto view_mat_data = [&gui_lite](std::vector<glm::mat4>& data){ data[0] = gui_lite.g_cam().vm(); };
    auto proj_mat_data = [](std::vector<glm::mat4>& data){ data[0] = glm::perspective((float) (kFov * (M_PI / 180.0f)), float(window_width) / window_height, kNear, kFar); };

    auto light_pos_data = [](std::vector<glm::vec4>& data){ data[0] = glm::vec4(1.0f, 0.0f, 0.0f, 0.0f); };
    auto cam_pos_data = [&gui_lite](std::vector<glm::vec3>& data){ data[0] = gui_lite.g_cam().g_pos(); };
    
    auto ambient_data = [](std::vector<glm::vec4>& data){ data[0] = glm::vec4(0.5f, 0.1f, 0.3f, 1.0f); };
    auto diffuse_data = [](std::vector<glm::vec4>& data){ data[0] = glm::vec4(0.5f, 0.2f, 0.3f, 1.0f); };
    auto specular_data = [](std::vector<glm::vec4>& data){ data[0] = glm::vec4(0.5f, 0.2f, 0.3f, 1.0f); };
    auto shininess_data = [](std::vector<float>& data){ data[0] = 50.0f; };
    
    // Shader uniforms
    std::cout << "Creating shader uniforms..." << std::endl;
    auto su_m_mat_floor = std::make_shared<rpa::CachedSU<glm::mat4, 1>>("model", rpa::UType::fm(4), floor_model_mat_data);
    
    auto su_v_mat = std::make_shared<rpa::CachedSU<glm::mat4, 1>>("view", rpa::UType::fm(4), view_mat_data);
    auto su_p_mat = std::make_shared<rpa::CachedSU<glm::mat4, 1>>("projection", rpa::UType::fm(4), proj_mat_data);
    
    auto su_l_pos = std::make_shared<rpa::CachedSU<glm::vec4, 1>>("light_position", rpa::UType::fv(4), light_pos_data);
    auto su_cam_pos = std::make_shared<rpa::CachedSU<glm::vec3, 1>>("camera_position", rpa::UType::fv(3), cam_pos_data);

    auto su_dif_sh = std::make_shared<rpa::CachedSU<glm::vec4, 1>>("diffuse", rpa::UType::fv(4), diffuse_data);
    auto su_spe_sh = std::make_shared<rpa::CachedSU<glm::vec4, 1>>("specular", rpa::UType::fv(4), specular_data);
    auto su_amb_sh = std::make_shared<rpa::CachedSU<glm::vec4, 1>>("ambient", rpa::UType::fv(4), ambient_data);
    auto su_shi_sh = std::make_shared<rpa::CachedSU<float, 1>>("shininess", rpa::UType::fs(), shininess_data);
    std::cout << "... done." << std::endl;

    // floor render pass
    std::cout << "Creating floor RenderPass..." << std::endl;
    rpa::RenderDataInput floor_pass_input;
    floor_pass_input.assign(0, "vertex_position", floor_meshi.m()->g_verts().data(), floor_meshi.m()->g_verts().size(), 4, GL_FLOAT);
    floor_pass_input.assignIndex(floor_meshi.m()->g_faces().data(), floor_meshi.m()->g_faces().size(), 3);
    rpa::RenderPass floor_pass(
        -1,
        floor_pass_input,
        {flat_vert_shader, NULL, NULL, geometry_shader, flat_frag_shader},
        {su_m_mat_floor, su_v_mat, su_p_mat, su_l_pos, su_cam_pos, su_dif_sh, su_amb_sh, su_spe_sh, su_shi_sh},
        {"fragment_color"}
    );
    std::cout << "... done." << std::endl;

    // psys render pass
    std::cout << "Creating psys RenderPass..." << std::endl;
    rpa::RenderDataInput psys_pass_input;
    psys_pass_input.assign(0, "vertex_position", psys.g_pmi().m()->g_verts().data(), psys.g_pmi().m()->g_verts().size(), 4, GL_FLOAT);
    psys_pass_input.assignIndex(psys.g_pmi().m()->g_faces().data(), psys.g_pmi().m()->g_faces().size(), 3);
    rpa::RenderPass psys_pass(
        -1,
        psys_pass_input,
        {instanced_vert_shader, NULL, NULL, instanced_geom_shader, instanced_frag_shader},
        {su_v_mat, su_p_mat, su_l_pos, su_cam_pos, su_dif_sh, su_amb_sh, su_spe_sh, su_shi_sh},
        {"fragment_color"}
    );

    // particle ubo
    const int PSYS_MODEL_UBO_ARR_SIZE = psys.g_pcnt();
    GLuint psys_model_ubo;
    GLuint psys_model_blk_idx;
    gdm::qd([&] GDM_OP {
        CHECK_GL_ERROR(glGenBuffers(1, &psys_model_ubo));
        CHECK_GL_ERROR(glBindBuffer(GL_UNIFORM_BUFFER, psys_model_ubo));
        CHECK_GL_ERROR(glBufferData(
            GL_UNIFORM_BUFFER,
            sizeof(float) * 4 * 4 * PSYS_MODEL_UBO_ARR_SIZE,
            psys.g_pmm().data(),
            GL_STREAM_DRAW
        ));
        std::cout << "linked program id: " << psys_pass.g_sp() << std::endl;
        CHECK_GL_ERROR(psys_model_blk_idx = glGetUniformBlockIndex(psys_pass.g_sp(), "models_array_block"));
        // bind ubo, at last
        std::cout << "uniform block id: " << psys_model_blk_idx << std::endl;
        CHECK_GL_ERROR(glUniformBlockBinding(psys_pass.g_sp(), psys_model_blk_idx, 0));
        CHECK_GL_ERROR(glBindBufferBase(GL_UNIFORM_BUFFER, 0, psys_model_ubo));
    });
    std::cout << "... done." << std::endl;


    std::cout << "Turning particle system on and setting parameters." << std::endl;
    psys.s_active(true);
    psys.s_grav(true);
    psys.s_plt(psy::lt_t(0.1f));
    std::cout << "Turned on." << std::endl;

    // main loop
    while (![&](void)->bool{
        bool should_close = false;
        gdm::qd([&] GDM_OP {
            should_close = glfwWindowShouldClose(window);
        });
        return should_close;
    }()) {
        psys.step(psy::lt_t(0.001f));
        gdm::qd([&] GDM_OP {
            // Setup some basic window stuff.
            glfwGetFramebufferSize(window, &window_width, &window_height);
            glViewport(0, 0, window_width, window_height);
            glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
            glEnable(GL_DEPTH_TEST);
            glEnable(GL_MULTISAMPLE);
            glEnable(GL_BLEND);
            glEnable(GL_CULL_FACE);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            glDepthFunc(GL_LESS);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            glCullFace(GL_BACK);

            if (psys.is_active()) {
                CHECK_GL_ERROR(glBindBuffer(GL_UNIFORM_BUFFER, psys_model_ubo));
                CHECK_GL_ERROR(glBufferData(
                    GL_UNIFORM_BUFFER,
                    sizeof(float) * 4 * 4 * PSYS_MODEL_UBO_ARR_SIZE,
                    psys.g_pmm().data(),
                    GL_STATIC_DRAW
                ));
            }

            // render floor
            floor_pass.setup();
            CHECK_GL_ERROR(glDrawElements(
                    GL_TRIANGLES,
                    floor_meshi.m()->g_faces().size() * 3,
                    GL_UNSIGNED_INT, 0
            ));

            if (psys.is_active()) {
                // render psys instanced
                psys_pass.setup();
                CHECK_GL_ERROR(glDrawElementsInstanced(
                    GL_TRIANGLES,
                    psys.g_pmi().m()->g_faces().size(),
                    GL_UNSIGNED_INT,
                    0,
                    psys.g_pcnt()
                ));
            }

            glfwSwapBuffers(window);

            glfwPollEvents();
        });
    }
    std::cout << "Shutting down." << std::endl;
    gdm::sync();

    gui_lite.shutdown();

    return EXIT_SUCCESS;
}
