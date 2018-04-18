#include "config.h"
#include "anim_mesh.h"
#include "texture_to_render.h"
#include <fstream>
#include <queue>
#include <iostream>
#include <stdexcept>
#include <glm/gtx/io.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtx/quaternion.hpp>

namespace {
    /*
     * For debugging purpose.
     */
    template <typename T>
    std::ostream& operator<<(std::ostream& os, const std::vector<T>& v) {
        size_t count = std::min(v.size(), static_cast<size_t>(10));
        for (size_t i = 0; i < count; ++i) os << i << " " << v[i] << "\n";
        os << "size = " << v.size() << "\n";
        return os;
    }

    std::ostream& operator<<(std::ostream& os, const BoundingBox& bounds) {
        os << "min = " << bounds.min << " max = " << bounds.max;
        return os;
    }

    glm::vec3 quat_to_v3(glm::fquat q) { return glm::vec3(q.x, q.y, q.z); }
    glm::vec3 q_rot(glm::fquat q, glm::vec3 v) { return v + 2.0f * glm::cross(glm::cross(v, quat_to_v3(q)) - q.w*v, quat_to_v3(q)); }
#define DIFF 1e-5
    glm::fquat cnorm(glm::fquat q) { return glm::normalize(q); }
}


Frame::Frame() : model_trans(), rori(), root_tran() {}
Frame::Frame(const Frame& other) : model_trans(), rori(), root_tran() {
    model_trans = other.model_trans;
    rori = other.rori;
    root_tran = other.root_tran;
}
Frame::Frame(Frame&& other) : model_trans(), rori(), root_tran() {
    std::swap(model_trans, other.model_trans);
    rori.swap(other.rori);
    root_tran.swap(other.root_tran);
}
Frame& Frame::operator=(const Frame& other) {
    model_trans = other.model_trans;
    rori = other.rori;
    root_tran = other.root_tran;
    return *this;
}
Frame& Frame::operator=(Frame&& other) {
    std::swap(model_trans, other.model_trans);
    rori.swap(other.rori);
    root_tran.swap(other.root_tran);
    return *this;
}
void KeyFrame::sl_l_erp(
    const KeyFrame& from, const KeyFrame& to,
    std::chrono::duration<double> t, Frame& target
) {
    float tau = float((t - from.t).count()) / float((to.t - from.t).count());
    target.rori.resize(from.rori.size());
    for (size_t i = 0; i < from.rori.size(); ++i) {
        target.rori[i] = glm::slerp(from.rori[i], to.rori[i], tau);
    }
    for (auto pit : from.root_tran) {
        target.root_tran[pit.first] = glm::mix(pit.second, to.root_tran.find(pit.first)->second, tau);
    }
}


const Joint& Skeleton::rootOf(Joint j) const {
    while (!j.isRoot()) j = joints[j.pid];
    return this->j(j.jid);
}
int Skeleton::rootOf(int jid) const {
    return rootOf(j(jid)).jid;
}



Mesh::Mesh() {
    skeleton = std::make_shared<Skeleton>();
}

Mesh::~Mesh() {}

void Mesh::loadPmd(const std::string& fn) {
    MMDReader mr;
    mr.open(fn);
    mr.getMesh(vertices, faces, vertex_normals, uv_coordinates);
    computeBounds();
    mr.getMaterial(materials);

    { // joints
        glm::vec3 position; int parent; int curr = 0;
        while (mr.getJoint(curr++, position, parent)) skeleton->joints.push_back(Joint {curr - 1, position, parent});
        for (auto joint : skeleton->joints) {
            // finish setting up parent joint
            if (joint.pid == -1) {
                skeleton->roots.insert(joint.jid);
                skeleton->f_default.root_tran[joint.jid] = glm::vec3();
            }
            else skeleton->joints[joint.pid].children.push_back(joint.jid);
            // setup initial uniforms
            skeleton->q_default.tran.push_back(joint.init_pos);
            skeleton->q_default.rota.push_back(glm::fquat(1.0f, 0.0f, 0.0f, 0.0f));
            skeleton->f_default.rori.push_back(glm::fquat(1.0f, 0.0f, 0.0f, 0.0f));
        }
    }
    { // vertices
        std::vector<SparseTuple> weights;
        mr.getJointWeights(weights);
        for (auto wd : weights) { // extract vertex info into uniforms
            jids0.push_back(wd.jid0);
            jids1.push_back(wd.jid1);
            jweight0.push_back(wd.weight0);
            // Offset from joint to point in initial model space, remains consistent after rotations
            joff0.push_back(glm::vec3(vertices[wd.vid]) - skeleton->joints[wd.jid0].init_pos);
            joff1.push_back(glm::vec3(vertices[wd.vid]) - skeleton->joints[wd.jid1].init_pos);
        }
    }
}
void Mesh::computeBounds() {
    bounds.min = glm::vec3(std::numeric_limits<float>::max());
    bounds.max = glm::vec3(-std::numeric_limits<float>::max());
    for (const auto& vert : vertices) {
        bounds.min = glm::min(glm::vec3(vert), bounds.min);
        bounds.max = glm::max(glm::vec3(vert), bounds.max);
    }
}


