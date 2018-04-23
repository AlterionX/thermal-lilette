#include "glfw_dispatch.h"
#include "render_pass.h"
#include "config.h"
#include "prim_mesh.h"
#include "gui.h"
#include "terrain.h"

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
    auto cube_meshi = geom::c_cube(glm::vec3(0.0f), 1000.0f);
    
    // Data binders
    auto cube_model_mat_data = [&cube_meshi](std::vector<glm::mat4>& data){ data[0] = cube_meshi.m_mat(); }; // This return model matrix for the floor.

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
    auto su_m_mat_cube = std::make_shared<rpa::CachedSU<glm::mat4, 1>>("model", rpa::UType::fm(4), cube_model_mat_data);
    
    auto su_v_mat = std::make_shared<rpa::CachedSU<glm::mat4, 1>>("view", rpa::UType::fm(4), view_mat_data);
    auto su_p_mat = std::make_shared<rpa::CachedSU<glm::mat4, 1>>("projection", rpa::UType::fm(4), proj_mat_data);
    
    auto su_l_pos = std::make_shared<rpa::CachedSU<glm::vec4, 1>>("light_position", rpa::UType::fv(4), light_pos_data);
    auto su_cam_pos = std::make_shared<rpa::CachedSU<glm::vec3, 1>>("camera_position", rpa::UType::fv(3), cam_pos_data);

    auto su_dif_sh = std::make_shared<rpa::CachedSU<glm::vec4, 1>>("diffuse", rpa::UType::fv(4), diffuse_data);
    auto su_spe_sh = std::make_shared<rpa::CachedSU<glm::vec4, 1>>("specular", rpa::UType::fv(4), specular_data);
    auto su_amb_sh = std::make_shared<rpa::CachedSU<glm::vec4, 1>>("ambient", rpa::UType::fv(4), ambient_data);
    auto su_shi_sh = std::make_shared<rpa::CachedSU<float, 1>>("shininess", rpa::UType::fs(), shininess_data);
    std::cout << "... done." << std::endl;

    // Cube render pass
    std::cout << "Creating cube RenderPass..." << std::endl;
    rpa::RenderDataInput cube_pass_input;
    cube_pass_input.assign(0, "vertex_position", cube_meshi.m()->g_verts().data(), cube_meshi.m()->g_verts().size(), 4, GL_FLOAT);
    cube_pass_input.assignIndex(cube_meshi.m()->g_faces().data(), cube_meshi.m()->g_faces().size(), 3);
    rpa::RenderPass cube_pass(
        -1,
        cube_pass_input,
        {flat_vert_shader, NULL, NULL, geometry_shader, flat_frag_shader},
        {su_m_mat_cube, su_v_mat, su_p_mat, su_l_pos, su_cam_pos, su_dif_sh, su_amb_sh, su_spe_sh, su_shi_sh},
        {"fragment_color"}
    );
    std::cout << "... done." << std::endl;

    // main loop
    while (![&](void)->bool{
        bool should_close = false;
        gdm::qd([&] GDM_OP {
            should_close = glfwWindowShouldClose(window);
        });
        return should_close;
    }()) {
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

            // render cube
            cube_pass.setup();
            CHECK_GL_ERROR(glDrawElements(
                    GL_TRIANGLES,
                    cube_meshi.m()->g_faces().size() * 3,
                    GL_UNSIGNED_INT, 0
            ));

            glfwSwapBuffers(window);

            glfwPollEvents();
        });
    }
    std::cout << "Shutting down." << std::endl;
    gdm::sync();

    gui_lite.shutdown();

    return EXIT_SUCCESS;
}
