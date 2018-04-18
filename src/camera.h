#pragma once

#ifndef __CAMERA_H__
#define __CAMERA_H__

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

class Camera {
    glm::fquat orient = glm::fquat(1.0f, 0.0f, 0.0f, 0.0f);
    glm::vec3 pos = glm::vec3(0.0f, 0.0f, 0.0f);
public:
    Camera();
    Camera(glm::fquat orient, glm::vec3 pos);
    glm::mat4 vm(void) const;
    const glm::vec3& g_pos(void) const;
    void trans(const glm::vec3& trans);
    void rot(const float& pitch, const float& yaw, const float& roll);
};

#endif