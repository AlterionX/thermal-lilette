#ifndef SKINNING_GUI_H
#define SKINNING_GUI_H

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <chrono>

#include "input_management.h"
#include "anim_mesh.h"
#include "stopwatch.h"
#include "camera.h"

struct MeshI;

/*
 * Hint: call glUniformMatrix4fv on thest pointers
 */
struct MatrixPointers {
    const float *projection, *model, *view;
};

struct Pane {
    glm::vec2 p;
    glm::vec2 sz;

    bool contains(const glm::vec2&) const;
};

class GUI {
public:
    GUI(GLFWwindow*, int view_width = -1, int view_height = -1, int preview_height = -1);
    ~GUI();
    void assignMI(MeshI*);

    void updateMatrices();
    MatrixPointers getMatrixPointers() const;

    glm::vec3 getCenter() const { return center_; }
    const glm::vec3& getCamera() const { return eye_; }
    bool isPoseDirty() const { return pose_changed_; }
    void clearPose() { pose_changed_ = false; }
    const float* getLightPositionPtr() const { return &light_position_[0]; }

    int getCurrentBone() const { return current_bone_; }
    const int* getCurrentBonePointer() const { return &current_bone_; }
    bool setCurrentBone(int i);
    const glm::mat4& getBoneTransform();

    bool isTransparent(void) const { return transparent_; }
    bool isPlaying(void) const { return !anim_time.isPaused(); }
    std::chrono::duration<float> getCAnimTime(void) const { return anim_time.elapsed(); }

    void startup(void);
    void shutdown(void);

    void screenshotSnap(void);
    void screenshotSave(void);
    void screenshotIfShould(void);

    const float* getPreviewFShift() const { return &pview_fshift_; }
    const glm::mat4& getPreviewOrthoSc();

private:
    GLFWwindow * window_;
    MeshI* mi;

    Pane pane_all;
    Pane pane_view;
    Pane pane_preview;

    std::chrono::StopWatch anim_time;
    int cfid = -1;

    iom::IOManager ioman;
    void register_cbs(void);

    bool fps_mode_ = false;
    bool pose_changed_ = true;
    bool transparent_ = false;

    glm::vec4 light_position_;

    float pan_speed_ = 0.1f;
    float rotation_speed_ = 0.14f;
    float zoom_speed_ = 0.1f;

    float camera_distance_ = 30.0;
    float aspect_;
    glm::vec3 eye_ = glm::vec3(0.0f, 0.1f, camera_distance_);
    glm::vec3 up_ = glm::vec3(0.0f, 1.0f, 0.0f);
    glm::vec3 look_ = glm::vec3(0.0f, 0.0f, -1.0f);
    glm::vec3 tangent_ = glm::cross(look_, up_);
    glm::vec3 center_ = eye_ - camera_distance_ * look_;
    glm::mat3 orientation_ = glm::mat3(tangent_, up_, look_);

    glm::mat4 view_matrix_ = glm::lookAt(eye_, center_, up_);
    glm::mat4 projection_matrix_;
    glm::vec3 windowToWorld(const glm::vec2& pos);
    glm::vec2 windowToNdc(const glm::vec2& pos);
    glm::vec2 worldToNdcNear(const glm::vec3& pos);

    int current_bone_ = -1;
    float roll_speed_ = M_PI / 64.0f;

    bool bmatrix_dirty = false;
    glm::mat4 bone_matrix_ = glm::mat4(1.0f);

    bool take_screenshot = false;
    bool save_screenshot = false;
    std::chrono::duration<double> preview_screenshot = std::chrono::duration<double>(-10);
    int screenshot_cnt = 0;
    unsigned char* screenshot = NULL;

    float pview_fshift_ = 0.0f;
    glm::mat4 ortho_sc_ = glm::mat4(1.0f);

};

class GUILite {
    GLFWwindow * win;
    Pane pane_win;

    Camera cam;

    iom::IOManager ioman;
    void register_cbs(void);
public:
    GUILite(GLFWwindow* window);
    GUILite() = delete;
    ~GUILite();
    
    void startup(void);
    void shutdown(void);

    const Camera& g_cam(void) const;

    float time_speed_ = 1.0;
    float cloud_freq_ = 500.0;
};


#endif
