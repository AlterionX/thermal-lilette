#include "prim_mesh.h"

#include "mesh.h"
#include "config.h"

#include <material.h>

#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/quaternion.hpp>

namespace geom {
    namespace {
        auto rect_prism = std::make_shared<TMesh>(std::vector<glm::vec4>{
            glm::vec4(0.5f, 0.5f, 0.5f, 1.0f), // top, right, out
            glm::vec4(0.5f, -0.5f, 0.5f, 1.0f), // bottom, ro
            glm::vec4(-0.5f, -0.5f, 0.5f, 1.0f), // b, left, o
            glm::vec4(-0.5f, 0.5f, 0.5f, 1.0f), // tlo
            glm::vec4(-0.5f, 0.5f, -0.5f, 1.0f), // tl, in
            glm::vec4(-0.5f, -0.5f, -0.5f, 1.0f), // bli
            glm::vec4(0.5f, -0.5f, -0.5f, 1.0f), // bri
            glm::vec4(0.5f, 0.5f, -0.5f, 1.0f) // tri
        }, std::vector<glm::uvec3>{
            glm::uvec3(0, 2, 1), // front bottom
            glm::uvec3(0, 3, 2), // f top
            glm::uvec3(3, 4, 5), // left t
            glm::uvec3(3, 5, 2), // lb
            glm::uvec3(4, 7, 6), // back t
            glm::uvec3(4, 6, 5), // bb
            glm::uvec3(7, 1, 6), // right t
            glm::uvec3(7, 0, 1), // rb
            glm::uvec3(7, 4, 0), // top t
            glm::uvec3(3, 0, 4), // tb
            glm::uvec3(1, 2, 6), // bottom t
            glm::uvec3(5, 6, 2) // bb
        }, std::vector<glm::vec4>{}, std::vector<glm::vec2>{}, std::vector<std::shared_ptr<Material>>{});
        auto plane = std::make_shared<TMesh>(std::vector<glm::vec4>{
            glm::vec4(0.5f, 0.0f, 0.5f, 1.0f),
            glm::vec4(0.5f, 0.0f, -0.5f, 1.0f),
            glm::vec4(-0.5f, 0.0f, -0.5f, 1.0f),
            glm::vec4(-0.5f, 0.0f, 0.5f, 1.0f)
        }, std::vector<glm::uvec3>{
            glm::uvec3(0, 1, 2),
            glm::uvec3(0, 2, 3)
        }, std::vector<glm::vec4>{}, std::vector<glm::vec2>{}, std::vector<std::shared_ptr<Material>>{});
        auto floor_plane = std::make_shared<TMesh>(std::vector<glm::vec4>{
            glm::vec4(kFloorXMin, kFloorY, kFloorZMax, 1.0f),
            glm::vec4(kFloorXMax, kFloorY, kFloorZMax, 1.0f),
            glm::vec4(kFloorXMax, kFloorY, kFloorZMin, 1.0f),
            glm::vec4(kFloorXMin, kFloorY, kFloorZMin, 1.0f)
        }, std::vector<glm::uvec3>{
            glm::uvec3(0, 1, 2),
            glm::uvec3(2, 3, 0)
        }, std::vector<glm::vec4>{
        }, std::vector<glm::vec2> {
            glm::vec2(0.0f, 1.0f),
            glm::vec2(1.0f, 1.0f),
            glm::vec2(1.0f, 0.0f),
            glm::vec2(0.0f, 0.0f)
        }, std::vector<std::shared_ptr<Material>>{});
        auto qfloor_plane = std::make_shared<QMesh>(std::vector<glm::vec4>{
            glm::vec4(kFloorXMin, kFloorY, kFloorZMax, 1.0f),
            glm::vec4(kFloorXMax, kFloorY, kFloorZMax, 1.0f),
            glm::vec4(kFloorXMax, kFloorY, kFloorZMin, 1.0f),
            glm::vec4(kFloorXMin, kFloorY, kFloorZMin, 1.0f),
        }, std::vector<glm::uvec4>{
            glm::uvec4(3, 0, 2, 1)
        }, std::vector<glm::vec4>{
        }, std::vector<glm::vec2> {
            glm::vec2(0.0f, 1.0f),
            glm::vec2(1.0f, 1.0f),
            glm::vec2(1.0f, 0.0f),
            glm::vec2(0.0f, 0.0f)
        }, std::vector<std::shared_ptr<Material>>{});
        auto sky_plane = std::make_shared<TMesh>(std::vector<glm::vec4>{
            glm::vec4(kSkyXMin, kSkyY, kSkyZMax, 1.0f),
            glm::vec4(kSkyXMax, kSkyY, kSkyZMax, 1.0f),
            glm::vec4(kSkyXMax, kSkyY, kSkyZMin, 1.0f),
            glm::vec4(kSkyXMin, kSkyY, kSkyZMin, 1.0f),
            glm::vec4(0.0, kSkyY, 0.0, 1.0f)
        }, std::vector<glm::uvec3>{
            glm::uvec3(4, 3, 2),
            glm::uvec3(4, 2, 1),
            glm::uvec3(4, 1, 0),
            glm::uvec3(4, 0, 3)
        }, std::vector<glm::vec4>{
        }, std::vector<glm::vec2> {
            glm::vec2(0.0f, 1.0f),
            glm::vec2(1.0f, 1.0f),
            glm::vec2(1.0f, 0.0f),
            glm::vec2(0.0f, 0.0f),
            glm::vec2(0.5f, 0.5f)
        }, std::vector<std::shared_ptr<Material>>{});
        auto sphere = [](float radius, int spoke_cnt, int tier_cnt) {
            std::vector<glm::vec4> obj_vertices;
            std::vector<glm::uvec3> obj_faces;

            for (int hu = 1; hu < tier_cnt - 1; ++hu) { // angle about z-axis
                double pitch_angle = hu * glm::pi<double>() / (tier_cnt - 1) + glm::pi<double>() / 2;
                for (int tu = 0; tu < spoke_cnt; ++tu) { // angle about y-axis
                    double yaw_angle = tu * 2 * glm::pi<double>() / spoke_cnt;
                    if (hu != tier_cnt - 2) {
                        // create the two tris
                        auto upper_face = glm::uvec3(
                            (hu - 1) * spoke_cnt + tu,
                            hu * spoke_cnt + tu,
                            (hu - 1) * spoke_cnt + (tu + 1) % spoke_cnt
                        );
                        auto bottom_face = glm::uvec3(
                            hu * spoke_cnt + (tu + 1) % spoke_cnt,
                            (hu - 1) * spoke_cnt + (tu + 1) % spoke_cnt,
                            hu * spoke_cnt + tu
                        );
                        obj_faces.push_back(upper_face);
                        obj_faces.push_back(bottom_face);
                    }
                    obj_vertices.push_back(glm::vec4(
                        radius * glm::cos(pitch_angle) * glm::sin(yaw_angle),
                        radius * glm::sin(pitch_angle),
                        radius * glm::cos(pitch_angle) * glm::cos(yaw_angle),
                        1.0f
                    ));
                }
            }
            // add caps
            obj_vertices.push_back(glm::vec4(0.0f, radius, 0.0f, 1.0f));
            obj_vertices.push_back(glm::vec4(0.0f, -radius, 0.0f, 1.0f));
            // faces for those caps
            for (int tu = 0; tu < spoke_cnt; ++tu) {
                auto topface = glm::uvec3(
                    obj_vertices.size() - 2,
                    tu, (tu + 1) % spoke_cnt
                );
                auto botface = glm::uvec3(
                    tu + (tier_cnt - 3) * spoke_cnt,
                    (tu + 1) % spoke_cnt + (tier_cnt - 3) * spoke_cnt,
                    obj_vertices.size() - 1
                );
                obj_faces.push_back(topface);
                obj_faces.push_back(botface);
            }
            return std::make_shared<TMesh>(obj_vertices, obj_faces, std::vector<glm::vec4>{}, std::vector<glm::vec2>{}, std::vector<std::shared_ptr<Material>>{});
        }(1.0f, 10, 10);
        auto line = std::make_shared<LMesh>(std::vector<glm::vec4>{
            glm::vec4(0.0f, 0.5f, 0.0f, 1.0f),
            glm::vec4(0.0f, -0.5f, 0.0f, 1.0f)
        }, std::vector<glm::uvec2>{
            glm::uvec2(0, 1)
        }, std::vector<glm::vec4>{}, std::vector<glm::vec2>{}, std::vector<std::shared_ptr<Material>>{});
        auto cylinder = [](const int& xSubDiv, const int& ySubDiv){
            /*
             * Create Cylinder from x = -0.5 to x = 0.5
             */
            float step_x = 1.0f / (xSubDiv - 1);
            float step_y = 1.0f / (ySubDiv - 1);
            glm::vec3 p = glm::vec3(-0.5f, 0.0f, 0.0f);

            // Setup the vertices of the lattice.
            // Note: vertex shader is used to generate the actual cylinder
            std::vector<glm::vec4> vertices;
            for (int i = 0; i < ySubDiv; ++i) {
                p.x = -0.5f;
                for (int j = 0; j < xSubDiv; ++j) {
                    vertices.push_back(glm::vec4(p, 1.0f));
                    p.x += step_x;
                }
                p.y += step_y;
            }

            // Compute the indices, this is just column / row indexing for the
            // vertical line segments and linear indexing for the horizontal
            // line segments.
            std::vector<glm::uvec2> indices;
            for (int n = 0; n < xSubDiv * ySubDiv; ++n) {
                int row = n / xSubDiv;
                int col = n % xSubDiv;
                if (col > 0) {
                    indices.emplace_back(n - 1, n);
                }
                if (row > 0) {
                    indices.emplace_back((row - 1) * xSubDiv + col, n);
                }
            }
            return std::make_shared<LMesh>(vertices, indices, std::vector<glm::vec4>{}, std::vector<glm::vec2>{}, std::vector<std::shared_ptr<Material>>{});
        }(16, 3);
        auto axes = std::make_shared<LMesh>(std::vector<glm::vec4>{
            glm::vec4(0.0f, 0.0f, 0.0f, 1.0f),
            glm::vec4(2.0f, 0.0f, 0.0f, 1.0f),
            glm::vec4(0.0f, 0.0f, 2.0f, 1.0f)
        }, std::vector<glm::uvec2>{
            glm::uvec2(0, 1),
            glm::uvec2(0, 2)
        }, std::vector<glm::vec4>{}, std::vector<glm::vec2>{}, std::vector<std::shared_ptr<Material>>{});
    }
    TMeshI c_rect_prism(const glm::vec3& center, const glm::vec3& dimen, const glm::fquat& rot) {
        return TMeshI(rect_prism, center, rot, dimen);
    }
    TMeshI c_cube(const glm::vec3& corner, const float& dimen, const glm::fquat& rot) {
        return c_rect_prism(corner, glm::vec3(dimen), rot);
    }
    TMeshI c_plane(const glm::vec3& center, const glm::vec2& dimen, const glm::fquat& rot) {
        return TMeshI(plane, center, rot, glm::vec3(dimen[0], 0.0f, dimen[1]));
    }
    TMeshI c_floor(void) {
        return TMeshI(floor_plane);
    }
    QMeshI c_qfloor(void) {
        return QMeshI(qfloor_plane);
    }
    TMeshI c_sky_plane(void) {
        return TMeshI(sky_plane);
    }
    LMeshI c_line(const glm::vec3& center, const float& length, const glm::fquat& rot) {
        glm::vec3 scale = glm::vec3(1.0f, length, 1.0f);
        return LMeshI(line, center, rot, scale);
    }
    LMeshI c_cylinder(const glm::vec3& center, const float& height, const float& radius, const glm::fquat& rot) {
        glm::vec3 scale = glm::vec3(radius / 2, height, radius / 2);
        return LMeshI(cylinder, center, rot, scale);
    }
    LMeshI c_axes(const glm::vec3 origin, const float& scale, const glm::fquat& rot) {
        return LMeshI(axes, origin, rot, glm::vec3(scale));
    }

    void cm_voxel(int w, int l, int h, std::vector<glm::vec4>& vert) {
        vert.empty();
        for(int i=0; i<w+1; i++)
            for(int j=0; j<l+1; j++)
                for(int k=0; k<h+1; k++)
                    vert.push_back(glm::vec4(
                        i,
                        k,
                        j,
                        1.0f));
    }
}