SkelI::SkelI(const std::shared_ptr<Skeleton>& skel) : skel(skel) {
    qbuf.rota.resize(skel->joints.size());
    qbuf.tran.resize(skel->joints.size());
    ro_state.rori.resize(skel->joints.size());
    for (size_t i = 0; i < skel->joints.size(); i++) {
        qbuf.rota[i] = skel->q_default.rota[i];
        qbuf.tran[i] = skel->q_default.tran[i];
        ro_state.rori[i] = skel->f_default.rori[i];
    }
    for (auto root_tran : skel->f_default.root_tran) {
        ro_state.root_tran[root_tran.first] = root_tran.second;
    }
}
void SkelI::reletavize(const int& jid, const glm::fquat& rot) {
    cache_refresh = true;
    // rotate j.rel_ori with change directly
    if (j(jid).isRoot()) {
        jrot(jid) = cnorm(glm::cross(rot, jrot(jid)));
        jro(jid) = jrot(jid);
    } else {
        auto shift = glm::cross(glm::conjugate(prot(jid)), glm::cross(rot, prot(jid)));
        jro(jid) = glm::cross(shift, jro(jid));
        jrot(jid) = cnorm(glm::cross(prot(jid), jro(jid)));
    }
    for (int cid : jc(jid)) recalc(cid);
}
void SkelI::recalc(const int& jid) {
    cache_refresh = true;
    if (j(jid).isRoot()) {
        jrot(jid) = jro(jid);
        jpos(jid) = jtran(jid) + jipos(jid);
    } else {
        jrot(jid) = cnorm(glm::cross(prot(jid), jro(jid)));
        jpos(jid) = ppos(jid) + q_rot(prot(jid), (jipos(jid) - pipos(jid)));
    }
    for (int cid : jc(jid)) recalc(cid);
}
void SkelI::offset(const glm::vec3& off) {
    cache_refresh = true;
    jtran(1) += off;
    for (auto jid : skel->roots) recalc(jid); // update
}
void SkelI::reset(const Frame& rof) {
    ro_state = rof; // copy
    for (auto jid : skel->roots) recalc(jid); // update
    cache_refresh = true;
}
void SkelI::refreshCache(void) {
    if (cache_refresh) {
        q_cache.rota.resize(skel->joints.size());
        q_cache.tran.resize(skel->joints.size());
        for (size_t i = 0; i < skel->joints.size(); i++) {
            q_cache.rota[i] = jrot(i);
            q_cache.tran[i] = jpos(i);
        }
    }
    cache_refresh = false;
}
KeyFrame SkelI::toKeyFrame(const std::chrono::duration<double>& t) const {
    KeyFrame kf;
    kf.rori.resize(ro_state.rori.size());
    for (size_t i = 0; i < ro_state.rori.size(); ++i) {
        kf.rori[i] = ro_state.rori[i];
    }
    for (auto pit : ro_state.root_tran) {
        kf.root_tran[pit.first] = pit.second;
    }
    kf.t = t;
    kf.ttr = new TextureToRender();
    return kf;
}



MeshI::MeshI(std::shared_ptr<Mesh>& mesh) : m(mesh), s(mesh->skeleton) {}
void MeshI::offset(const glm::vec3& off) {
    s.offset(off);
}
void MeshI::rotJoint(int jid, const glm::vec4& rot) { // model space axis angle, wraps quaternion
    auto rot_axis = glm::normalize(glm::vec3(rot));
    float hasin = glm::sin(rot[3] / 2);
    s.reletavize(jid, glm::normalize(glm::fquat(1 - hasin * hasin, rot_axis * hasin)));
}
std::list<KeyFrame>::iterator MeshI::frameAtOrAfter(const std::chrono::duration<double>& t) {
    auto it = key_frames.begin();
    while (it != key_frames.end() && it->t < t) ++it;
    return it;
}
std::list<KeyFrame>::iterator MeshI::frameAfter(const std::chrono::duration<double>& t) {
    auto it = key_frames.begin();
    while (it != key_frames.end() && it->t <= t) ++it;
    return it;
}
std::list<KeyFrame>::iterator MeshI::frameAt(const int& fid) {
    int to_frame = fid;
    auto it = key_frames.begin();
    while (it != key_frames.end() && to_frame-- > 0) ++it;
    return it;
}
std::list<KeyFrame>::const_iterator MeshI::frameAtOrAfter(const std::chrono::duration<double>& t) const {
    auto it = key_frames.begin();
    while (it != key_frames.end() && it->t < t) ++it;
    return it;
}
std::list<KeyFrame>::const_iterator MeshI::frameAfter(const std::chrono::duration<double>& t) const {
    auto it = key_frames.begin();
    while (it != key_frames.end() && it->t <= t) ++it;
    return it;
}
std::list<KeyFrame>::const_iterator MeshI::frameAt(const int& fid) const {
    int to_frame = fid;
    auto it = key_frames.begin();
    while (it != key_frames.end() && to_frame-- > 0) ++it;
    return it;
}

