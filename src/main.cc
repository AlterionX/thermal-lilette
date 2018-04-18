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
const std::string window_title = "Minecraft";

int subt_size = 128;
int height_size = 128;
int gv_size = 5;

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

const char* floor_vert_shader =
#include "shaders/floor.vert"
;

const char* floor_tcs_shader =
#include "shaders/floor.tcs"
;

const char* floor_tes_shader =
#include "shaders/floor.tes"
;

const char* floor_geom_shader =
#include "shaders/floor.geom"
;

const char* floor_fragment_shader =
#include "shaders/floor.frag"
;

const char* voxel_vert_shader =
#include "shaders/voxel.vert"
;

const char* voxel_fragment_shader =
#include "shaders/voxel.frag"
;

const char* sky_fragment_shader =
#include "shaders/sky.frag"
;

const char* bone_vertex_shader =
#include "shaders/bone.vert"
;

const char* bone_fragment_shader =
#include "shaders/bone.frag"
;

const char* cylinder_vertex_shader =
#include "shaders/cylinder.vert"
;

const char* cylinder_frag_shader =
#include "shaders/cylinder.frag"
;

const char* axes_vertex_shader =
#include "shaders/axes.vert"
;

const char* axes_frag_shader =
#include "shaders/axes.frag"
;

const char* preview_vertex_shader =
#include "shaders/preview.vert"
;

const char* preview_frag_shader =
#include "shaders/preview.frag"
;

const char* scroll_bar_vertex_shader =
#include "shaders/scroll_bar.vert"
;

const char* scroll_bar_frag_shader =
#include "shaders/scroll_bar.frag"
;

const char* flat_vert_shader =
#include "shaders/flat.vert"
;

const char* flat_frag_shader =
#include "shaders/flat.frag"
;

