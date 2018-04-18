#include "camera.h"

#include <glm/gtx/transform.hpp>
#include <glm/gtx/io.hpp>

#include <iostream>

namespace {
    glm::vec3 quat_to_v3(glm::fquat q) { return glm::vec3(q.x, q.y, q.z); }
    glm::vec3 q_rot(glm::fquat q, glm::vec3 v) { return v + 2.0f * glm::cross(glm::cross(v, quat_to_v3(q)) - q.w*v, quat_to_v3(q)); }
}

Camera::Camera() {
    orient = glm::lookAt(
        glm::vec3(0.0f, 0.1f, 30.0f),
        glm::vec3(0.0f, 0.0f, -1.0f),
        glm::vec3(0.0f, 1.0f, 0.0f)
    );
}
const glm::vec3& Camera::g_pos(void) const {
    return pos;
}
Camera::Camera(glm::fquat orient, glm::vec3 pos) : orient(orient), pos(pos) {
}
glm::mat4 Camera::vm(void) const {
    return glm::mat4_cast(orient) * glm::translate(pos);
}
void Camera::trans(const glm::vec3& trans) { pos += q_rot(glm::conjugate(orient), trans); }
void Camera::rot(const float& pitch, const float& yaw, const float& roll) {
    auto rot_p = glm::normalize(glm::cross(orient, glm::fquat(glm::cos(pitch), glm::sin(pitch), 0.0f, 0.0f)));
    auto rot_py = glm::normalize(glm::cross(glm::conjugate(orient), glm::cross(rot_p, glm::cross(orient, glm::fquat(glm::cos(yaw), 0.0f, glm::sin(yaw), 0.0f)))));
    orient = glm::cross(rot_py, glm::fquat(glm::cos(roll), 0.0f, 0.0f, glm::sin(roll)));
}
                