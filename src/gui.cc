#include "gui.h"

#include <jpegio.h>
#include <iostream>
#include <cstring>
#include <glm/gtc/matrix_access.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/string_cast.hpp> // TODO: remove this

#include "config.h"
#include "glfw_dispatch.h"


namespace {
    glm::vec2 w_to_ndc(const glm::vec2& pos, const Pane& pane) {
        return 2.0f * (pos / pane.sz - 0.5f);
    }
    glm::vec3 w_to_world(const glm::vec2& pos, const glm::mat4& p_mat, const glm::mat4& v_mat, const Pane& pane) {
        glm::vec2 px = w_to_ndc(pos, pane);
        auto sub = glm::inverse(p_mat * v_mat) * glm::vec4(px, -1.0f, 1.0f);
        return glm::vec3(sub / sub[3]);
    }
    glm::vec2 world_to_ndc(const glm::vec3& pos, const glm::mat4& p_mat, const glm::mat4& v_mat, const glm::vec3& eye, const glm::vec3& look) {
        glm::vec3 p_dir = glm::normalize(pos - eye);
        glm::vec3 w_p_pos = eye + p_dir * glm::dot(look, look) / glm::dot(p_dir, look);
        return glm::vec2(p_mat * v_mat * glm::vec4(w_p_pos, 1.0f));
    }
    float intersectBoneDist(glm::vec3 p1, glm::vec3 p2, glm::vec3 rd, glm::vec3 ro) {
        glm::vec3 pd = glm::normalize(p2 - p1);
        glm::vec3 n = glm::cross(pd, rd);
        glm::vec3 rn = glm::cross(rd, n);
        glm::vec3 pn = glm::cross(pd, n);
        glm::vec3 rc = ro + rd * glm::dot(p1 - ro, pn) / glm::dot(rd, pn);
        glm::vec3 pc = p1 + pd * glm::dot(ro - p1, rn) / glm::dot(pd, rn);

        if (glm::distance(rc, pc) > kCylinderRadius) // out of radius
            return -1;
        if (glm::distance(p1, p2) < glm::distance(p1, pc)
            || glm::distance(p1, p2) < glm::distance(p2, pc)) // out of interval
            return -1;
        if (glm::distance(rc, ro) < kNear)
            return -1;

        return glm::distance(rc, ro);
    }
    int intersectBoneID(SkelI &skeleton, glm::vec3 rd, glm::vec3 ro) {
        float d = std::numeric_limits<float>::max();
        int bone_id = -1;
        for (size_t i = 0; i < skeleton.jcnt_c(); i++) {
            const auto& joint = skeleton.j_c(i);
            if (skeleton.pid_c(i) < 0) continue;
            glm::vec3 j_pos = skeleton.jpos_c(i);
            glm::vec3 p_pos = skeleton.ppos_c(i);
            float di = intersectBoneDist(j_pos, p_pos, rd, ro);
            if (di > 0 && (bone_id == -1 || d > di)) {
                bone_id = i;
                d = di;
            }
        }
        return bone_id;
    }
}

bool Pane::contains(const glm::vec2& pos) const {
    auto rp = pos - p;
    return glm::all(glm::greaterThanEqual(rp, glm::vec2(0.0f)) && glm::lessThan(rp, sz));
}

GUI::GUI(GLFWwindow* window, int view_w, int view_h, int preview_h)
    : window_(window) {
    int win_h, win_w;
    glfwGetWindowSize(window_, &win_w, &win_h);
    if (view_w < 0 || view_h < 0) {
        view_w = win_w;
        view_h = win_h;
    }
    pane_all = Pane {{0.0f, 0.0f}, {win_w, win_h}};
    pane_view = Pane {{0.0f, 0.0f}, {view_w, view_h}};
    pane_preview = Pane {{view_w, 0.0f}, {win_w - view_w, preview_h}};

    float aspect_ = pane_view.sz[0] / pane_view.sz[1];

    projection_matrix_ = glm::perspective((float) (kFov * (M_PI / 180.0f)), aspect_, kNear, kFar);

    register_cbs();
}

