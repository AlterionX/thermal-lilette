#ifndef __ANIM_MESH_H__
#define __ANIM_MESH_H__

#include <ostream>
#include <vector>
#include <string>
#include <map>
#include <chrono>
#include <list>
#include <unordered_set>
#include <unordered_map>
#include <limits>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <mmdadapter.h>

#include <iostream>

class TextureToRender;

struct BoundingBox {
    BoundingBox()
        : min(glm::vec3(-std::numeric_limits<float>::max())),
        max(glm::vec3(std::numeric_limits<float>::max())) {}
    glm::vec3 min;
    glm::vec3 max;
};

struct LineMesh {
    std::vector<glm::vec4> vertices;
    std::vector<glm::uvec2> indices;
};

struct SimpleMesh {
    std::vector<glm::vec4> vertices;
    std::vector<glm::vec2> tex_coords;
    std::vector<glm::uvec3> indices;
};

struct Configuration {
    std::vector<glm::vec3> tran;
    std::vector<glm::fquat> rota;

    const void* transData() const { return tran.data(); }
    const void* rotData() const { return rota.data(); }
};

struct Frame {
    glm::vec3 model_trans;
    std::vector<glm::fquat> rori; // relative orientation
    std::unordered_map<int, glm::vec3> root_tran;

    Frame();
    Frame(const Frame& other);
    Frame(Frame&& other);
    Frame& operator=(const Frame& other);
    Frame& operator=(Frame&& other);
};
struct KeyFrame : public Frame {
    std::chrono::duration<double> t; // timestamp of this frame
    TextureToRender* ttr;
    // slerp rotations and piecewise lerp movement
    static void sl_l_erp(
        const KeyFrame& from, const KeyFrame& to,
        std::chrono::duration<double> tau, Frame& target
    );
};

struct Joint {
    Joint() : jid(-1), pid(-1), init_pos(glm::vec3(0.0f)) {}
    Joint(int id, glm::vec3 wcoord, int parent) : jid(id), pid(parent), init_pos(wcoord) {}

    int jid; int pid;
    glm::vec3 init_pos; // initial in model space
    std::vector<int> children;

    bool isRoot(void) const { return pid < 0; }
};

struct Skeleton {
    std::unordered_set<int> roots;
    std::vector<Joint> joints;

    Frame f_default;
    Configuration q_default;

    const Joint& rootOf(Joint j) const;
    int rootOf(int jid) const;

    const Joint& j(int jid) const { return joints[jid]; }
    int pid(int jid) const { return joints[jid].pid; }
    const glm::vec3& jipos(int jid) const { return j(jid).init_pos; }
    const std::vector<int>& jc(int jid) const { return j(jid).children; }
    size_t jcnt(void) const { return joints.size(); }
};

struct Mesh {
    Mesh();
    ~Mesh();
    /*** Uniforms for opengl ***/
    // geometry data
    std::vector<glm::vec4> vertices;
    std::vector<glm::vec4> vertex_normals;
    std::vector<glm::vec4> face_normals;
    std::vector<glm::uvec3> faces;
    // rigging data
    std::shared_ptr<Skeleton> skeleton;
    std::vector<int32_t> jids0;
    std::vector<int32_t> jids1;
    std::vector<float> jweight0; // jweight1 = 1 - jweight0
    std::vector<glm::vec3> joff0;
    std::vector<glm::vec3> joff1;
    // texturing data
    std::vector<glm::vec2> uv_coordinates;
    std::vector<Material> materials;

    BoundingBox bounds;

    void loadPmd(const std::string& fn);
    int boneCnt(void) const { return skeleton->joints.size(); }
    glm::vec3 getCenter(void) const { return 0.5f * glm::vec3(bounds.min + bounds.max); }
private:
    void computeBounds(void);
    void computeNormals(void);
};

class SkelI {
    Frame ro_state;
    Configuration qbuf;
    bool cache_refresh = true;

public:
    std::shared_ptr<Skeleton> skel;
    Configuration q_cache;

    SkelI(const std::shared_ptr<Skeleton>& skel);

    // Frame manipulation and recalculation
    void offset(const glm::vec3& off);
    void reset(const Frame& rof);
    void reletavize(const int& jid, const glm::fquat& rot);
    void recalc(const int& jid);
    KeyFrame toKeyFrame(const std::chrono::duration<double>& t) const;

    const Configuration* getCQ(void) { return &q_cache; }
    void refreshCache(void);
    const glm::vec3* collectJointTrans(void) const { return q_cache.tran.data(); }
    const glm::fquat* collectJointRot(void) const { return q_cache.rota.data(); }
private:
    // Delegated to underlying Skeleton
    const Joint& j(const int& jid) const { return skel->j(jid); }
    int pid(const int& jid) const { return skel->pid(jid); }
    const Joint& p(const int& jid) const { return skel->j(pid(jid)); }
    const glm::vec3& jipos(const int& jid) const { return skel->jipos(jid); }
    const glm::vec3& pipos(const int& jid) const { return skel->jipos(pid(jid)); }
    const std::vector<int>& jc(const int& jid) const { return skel->jc(jid); }
    size_t jcnt(void) const { return skel->jcnt(); }
    // Frame data
    glm::vec3& jtran(const int& jid) { return ro_state.root_tran[skel->rootOf(jid)]; }
    glm::fquat& jro(const int& jid) { return ro_state.rori[jid]; }
    glm::fquat& pro(const int& jid) { return ro_state.rori[pid(jid)]; }