void MeshI::saveFrame(const std::chrono::duration<double>& t) {
    if (t < std::chrono::duration<double>::zero()) {
        std::chrono::duration<double> new_t = t;
        if (key_frames.empty()) {
            new_t = std::chrono::duration<double>::zero();
        } else {
            new_t = key_frames.back().t + std::chrono::duration<double>(1);
        }
        key_frames.push_back(s.toKeyFrame(new_t));
        preview_to_render_ = frameAt(key_frames.size() - 1);
    } else {
        auto it = frameAfter(t);
        key_frames.insert(it, s.toKeyFrame(t));
        preview_to_render_ = --it; // TODO: correct index??
    }
}
void MeshI::moveFrame(const int& fid, const std::chrono::duration<double>& t) {
    auto it = frameAt(fid);
    if (it == key_frames.end()) return;
    auto re_it = frameAtOrAfter(t);
    key_frames.splice(it, key_frames, re_it);
    it->t = t;
    if (re_it != key_frames.end() && re_it->t == t) {
        key_frames.erase(re_it); // remove old frame if is at same time
    }
}
void MeshI::updateFrame(const int& fid) {
    auto it = frameAt(fid);
    if (it != key_frames.end()) {
        *it = s.toKeyFrame(it->t);
        preview_to_render_ = it;
    }
}
void MeshI::deleteFrame(const int& fid) {
    auto it = frameAt(fid);
    if (it != key_frames.end()) {
        it->ttr->~TextureToRender();
        key_frames.erase(it);
    }
}
void MeshI::loadFrame(const int& fid) {
    auto it = frameAt(fid);
    if (it != key_frames.end()) {
        Frame copy(*it);
        s.reset(copy);
        s.refreshCache();
    }
}
void MeshI::updateAnim(const std::chrono::duration<double>& t) {
    if (!key_frames.empty() && t >= std::chrono::duration<double>::zero()) {
        Frame current;
        auto f_it = frameAtOrAfter(t);
        if (f_it == key_frames.begin()) { // no previous, just use the first
            current = *f_it;
        } else if (f_it == key_frames.end()) { // no interpolation
            current = key_frames.back();
        } else {
            // take previous and interp
            auto p_it = f_it--;
            KeyFrame::sl_l_erp(*p_it, *f_it, t, current);
        }
        s.reset(current);
    }
    s.refreshCache(); // always refresh the cache
}


bool MeshI::bindTextureIfAny(int width, int height) {
    if(preview_to_render_ != key_frames.end()) {
        // std::cout << "*** bind to render texture" << std::endl;
        preview_to_render_->ttr->createNoWrap(width, height);
        preview_to_render_->ttr->bind();
        preview_to_render_->ttr->report();
        // std::cout << "*** ready to render" << std::endl;
        return true;
    }
    return false;
}
bool MeshI::unbindTextureIfAny() {
    if(preview_to_render_ != key_frames.end()) {
        // std::cout << "*** unbind to render texture" << std::endl;
        preview_to_render_->ttr->unbind();
        preview_to_render_->ttr->report();
        preview_to_render_ = key_frames.end();
        // std::cout << "*** finished unbind, ready to render texture" << std::endl;
        return true;
    }
    return false;
}
void MeshI::setBindId(int id) {
    if(id >= int(key_frames.size())) return;
    pview_bind_id_ = id;
    pview_bind_ = frameAt(id);
    pview_border_ = (pview_bind_id_ == pview_focus_id_) ? 1 : 0;
    pview_ortho_matrix_ = glm::ortho(0.0f, 1.0f, -2.0f + id, 1.0f + id);
}
void MeshI::setFocusId(int id) {
    if(id >= int(key_frames.size())) return;
    pview_focus_id_ = id;
    pview_focus_ = frameAt(id);
}
void MeshI::addFocusId(int val) {
    pview_focus_id_ = glm::clamp(pview_focus_id_ + val, 0, int(key_frames.size() - 1));
    pview_focus_ = frameAt(pview_focus_id_);
}
const unsigned int MeshI::getPreviewTextureId() const {
    return pview_bind_->ttr->getTexture();
}