GUI::~GUI() {}

void GUI::startup() {
    ioman.activate(window_);
}

void GUI::shutdown() {
    ioman.deactivate(window_);
}

void GUI::assignMI(MeshI* mesh) {
    mi = mesh;
    center_ = mi->m->getCenter();
}

glm::vec3 GUI::windowToWorld(const glm::vec2& pos) {
    return w_to_world(pos, projection_matrix_, view_matrix_, pane_all);
}

glm::vec2 GUI::windowToNdc(const glm::vec2& pos) {
    return w_to_ndc(pos, pane_all);
}

glm::vec2 GUI::worldToNdcNear(const glm::vec3& pos) {
    return world_to_ndc(pos, projection_matrix_, view_matrix_, eye_, look_);
}

void GUI::updateMatrices() {
    // Compute our view, and projection matrices.
    if (fps_mode_)
        center_ = eye_ + camera_distance_ * look_;
    else
        eye_ = center_ - camera_distance_ * look_;

    view_matrix_ = glm::lookAt(eye_, center_, up_);
    light_position_ = glm::vec4(eye_, 1.0f);

    aspect_ = pane_view.sz[0] / pane_view.sz[1];
    projection_matrix_ =
        glm::perspective((float) (kFov * (M_PI / 180.0f)), aspect_, kNear, kFar);
}

MatrixPointers GUI::getMatrixPointers() const {
    MatrixPointers ret;
    ret.projection = &projection_matrix_[0][0];
    ret.model = &mi->mm()[0][0];
    ret.view = &view_matrix_[0][0];
    return ret;
}

bool GUI::setCurrentBone(int i) {
    if (i < 0 || i >= mi->m->boneCnt()) return false;
    current_bone_ = i;
    bmatrix_dirty = true;
    return true;
}
const glm::mat4& GUI::getBoneTransform() {
    if (bmatrix_dirty || isPlaying()) {
        glm::vec3 j_pos = mi->s_inst().jpos_c(current_bone_);
        glm::vec3 p_pos = mi->s_inst().ppos_c(current_bone_);
        glm::vec3 p_dir = glm::normalize(j_pos - p_pos);
        glm::vec3 n_dir = glm::cross(glm::vec3(0.0, 1.0, 0.0), p_dir);
        float angle = glm::acos(p_dir[1]);
        bone_matrix_ = glm::translate(p_pos)
            * glm::rotate(angle, n_dir)
            * glm::scale(glm::vec3(kCylinderRadius, glm::distance(j_pos, p_pos), kCylinderRadius))
            ;
        bmatrix_dirty = false;
    }
    return bone_matrix_;
}
void GUI::screenshotSnap() {
    if(screenshot != NULL) free(screenshot);
    screenshot = (unsigned char*) malloc (sizeof(unsigned char)
                    * int(pane_view.sz[0] * pane_view.sz[1]) * 3);
    glReadPixels(0, 0, pane_view.sz[0], pane_view.sz[1], GL_RGB, GL_UNSIGNED_BYTE, screenshot);
}
void GUI::screenshotSave() {
    if(screenshot == NULL) return;
    std::cout << "\nScreenshot: ";
    for(int j=0; j<50; j++)
        std::cout << (int)screenshot[j*100] << " ";
    std::string filename = std::string("screenshot-") + std::to_string(++screenshot_cnt) + std::string(".jpg");
    SaveJPEG(filename, pane_view.sz[0], pane_view.sz[1], screenshot);
    std::cout << "saved screenshot to " << filename << std::endl;
}
void GUI::screenshotIfShould(void) {
    if (take_screenshot) {
        screenshotSnap();
        take_screenshot = false;
    }
    if (save_screenshot) {
        screenshotSave();
        save_screenshot = false;
    }
}