    // Resulting data from calcualting Frame
    glm::vec3& jpos(const int& jid) { return qbuf.tran[jid]; }
    glm::vec3& ppos(const int& jid) { return qbuf.tran[pid(jid)]; }
    glm::fquat& jrot(const int& jid) { return qbuf.rota[jid]; }
    glm::fquat& prot(const int& jid) { return qbuf.rota[pid(jid)]; }
public: // const public copies of easy accessors
    // Delegated to underlying Skeleton
    const Joint& j_c(const int& jid) const { return skel->j(jid); }
    int pid_c(const int& jid) const { return skel->pid(jid); }
    const Joint& p_C(const int& jid) const { return skel->j(pid(jid)); }
    const glm::vec3& jipos_c(const int& jid) const { return skel->jipos(jid); }
    const glm::vec3& pipos_c(const int& jid) const { return skel->jipos(pid(jid)); }
    const std::vector<int>& jc_c(const int& jid) const { return skel->jc(jid); }
    size_t jcnt_c(void) const { return skel->jcnt(); }
    // Frame data
    const glm::vec3& jtran_c(const int& jid) const { return ro_state.root_tran.find(skel->rootOf(jid))->second; }
    const glm::fquat& jro_c(const int& jid) const { return ro_state.rori[jid]; }
    const glm::fquat& pro_c(const int& jid) const { return ro_state.rori[pid(jid)]; }
    // Resulti&ng data from calcualting Frame
    const glm::vec3& jpos_c(const int& jid) const { return qbuf.tran[jid]; }
    const glm::vec3& ppos_c(const int& jid) const { return qbuf.tran[pid(jid)]; }
    const glm::fquat& jrot_c(const int& jid) const { return qbuf.rota[jid]; }
    const glm::fquat& prot_c(const int& jid) const { return qbuf.rota[pid(jid)]; }
};

class MeshI {
    glm::mat4 mmat;
    glm::mat4 mmatinv;

public:
    MeshI(std::shared_ptr<Mesh>& mesh);

    // Access to areas of the model
    std::shared_ptr<Mesh> m;
    SkelI s;
    SkelI& s_inst(void) { return s; }

    // Base model
    const glm::mat4& mm(void) const { return mmat; }
    const glm::mat4& mminv(void) const { return mmatinv; }
    void offset(const glm::vec3& off);

    // Skeleton manipulation
    void rotJoint(int jid, const glm::vec4& rot); // axis angle, convert to quat and wraps recalc
    void setRelOris(const Frame& rof) { s.reset(rof); }
    const Configuration* getCQ() { return s.getCQ(); } // Configuration is abbreviated as Q

    // animation frames
private:
    SimpleMesh preview_mesh;
    std::list<KeyFrame> key_frames; // always sorted, no two should have the same time
    std::list<KeyFrame>::iterator preview_to_render_ = key_frames.end();
    std::list<KeyFrame>::iterator pview_focus_ = key_frames.end();
    std::list<KeyFrame>::iterator pview_bind_ = key_frames.end();
    int pview_focus_id_ = -1, pview_bind_id_ = -1;
    int pview_border_ = 0;
    glm::mat4 pview_ortho_matrix_ = glm::mat4(1.0f);
    std::vector<TextureToRender*> previews;

    // std::list<KeyFrame>::iterator c_frame; // this should be the last key frame before this frame
    std::list<KeyFrame>::iterator frameAtOrAfter(const std::chrono::duration<double>& t);
    std::list<KeyFrame>::iterator frameAfter(const std::chrono::duration<double>& t);
    std::list<KeyFrame>::iterator frameAt(const int& fid);
    std::list<KeyFrame>::iterator frameAfter(const int& fid) { return frameAt(fid + 1); }

    std::list<KeyFrame>::const_iterator frameAtOrAfter(const std::chrono::duration<double>& t) const;
    std::list<KeyFrame>::const_iterator frameAfter(const std::chrono::duration<double>& t) const;
    std::list<KeyFrame>::const_iterator frameAt(const int& fid) const;
    std::list<KeyFrame>::const_iterator frameAfter(const int& fid) const { return frameAt(fid + 1); }
public:

    const KeyFrame& getFrameAtOrAfter(const std::chrono::duration<double>& t) const { return *frameAtOrAfter(t); }
    const KeyFrame& getFrameAfter(const std::chrono::duration<double>& t) const { return *frameAfter(t); }
    const KeyFrame& getFrameAt(const int& fid) const { return *frameAt(fid); }
    const KeyFrame& getFrameAfter(const int& fid) const { return *frameAfter(fid + 1); }

    void saveFrame(
        const std::chrono::duration<double>& t = std::chrono::duration<double>(-1)
    ); // insert at time t, or 1 second after the last frame if t < 0
    void moveFrame(const int& fid, const std::chrono::duration<double>& t); // shift frame fid to time t
    void updateFrame(const int& fid); // replace frame fid with current
    void deleteFrame(const int& fid); // delete pointed to frame
    void loadFrame(const int& fid); // load frame fid to current model

    void updateAnim(const std::chrono::duration<double>& t = std::chrono::duration<double>(-1)); // interpolate to time t

    // File IO
    void saveAnimFile(const std::string& fn);
    void loadAnimFile(const std::string& fn);

    // Preview
    bool bindTextureIfAny(int width, int height);
    bool unbindTextureIfAny();
    void setBindId(int id);
    void setFocusId(int id);
    int getFocusId(void) const { return pview_focus_id_; }
    void addFocusId(int val);
    const unsigned int getPreviewTextureId() const;
    const int getPreviewFNum() const { return key_frames.size(); }
    const int* getPreviewBorder() const { return &pview_border_; }
    const glm::mat4& getPreviewOrtho() const { return pview_ortho_matrix_; }
    SimpleMesh* getPreviewMesh() { return &preview_mesh; }
};

#endif
