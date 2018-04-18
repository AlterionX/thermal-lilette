#pragma once

#ifndef __PRIM_MESH_H__
#define __PRIM_MESH_H__

#include "mesh.h"

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

namespace geom {
    TMeshI c_rect_prism(
        const glm::vec3& corner = glm::vec3(0.5f),
        const glm::vec3& dimen = glm::vec3(1.0f),
        const glm::fquat& rot = glm::fquat(1.0f, 0.0f, 0.0f, 0.0f)
    );
    TMeshI c_cube(
        const glm::vec3& corner = glm::vec3(0.5f),
        const float& dimen = 1.0f,
        const glm::fquat& rot = glm::fquat(1.0f, 0.0f, 0.0f, 0.0f)
    );
    TMeshI c_plane(
        const glm::vec3& corner = glm::vec3(0.0f, 0.0f, 0.0f),
        const glm::vec2& dimen = glm::vec2(1.0f, 1.0f),
        const glm::fquat& rot = glm::fquat(1.0f, 0.0f, 0.0f, 0.0f)
    );
    TMeshI c_ui_plane(
        const glm::vec3& corner = glm::vec3(0.0f, 0.0f, 0.0f),
        const glm::vec2& dimen = glm::vec2(1.0f, 1.0f),
        const glm::fquat& rot = glm::fquat(0.0f, 1.0f, 0.0f, 0.0f) // should rotate to vertical around the x plane, might need to flip this for NDC
    );
    TMeshI c_floor(void);
    QMeshI c_qfloor(void);
    TMeshI c_sky_plane(void);
    LMeshI c_line(
        const glm::vec3& start = glm::vec3(0.0f, 0.0f, 0.0f),
        const float& length = 1.0f,
        const glm::fquat& rot = glm::fquat(1.0f, 0.0f, 0.0f, 0.0f)
    );
    LMeshI c_cylinder(
        const glm::vec3& start = glm::vec3(0.0f, -0.5f, 0.0f),
        const float& radius = 1.0f,
        const float& height = 1.0f,
        const glm::fquat& rot = glm::fquat(1.0f, 0.0f, 0.0f, 0.0f)
    );
    LMeshI c_axes(
        const glm::vec3 origin = glm::vec3(0.0f, -0.5f, 0.0f),
        const float& scale = 1.0f,
        const glm::fquat& rot = glm::fquat(1.0f, 0.0f, 0.0f, 0.0f)
    );

    // width, length, height -> [OpenGL] x, z, y
    void cm_voxel(int w, int l, int h, std::vector<glm::vec4>& vert);
    // TMesh cm_voxel(glm::vec3 dimen);
}

#endif