void GUI::register_cbs(void) {
    // screenshot
    ioman.rcb_i(
        iom::IOReq {iom::kb_k('j'), iom::MODE::RELEASE},
        [&] TI_LAMBDA {
            take_screenshot = true;
            save_screenshot = true;
        }
    );
    // close window
    ioman.rcb_i(
        iom::IOReq {iom::kb_k(GLFW_KEY_ESCAPE), iom::MODE::PRESS},
        [&] TI_LAMBDA { std::thread([this](){ gdm::qd([&] GDM_OP {
            glfwSetWindowShouldClose(window_, GL_TRUE);
        }); }).detach(); }
    );
    // dollying in
    ioman.rcb_c(
        iom::IOReq {iom::kb_k('w'), iom::MODE::DOWN},
        [&] TD_LAMBDA {
            if (fps_mode_) eye_ += zoom_speed_ * look_ * float(dt.count());
            else camera_distance_ -= zoom_speed_ * float(dt.count());
        }
    );
    // dollying out
    ioman.rcb_c(
        iom::IOReq {iom::kb_k('s'), iom::MODE::DOWN},
        [&] TD_LAMBDA {
            if (fps_mode_) eye_ -= zoom_speed_ * look_ * float(dt.count());
            else camera_distance_ += zoom_speed_ * float(dt.count());
        }
    );
    // panning
    ioman.rcb_c(
        iom::IOReq {iom::kb_k('a') , iom::MODE::DOWN},
        [&] TD_LAMBDA {
            if (fps_mode_) eye_ -= pan_speed_ * tangent_ * float(dt.count());
            else center_ -= pan_speed_ * tangent_ * float(dt.count());
        }
    );
    ioman.rcb_c(
        iom::IOReq {iom::kb_k('d') , iom::MODE::DOWN},
        [&] TD_LAMBDA {
            if (fps_mode_) eye_ += pan_speed_ * tangent_ * float(dt.count());
            else center_ += pan_speed_ * tangent_ * float(dt.count());
        }
    );
    // panning down
    ioman.rcb_c(
        iom::IOReq {iom::kb_k(GLFW_KEY_DOWN) , iom::MODE::DOWN},
        [&] TD_LAMBDA {
            if (fps_mode_) eye_ -= pan_speed_ * up_ * float(dt.count());
            else center_ -= pan_speed_ * up_ * float(dt.count());
        }
    );
    // panning up
    ioman.rcb_c(
        iom::IOReq {iom::kb_k(GLFW_KEY_UP) , iom::MODE::DOWN},
        [&] TD_LAMBDA {
            if (fps_mode_) eye_ += pan_speed_ * up_ * float(dt.count());
            else center_ += pan_speed_ * up_ * float(dt.count());
        }
    );
    // drag camera
    static auto mmb = iom::mb_k(GLFW_MOUSE_BUTTON_MIDDLE, 0);
    static auto rmb = iom::mb_k(GLFW_MOUSE_BUTTON_RIGHT, 0);
    static auto lmb = iom::mb_k(GLFW_MOUSE_BUTTON_LEFT, 0);
    static auto to_vec2 = [](const iom::IOState& ios_p) {
        return glm::vec2(float(ios_p.x), float(ios_p.y));
    };
    static auto wmd = [=](const iom::IOView& view) {
        return to_vec2(view.pos_d.find(iom::mp())->second) * glm::vec2(1.0f, -1.0f);
    };
    static auto wmt = [=](const iom::IOView& view) {
        return (to_vec2(view.pos_t.find(iom::mp())->second) - glm::vec2(0.0f, pane_all.sz[1])) * glm::vec2(1.0f, -1.0f);
    };
    ioman.rcb_c(
        iom::IOReq {rmb, iom::MODE::DOWN},
        [&] TD_LAMBDA {
            if (view.pos_d.count(iom::mp())) {
                auto pos_d = wmd(view);
                pos_d = glm::vec2(pos_d[1], pos_d[0]);
                glm::vec3 axis = glm::normalize(orientation_ * glm::vec3(pos_d, 0.0f));
                orientation_ = glm::mat3(glm::rotate(rotation_speed_, axis) * glm::mat4(orientation_));
                tangent_ = glm::column(orientation_, 0);
                up_ = glm::column(orientation_, 1);
                look_ = glm::column(orientation_, 2);
            }
        }
    );
    // drag bone
    ioman.rcb_c(
        iom::IOReq {lmb, iom::MODE::DOWN},
        [&] TD_LAMBDA {
            if (view.pos_d.count(iom::mp()) && current_bone_ != -1) {
                auto pos_t = wmt(view);
                auto pos_d = wmd(view);
                glm::vec2 pos_l = pos_t - pos_d;
                // calculate angle
                glm::vec3 p_pos = mi->s_inst().ppos_c(current_bone_);
                glm::vec2 proj_p_pos = worldToNdcNear(glm::vec3(mi->mm() * glm::vec4(p_pos, 1.0f)));
                glm::vec2 v1 = glm::normalize(windowToNdc(pos_t) - proj_p_pos);
                glm::vec2 v2 = glm::normalize(windowToNdc(pos_l) - proj_p_pos);
                float angle = glm::acos(glm::clamp(glm::dot(v1, v2), -1.0f, 1.0f));
                if (v1[0] * v2[1] < v1[1] * v2[0]) angle *= -1.0f;
                // compute axis
                glm::vec3 axis = glm::vec3(mi->mminv() * glm::vec4(look_, 0.0f));
                // rotate bone
                mi->rotJoint(mi->s_inst().pid_c(current_bone_), glm::vec4(axis, angle));
                bmatrix_dirty = true;
                pose_changed_ = true;
            }
        }
    );
    // drag model
    ioman.rcb_c(
        iom::IOReq {mmb, iom::MODE::DOWN},
        [&] TD_LAMBDA {
            if (view.pos_d.count(iom::mp()) && current_bone_ != -1) {
                auto pos_t = wmt(view);
                auto pos_d = wmd(view);
                auto pos_l = pos_t - pos_d;
                glm::vec3 t_pos = mi->s_inst().ppos_c(current_bone_);
                t_pos = glm::vec3(mi->mminv() * glm::vec4(t_pos, 1.0f));
                glm::vec3 dv(windowToWorld(pos_t) - windowToWorld(pos_l));
                dv *= glm::abs(glm::dot(t_pos - eye_, look_)) / kNear;
                mi->offset(dv);
                bmatrix_dirty = true;
                pose_changed_ = true;
            }
        }
    );
    // if you don't have anything else to do, find the bone being looked at
    ioman.rcb_ci([&] SI_LAMBDA {
        static std::shared_ptr<bool> complete = std::make_shared<bool>(true);
        if (*complete) {
            if (!view.dks.count(mmb) && !view.dks.count(rmb) && !view.dks.count(lmb) && view.pos_t.count(iom::mp())) {
                auto pos_t = wmt(view);
                auto w_pos = windowToWorld(pos_t) - eye_;
                w_pos = glm::vec3(mi->mminv() * glm::vec4(w_pos, 0.0f));
                auto e_pos = glm::vec3(mi->mminv() * glm::vec4(eye_, 1.0));
                *complete = false;
                std::thread([=]() {
                    setCurrentBone(intersectBoneID(
                        mi->s_inst(),
                        w_pos, e_pos
                    ));
                    *complete = true;
                }).detach();
            }
        }
    });
    // roll
    static auto general_roll = [&](float roll_speed) {
        glm::vec3 j_pos = mi->s_inst().jpos_c(current_bone_);
        glm::vec3 p_pos = mi->s_inst().ppos_c(current_bone_);
        glm::vec3 axis = glm::normalize(p_pos - j_pos);
        mi->rotJoint(mi->s_inst().pid_c(current_bone_), glm::vec4(axis, roll_speed));
        bmatrix_dirty = true;
        pose_changed_ = true;
    };
    ioman.rcb_c(
        iom::IOReq {iom::kb_k(GLFW_KEY_LEFT), iom::MODE::DOWN},
        [&] TD_LAMBDA {general_roll(dt.count() * roll_speed_);}
    );
    ioman.rcb_c(
        iom::IOReq {iom::kb_k(GLFW_KEY_RIGHT), iom::MODE::DOWN},
        [&] TD_LAMBDA {general_roll(-dt.count() * roll_speed_);}
    );
    // toggle fps_mode
    ioman.rcb_i(
        iom::IOReq {iom::kb_k('c'), iom::MODE::RELEASE},
        [&] TI_LAMBDA {fps_mode_ = !fps_mode_;}
    );
    // fiddling around with bones
    ioman.rcb_i(
        iom::IOReq {iom::kb_k('['), iom::MODE::RELEASE},
        [&] TI_LAMBDA {
            --current_bone_;
            current_bone_ += mi->m->boneCnt();
            current_bone_ %= mi->m->boneCnt();
            bmatrix_dirty = true;
        }
    );
    ioman.rcb_i(
        iom::IOReq {iom::kb_k(']'), iom::MODE::RELEASE},
        [&] TI_LAMBDA {
            ++current_bone_;
            current_bone_ += mi->m->boneCnt();
            current_bone_ %= mi->m->boneCnt();
            bmatrix_dirty = true;
        }
    );
    // transparency toggle
    ioman.rcb_i(
        iom::IOReq {iom::kb_k('t'), iom::MODE::RELEASE},
        [&] TI_LAMBDA {
            transparent_ = !transparent_;
        }
    );
    // Key Frame Create
    ioman.rcb_i(
        iom::IOReq {iom::kb_k('f'), iom::MODE::RELEASE},
        [&] TI_LAMBDA {
            if (!isPlaying()) {
                mi->saveFrame();
            } // ignore any other attempts to save frames
        }
    );
    // Key Frame Create after current frame
    ioman.rcb_i(
        iom::IOReq {iom::kb_k('f', GLFW_MOD_SHIFT), iom::MODE::RELEASE},
        [&] TI_LAMBDA {
            if (!isPlaying()) {
                mi->saveFrame(anim_time.elapsed());
            } // ignore any other attempts to save frames
        }
    );
    // Play frames, or pause
    ioman.rcb_i(
        iom::IOReq {iom::kb_k('p'), iom::MODE::RELEASE},
        [&] TI_LAMBDA { anim_time.toggle(); }
    );
    // Rewind to start
    ioman.rcb_i(
        iom::IOReq {iom::kb_k('r'), iom::MODE::RELEASE},
        [&] TI_LAMBDA { anim_time.reset(); }
    );
    static float scroll_speed = 1.0 / 5.0;
    // FIXME: Key Frame Selection
    ioman.rcb_i(
        iom::IOReq {iom::mb_k(GLFW_MOUSE_BUTTON_LEFT), iom::MODE::RELEASE},
        [&] TI_LAMBDA {
            auto pos = wmt(view);
            if (pane_view.contains(pos)) return;
            int idx = 3.0f - pos[1] / pane_preview.sz[1] + pview_fshift_;
            mi->setFocusId(idx);
        }
    );
    // Updated currently selected keyframe
    ioman.rcb_i(
        iom::IOReq {iom::kb_k('u'), iom::MODE::RELEASE},
        [&] TI_LAMBDA { if (!isPlaying()) mi->updateFrame(mi->getFocusId()); }
    );
    // Delete selected keyframe
    ioman.rcb_i(
        iom::IOReq {iom::kb_k(GLFW_KEY_DELETE), iom::MODE::RELEASE},
        [&] TI_LAMBDA { if (!isPlaying()) mi->deleteFrame(mi->getFocusId()); }
    );
    // Replace current pose with selected pose
    ioman.rcb_i(
        iom::IOReq {iom::kb_k(GLFW_KEY_SPACE), iom::MODE::RELEASE},
        [&] TI_LAMBDA { if (!isPlaying()) mi->loadFrame(mi->getFocusId()); bmatrix_dirty = true; }
    );
    // FIXME: Save current animation to some file
    ioman.rcb_i(
        iom::IOReq {iom::kb_k('s', GLFW_MOD_CONTROL), iom::MODE::RELEASE},
        [&] TI_LAMBDA { std::thread([this](){ mi->saveAnimFile("animation.json"); }).detach(); }
    );
    // FIXME: Save current animation to some file with provided name
    ioman.rcb_i(
        iom::IOReq {iom::kb_k('s', GLFW_MOD_CONTROL | GLFW_MOD_SHIFT), iom::MODE::RELEASE},
        [&] TI_LAMBDA { std::thread([this](){ mi->saveAnimFile("animation.json"); }).detach(); }
    );
    // Mouse Scrolling
    ioman.rcb_c(
        iom::IOReq {iom::sp(), iom::MODE::RELEASE},
        [&] TD_LAMBDA {
            if (!view.pos_d.count(iom::sp()) || !view.pos_t.count(iom::mp()) || pane_view.contains(wmt(view))) return;
            auto pos_d = to_vec2(view.pos_d.find(iom::sp())->second);
            pview_fshift_ -= scroll_speed * pos_d[1];
            pview_fshift_ = glm::clamp(pview_fshift_, 0.0f, (float) mi->getPreviewFNum() - 0.5f);
        }
    );
    // FIXME: Page up, select lower preview id
    ioman.rcb_i(
        iom::IOReq {iom::kb_k(GLFW_KEY_PAGE_UP), iom::MODE::RELEASE},
        [&] TI_LAMBDA {
            mi->addFocusId(-1);
        }
    );
    // FIXME: Page down, select higher preview id
    ioman.rcb_i(
        iom::IOReq {iom::kb_k(GLFW_KEY_PAGE_DOWN), iom::MODE::RELEASE},
        [&] TI_LAMBDA {
            mi->addFocusId(1);
        }
    );
}