void regen_terrain(Terrain& ter, glm::vec2 pos, int stuffs[3][3], glm::vec2& c_blk) { // 9 of these
    glm::ivec2 blk = glm::ivec2(glm::ceil(pos / 256.0f)) * -1;
    c_blk = blk;
    for (int i = -1; i < 2; i++) {
        for (int j = -1; j < 2; j++) {
            ter.getSubTerrain(blk[0] + i, blk[1] + j);
            stuffs[i + 1][j + 1] = ter.getPosId(blk[0] + i, blk[1] + j);
        }
    }
}
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

    auto cube_meshi = geom::c_cube(glm::vec3(0.0f), 1000.0f);
    auto floor_meshi = geom::c_floor();
    auto sky_meshi = geom::c_sky_plane();
    std::vector<glm::vec4> voxel_vert;
    geom::cm_voxel(subt_size, subt_size, height_size, voxel_vert);

    glm::vec4 light_pos_f = glm::vec4(1.0f, 0.0f, 0.0f, 0.0f);

    auto floor_model_mat_data = [&floor_meshi](std::vector<glm::mat4>& data){ data[0] = floor_meshi.m_mat(); }; // This return model matrix for the floor.
    auto cube_model_mat_data = [&cube_meshi](std::vector<glm::mat4>& data){ data[0] = cube_meshi.m_mat(); }; // This return model matrix for the floor.

    auto cam_pos_data = [&gui_lite](std::vector<glm::vec3>& data){ data[0] = gui_lite.g_cam().g_pos(); };

    auto view_mat_data = [&gui_lite](std::vector<glm::mat4>& data){ data[0] = gui_lite.g_cam().vm(); };

    auto proj_mat_data = [](std::vector<glm::mat4>& data){ data[0] = glm::perspective((float) (kFov * (M_PI / 180.0f)), float(window_width) / window_height, kNear, kFar); };

    auto light_pos_data = [&light_pos_f](std::vector<glm::vec4>& data){ data[0] = light_pos_f; };
    auto ambient_data = [](std::vector<glm::vec4>& data){ data[0] = glm::vec4(0.5f, 0.1f, 0.3f, 1.0f); };
    auto diffuse_data = [](std::vector<glm::vec4>& data){ data[0] = glm::vec4(0.5f, 0.2f, 0.3f, 1.0f); };
    auto specular_data = [](std::vector<glm::vec4>& data){ data[0] = glm::vec4(0.5f, 0.2f, 0.3f, 1.0f); };
    auto shininess_data = [](std::vector<float>& data){ data[0] = 50.0f; };

    // TERRAIN TEXTURE GENERATION
    Terrain ter({gv_size, gv_size, subt_size, subt_size, height_size}, glm::vec3(0.0, 0.0, 0.0));
    GLuint ter_tex;
    ter.create();
    ter.getSubTerrain(0, 1);
    ter.getSubTerrain(1, 0);
    ter.getSubTerrain(1, 1); // switch this up front to see the glitch
    int voxel_mesh_ids[3][3];
    int ter_y = 0;
    int ter_x = 0;
    glm::vec2 c_blk = glm::vec2(0.0f, 0.0f);
    regen_terrain(ter, glm::vec2(0.0f), voxel_mesh_ids, c_blk);
    auto voxel_model_mat_data = [&ter_x, &ter_y, &c_blk](std::vector<glm::mat4>& data){
        float scale = 2.0f;
        data[0] = glm::translate(glm::vec3(ter_x + c_blk[0] - 1, 0.0f, ter_y + c_blk[1] - 1) * 128.0f * scale + glm::vec3(0.0f, -128.0f, 0.0f))
            * glm::scale(glm::vec3(scale));
    };
    float sky_time_f = 0.0;

    auto texture_binder = [&ter_tex](int loc, int cnt, const void *data) {
        // std::cout << "Bind ter height" << std::endl; // TODO: not show up during rendering?
        CHECK_GL_ERROR(glBindTexture(GL_TEXTURE_2D, *((const GLuint*) data)));
    };
    auto ter_height_data = [&ter, &ter_x, &ter_y](std::vector<const GLuint*>& data) {
        data[0] = ter.getSubTerrain(ter_x, ter_y).getTexture();
    };
    auto ter_shift_data = [&ter, &ter_x, &ter_y](std::vector<glm::vec3>& data) {
         // std::cout << "focus " << ter_focus_id << std::endl; // TODO: not show up during rendering?
        data[0] = ter.getSubTerrain(ter_x, ter_y).getStartPos();
    };
    auto sky_time_data = [&sky_time_f](std::vector<float>& data) {
         // std::cout << "skyyyyyy" << std::endl; // TODO: not show up during rendering?
        data[0] = sky_time_f;
    };
    auto cloud_freq_data = [&gui_lite](std::vector<float>& data) {
         // std::cout << "skyyyyyy" << std::endl; // TODO: not show up during rendering?
        data[0] = gui_lite.cloud_freq_;
    };

    std::cout << "Creating shader uniforms..." << std::endl;
    auto su_m_mat_floor = std::make_shared<rpa::CachedSU<glm::mat4, 1>>("model", rpa::UType::fm(4), floor_model_mat_data);
    auto su_m_mat_cube = std::make_shared<rpa::CachedSU<glm::mat4, 1>>("model", rpa::UType::fm(4), cube_model_mat_data);
    auto su_m_mat_voxel = std::make_shared<rpa::CachedSU<glm::mat4, 1>>("model", rpa::UType::fm(4), voxel_model_mat_data);
    auto su_v_mat = std::make_shared<rpa::CachedSU<glm::mat4, 1>>("view", rpa::UType::fm(4), view_mat_data);
    auto su_p_mat = std::make_shared<rpa::CachedSU<glm::mat4, 1>>("projection", rpa::UType::fm(4), proj_mat_data);
    auto su_cam_pos = std::make_shared<rpa::CachedSU<glm::vec3, 1>>("camera_position", rpa::UType::fv(3), cam_pos_data);
    auto su_l_pos = std::make_shared<rpa::CachedSU<glm::vec4, 1>>("light_position", rpa::UType::fv(4), light_pos_data);

    // bland
    auto su_dif_sh = std::make_shared<rpa::CachedSU<glm::vec4, 1>>("diffuse", rpa::UType::fv(4), diffuse_data);
    auto su_amb_sh = std::make_shared<rpa::CachedSU<glm::vec4, 1>>("ambient", rpa::UType::fv(4), ambient_data);
    auto su_spe_sh = std::make_shared<rpa::CachedSU<glm::vec4, 1>>("specular", rpa::UType::fv(4), specular_data);
    auto su_shi_sh = std::make_shared<rpa::CachedSU<float, 1>>("shininess", rpa::UType::fs(), shininess_data);

    auto ter_height = std::make_shared<rpa::CachedSU<const GLuint*, 1>>("ter_height", texture_binder, ter_height_data);
    auto ter_shift = std::make_shared<rpa::CachedSU<glm::vec3, 1>>("ter_shift", rpa::UType::fv(3), ter_shift_data);

    auto sky_time = std::make_shared<rpa::CachedSU<float, 1>>("sky_time", rpa::UType::fs(), sky_time_data);
    auto cloud_freq = std::make_shared<rpa::CachedSU<float, 1>>("cloud_freq", rpa::UType::fs(), cloud_freq_data);
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

    // Floor render pass
    std::cout << "Creating floor RenderPass..." << std::endl;
    rpa::RenderDataInput floor_pass_input;
    floor_pass_input.assign(0, "vertex_position", floor_meshi.m()->g_verts().data(), floor_meshi.m()->g_verts().size(), 4, GL_FLOAT);
    floor_pass_input.assign(1, "uv", floor_meshi.m()->g_texcs().data(),
                               floor_meshi.m()->g_texcs().size(), 2, GL_FLOAT);
    floor_pass_input.assignIndex(floor_meshi.m()->g_faces().data(), floor_meshi.m()->g_faces().size(), 4);
    rpa::RenderPass floor_pass(-1,
        floor_pass_input,
        // {floor_vert_shader, floor_tcs_shader, floor_tes_shader, floor_geom_shader, floor_fragment_shader},
        {vertex_shader, NULL, NULL, geometry_shader, floor_fragment_shader},
        {su_m_mat_floor, su_v_mat, su_p_mat, su_l_pos, ter_height},
        {"fragment_color"}
    );
    std::cout << "... done." << std::endl;

    // Voxel render pass
    std::cout << "Creating voxel RenderPass..." << std::endl;
    std::vector<std::vector<rpa::RenderPass>> voxel_passes;
    voxel_passes.reserve(3);
    for (int i = 0; i < 3; i++) {
        voxel_passes.emplace_back();
        voxel_passes[i].reserve(3);
        for (int j = 0; j < 3; j++) {
            SubTerrain& sub_ter = ter.getSubTerrain(voxel_mesh_ids[i][j]);
            rpa::RenderDataInput voxel_pass_input;
            voxel_pass_input.assign(0, "vertex_position", voxel_vert.data(), voxel_vert.size(), 4, GL_FLOAT);
            voxel_pass_input.assignIndex(sub_ter.getVoxelFace()->data(), sub_ter.getVoxelFace()->size(), 4);
            voxel_passes[i].emplace_back(rpa::RenderPass(
                -1,
                voxel_pass_input,
                {voxel_vert_shader, NULL, NULL, geometry_shader, voxel_fragment_shader},
                {su_m_mat_voxel, su_v_mat, su_p_mat, su_l_pos, ter_height, ter_shift, sky_time},
                {"fragment_color"}
            ));
        }
    }
    std::cout << "... done." << std::endl;

    // Sky render pass
    std::cout << "Creating sky RenderPass..." << std::endl;
    rpa::RenderDataInput sky_pass_input;
    sky_pass_input.assign(0, "vertex_position", sky_meshi.m()->g_verts().data(), sky_meshi.m()->g_verts().size(), 4, GL_FLOAT);
    sky_pass_input.assign(1, "uv", sky_meshi.m()->g_texcs().data(),
                               sky_meshi.m()->g_texcs().size(), 2, GL_FLOAT);
    sky_pass_input.assignIndex(sky_meshi.m()->g_faces().data(), sky_meshi.m()->g_faces().size(), 4);
    rpa::RenderPass sky_pass(-1,
        sky_pass_input,
        {vertex_shader, NULL, NULL, geometry_shader, sky_fragment_shader},
        {su_m_mat_floor, su_v_mat, su_p_mat, su_l_pos, sky_time, cloud_freq},
        {"fragment_color"}
    );
    std::cout << "... done." << std::endl;

    auto center = std::chrono::steady_clock::now();
    float prev_time_f = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - center).count();

    std::cout << "Beginning main loop" << std::endl;

    while (![&](void)->bool{
        bool should_close = false;
        gdm::qd([&] GDM_OP {
            should_close = glfwWindowShouldClose(window);
        });
        return should_close;
    }()) {
        auto c_pos = gui_lite.g_cam().g_pos();
        regen_terrain(ter, glm::vec2(c_pos[0], c_pos[2]), voxel_mesh_ids, c_blk);
        gdm::qd([&] GDM_OP {
            float cur_time_f = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - center).count();
            sky_time_f += (cur_time_f - prev_time_f) / 5e5 * gui_lite.time_speed_;
            prev_time_f = cur_time_f;
            // std::cout << sky_time_f << glm::to_string(light_pos_f) << std::endl;

            // Setup some basic window stuff.
            glfwGetFramebufferSize(window, &window_width, &window_height);
            glViewport(0, 0, window_width, window_height);
            if((int)(sky_time_f*2)%2 ==1) { // night
                glClearColor(0.02f, 0.0f, 0.1f, 0.0f);
                light_pos_f = glm::vec4(cos(2 * M_PI * sky_time_f + M_PI), sin(2 * M_PI * sky_time_f + M_PI), 0.0f, 0.0f);
            }
            else { // day
                glClearColor(0.5f, 0.5f, 1.0f, 0.0f);
                light_pos_f = glm::vec4(cos(2 * M_PI * sky_time_f), sin(2 * M_PI * sky_time_f), 0.0f, 0.0f);
            }
            glEnable(GL_DEPTH_TEST);
            glEnable(GL_MULTISAMPLE);
            glEnable(GL_BLEND);
            glEnable(GL_CULL_FACE);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            glDepthFunc(GL_LESS);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            glCullFace(GL_BACK);

            // render floor
            // floor_pass.setup();
            // CHECK_GL_ERROR(glBindTexture(GL_TEXTURE_2D, *ter.getSubTerrainTexture(0))); // suppose to be this
            // // CHECK_GL_ERROR(glDrawElements(
            // //         GL_PATCHES,
            // //         floor_meshi.m()->g_faces().size() * 4,
            // //         GL_UNSIGNED_INT, 0
            // // ));
            // CHECK_GL_ERROR(glDrawElements(
            //         GL_TRIANGLES,
            //         floor_meshi.m()->g_faces().size() * 3,
            //         GL_UNSIGNED_INT, 0
            // ));

            // render voxels
            for(ter_x = 0; ter_x < 3; ter_x++) {
                for (ter_y = 0; ter_y < 3; ter_y++) {
                    voxel_passes[ter_x][ter_y].setup();
                    voxel_passes[ter_x][ter_y].updateIndexVBO(
                        ter.getSubTerrain(voxel_mesh_ids[ter_x][ter_y]).getVoxelFace()->data(),
                        ter.getSubTerrain(voxel_mesh_ids[ter_x][ter_y]).getVoxelFace()->size()
                    );
                    // TODO rebind faces
                    CHECK_GL_ERROR(glDrawElements(
                            GL_TRIANGLES,
                            ter.getSubTerrain(voxel_mesh_ids[ter_x][ter_y]).getVoxelFace()->size() * 3,
                            GL_UNSIGNED_INT, 0
                    ));
                }
            }

            // render cube
            cube_pass.setup();
            CHECK_GL_ERROR(glDrawElements(
                    GL_TRIANGLES,
                    cube_meshi.m()->g_faces().size() * 3,
                    GL_UNSIGNED_INT, 0
            ));

            // render sky
            sky_pass.setup();
            CHECK_GL_ERROR(glDrawElements(
                    GL_TRIANGLES,
                    sky_meshi.m()->g_faces().size() * 3,
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