const glm::mat4& GUI::getPreviewOrthoSc() {
    if(mi->getPreviewFNum() == 0)
        ortho_sc_ = glm::ortho(0.0f, 0.0f, 0.0f, 0.0f);
    else
        ortho_sc_ = glm::ortho(-15.0f, 1.0f, 1.0f - ((float)mi->getPreviewFNum() + 0.5f), 1.0f);
    return ortho_sc_;
}


GUILite::GUILite(GLFWwindow* window) : win(window) {
    int win_h, win_w;
    glfwGetWindowSize(win, &win_w, &win_h);
    pane_win = Pane {{0.0f, 0.0f}, {win_w, win_h}};

    register_cbs();
}
GUILite::~GUILite() {}
void GUILite::startup(void) {
    ioman.activate(win);
}
void GUILite::shutdown(void) {
    ioman.deactivate(win);
}
void GUILite::register_cbs(void) {
    // close window
    ioman.rcb_i(
        iom::IOReq {iom::kb_k(GLFW_KEY_ESCAPE), iom::MODE::PRESS},
        [&] TI_LAMBDA { gdm::qd([&] GDM_OP {
            glfwSetWindowShouldClose(win, GL_TRUE);
        }); }
    );
    static auto move_frac = 10.0f;
    // dollying in
    ioman.rcb_c(
        iom::IOReq {iom::kb_k('w'), iom::MODE::DOWN},
        [&] TD_LAMBDA { cam.trans(glm::vec3(0.0f, 0.0f, float(dt.count() / move_frac))); }
    );
    // dollying out
    ioman.rcb_c(
        iom::IOReq {iom::kb_k('s'), iom::MODE::DOWN},
        [&] TD_LAMBDA { cam.trans(glm::vec3(0.0f, 0.0f, float(-dt.count() / move_frac))); }
    );
    // panning
    ioman.rcb_c(
        iom::IOReq {iom::kb_k('a') , iom::MODE::DOWN},
        [&] TD_LAMBDA { cam.trans(glm::vec3(float(dt.count() / move_frac), 0.0f, 0.0f)); }
    );
    ioman.rcb_c(
        iom::IOReq {iom::kb_k('d') , iom::MODE::DOWN},
        [&] TD_LAMBDA { cam.trans(glm::vec3(float(-dt.count() / move_frac), 0.0f, 0.0f)); }
    );
    // panning down
    ioman.rcb_c(
        iom::IOReq {iom::kb_k('e') , iom::MODE::DOWN},
        [&] TD_LAMBDA { cam.trans(glm::vec3(0.0f, float(dt.count() / move_frac), 0.0f)); }
    );
    // panning up
    ioman.rcb_c(
        iom::IOReq {iom::kb_k('q') , iom::MODE::DOWN},
        [&] TD_LAMBDA { cam.trans(glm::vec3(0.0f, float(-dt.count() / move_frac), 0.0f)); }
    );

    // enable gas model
    ioman.rcb_i(
        iom::IOReq {iom::kb_k('.', GLFW_MOD_CONTROL) , iom::MODE::RELEASE},
        [&] TI_LAMBDA {
            show_gas = !show_gas;
        }
    );
    // stop gas model
    ioman.rcb_i(
        iom::IOReq {iom::kb_k('/') , iom::MODE::RELEASE},
        [&] TI_LAMBDA {
            gas_dt = -1.0;
            std::cout << "set gas_dt= " << gas_dt << std::endl;
        }
    );

    static float dt_rate=0.000001;   
    // forward gas model
    ioman.rcb_i(
        iom::IOReq {iom::kb_k('.') , iom::MODE::RELEASE},
        [&] TI_LAMBDA {
            gas_dt += dt_rate;
            if(gas_dt < 0)
                gas_dt = -dt_rate;
            std::cout << "set gas_dt= " << gas_dt << std::endl;
        }
    );
    // slow down gas model
    ioman.rcb_i(
        iom::IOReq {iom::kb_k(',') , iom::MODE::RELEASE},
        [&] TI_LAMBDA {
            gas_dt -= dt_rate;
            if(gas_dt < 0)
                gas_dt = 0.0;
            std::cout << "set gas_dt= " << gas_dt << std::endl;
        }
    );
    // camera
    static auto lmb = iom::mb_k(GLFW_MOUSE_BUTTON_LEFT, 0);
    static auto to_vec2 = [](const iom::IOState& ios_p) {
        return glm::vec2(float(ios_p.x), float(ios_p.y));
    };
    static auto wmd = [=](const iom::IOView& view) {
        return to_vec2(view.pos_d.find(iom::mp())->second) * glm::vec2(1.0f, -1.0f);
    };
    static auto wmt = [=](const iom::IOView& view) {
        return (to_vec2(view.pos_t.find(iom::mp())->second) - glm::vec2(0.0f, pane_win.sz[1])) * glm::vec2(1.0f, -1.0f);
    };
    static bool ignore_jump = false;
    ioman.rcb_c(
        iom::IOReq {},
        [&] TD_LAMBDA {
            if (!ignore_jump) {
                ignore_jump = true;
                return;
            }
            auto pos_d = wmd(view) / pane_win.sz / glm::vec2(2.0);
            cam.rot(-pos_d[1], pos_d[0], 0.0f);
        }
    );
}
const Camera& GUILite::g_cam(void) const {
    return cam;
}
