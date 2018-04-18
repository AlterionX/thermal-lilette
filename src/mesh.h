#pragma once

#ifndef __MESH_H__
#define __MESH_H__

#include <memory>
#include <vector>

#include <material.h>

#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/quaternion.hpp>

namespace geom {
    template<typename face_t, typename matl_t, typename texc_t, typename vert_t, typename norm_t>
    class DOMesh {// Basic data
        // Some basic definitions
        typedef std::shared_ptr<matl_t> mptr_t;

        // Geometry data
        std::shared_ptr<std::vector<vert_t>> verts;
        std::shared_ptr<std::vector<face_t>> faces;
        // Texture data
        std::shared_ptr<std::vector<norm_t>> norms;
        std::shared_ptr<std::vector<texc_t>> texcs;
        std::shared_ptr<std::vector<mptr_t>> matls;

        // For moving
        void swap(DOMesh&& other) {
            this->verts.swap(other.verts);
            this->faces.swap(other.faces);
            this->norms.swap(other.norms);
            this->texcs.swap(other.texcs);
            this->matls.swap(other.matls);
        }
        void copy(const DOMesh& other) {
            this->verts = other.verts;
            this->faces = other.faces;
            this->norms = other.norms;
            this->texcs = other.texcs;
            this->matls = other.matls;
        }
    public:
        // Constructors, destuctor, and eq operators
        DOMesh() {}
        DOMesh(
            const std::shared_ptr<std::vector<vert_t>>& verts,
            const std::shared_ptr<std::vector<face_t>>& faces,
            const std::shared_ptr<std::vector<norm_t>>& norms,
            const std::shared_ptr<std::vector<texc_t>>& texcs,
            const std::shared_ptr<std::vector<mptr_t>>& matls
        ) : verts(verts), faces(faces), norms(norms), texcs(texcs), matls(matls) {}
        DOMesh(
            std::shared_ptr<std::vector<vert_t>>&& verts,
            std::shared_ptr<std::vector<face_t>>&& faces,
            std::shared_ptr<std::vector<norm_t>>&& norms,
            std::shared_ptr<std::vector<texc_t>>&& texcs,
            std::shared_ptr<std::vector<mptr_t>>&& matls
        ) : verts(verts), faces(faces), norms(norms), texcs(texcs), matls(matls) {}

        DOMesh(const DOMesh& other){
            this->copy(other);
        }
        DOMesh& operator=(const DOMesh& other) {
            this->copy(other);
            return *this;
        }
        DOMesh(DOMesh&& other) {
            this->swap(std::move(other));
        }
        DOMesh& operator=(DOMesh&& other) {
            this->swap(std::move(other));
            return *this;
        }

        ~DOMesh() {}

        const std::vector<vert_t>& g_verts(void) const { return *verts; }
        const std::vector<face_t>& g_faces(void) const { return *faces; }
        const std::vector<norm_t>& g_norms(void) const { return *norms; }
        const std::vector<texc_t>& g_texcs(void) const { return *texcs; }
        const std::vector<mptr_t>& g_matls(void) const { return *matls; }

        void r_verts(const std::shared_ptr<std::vector<vert_t>> renew) { verts = renew; }
        void r_faces(const std::shared_ptr<std::vector<face_t>> renew) { faces = renew; }
        void r_norms(const std::shared_ptr<std::vector<norm_t>> renew) { norms = renew; }
        void r_texcs(const std::shared_ptr<std::vector<texc_t>> renew) { texcs = renew; }
        void r_matls(const std::shared_ptr<std::vector<mptr_t>> renew) { matls = renew; }

        /*void calc_norms(void) {
            std::vector<std::vector<norm_t>> data;
            data.resize(verts.size());
            for (face_t face : faces) {
                std::vector<norm_t> c_norms = face.calc_norm();
                for (size_t i = 0; i < face.size(); ++i) {
                    data[i].push_back(c_norms[i]);
                }
            }
            for (size_t i = 0; i < verts.size(); i++) {
                norm_t sum;
                for (norm_t c_norm : data[i]) {
                    sum += c_norm;
                }
                sum *= float(1.0f / data[i].size());
            }
        }*/
    };

    template<typename face_t, typename matl_t, typename texc_t, typename vert_t, typename norm_t>
    class Mesh : private DOMesh<face_t, matl_t, texc_t, vert_t, norm_t> {// Basic data
        typedef std::shared_ptr<matl_t> mptr_t;
        typedef DOMesh<face_t, matl_t, texc_t, vert_t, norm_t> super; // Basic inheritance

    public:
        // Constructors, destuctor, and eq operators
        Mesh() : super(
            std::make_shared<std::vector<vert_t>>(0),
            std::make_shared<std::vector<face_t>>(0),
            std::make_shared<std::vector<norm_t>>(0),
            std::make_shared<std::vector<texc_t>>(0),
            std::make_shared<std::vector<mptr_t>>(0)
        ){}
        Mesh(
            const std::vector<vert_t>& verts,
            const std::vector<face_t>& faces,
            const std::vector<norm_t>& norms,
            const std::vector<texc_t>& texcs,
            const std::vector<mptr_t>& matls
        ) : super(
            std::move(std::make_shared<std::vector<vert_t>>(verts)),
            std::move(std::make_shared<std::vector<face_t>>(faces)),
            std::move(std::make_shared<std::vector<norm_t>>(norms)),
            std::move(std::make_shared<std::vector<texc_t>>(texcs)),
            std::move(std::make_shared<std::vector<mptr_t>>(matls))
        ){}

        Mesh(const Mesh&) = delete;
        Mesh& operator=(const Mesh&) = delete;

        Mesh(Mesh&& other) {
            this->super::swap(std::move(other));
        }
        Mesh& operator=(Mesh&& other) {
            this->super::swap(std::move(other));
            return *this;
        }

        ~Mesh() {}

        const std::vector<vert_t>& g_verts(void) const { return super::g_verts(); }
        const std::vector<face_t>& g_faces(void) const { return super::g_faces(); }
        const std::vector<norm_t>& g_norms(void) const { return super::g_norms(); }
        const std::vector<texc_t>& g_texcs(void) const { return super::g_texcs(); }
        const std::vector<mptr_t>& g_matls(void) const { return super::g_matls(); }
    };
    // instancing of meshes
    template<typename face_t, typename matl_t, typename texc_t, typename vert_t, typename norm_t, typename mesh_t>
    class MeshI {
        std::shared_ptr<mesh_t> mptr;

        // positioning
        glm::vec3 trans;
        glm::fquat rot;
        glm::vec3 scale;

        // cached values for opengl
        bool regen_mm;
        glm::mat4 mm_forw;
        glm::mat4 mm_back;

        void clone(const MeshI& other) {
            this->trans = other.trans;
            this->rot = other.rot;
            this->scale = other.scale;
            this->regen_mm = true;
        }
        void swap(MeshI&& other) {
            std::swap(this->trans, other.trans);
            std::swap(this->rot, other.rot);
            std::swap(this->scale, other.scale);
            this->regen_mm = true;
        }
        void regen(void) {
            mm_forw = glm::translate(glm::mat4_cast(rot) * glm::scale(scale), trans);
            mm_back = glm::inverse(mm_forw);
        }
    public:
        MeshI() = delete;
        MeshI(const std::shared_ptr<mesh_t>& sptr) : mptr(sptr), trans(), rot(1.0f, 0.0f, 0.0f, 0.0f), scale(1.0f) {}
        MeshI(const std::shared_ptr<mesh_t>& sptr, glm::vec3 trans, glm::fquat rot, glm::vec3 scale) : mptr(sptr), trans(trans), rot(rot), scale(scale), regen_mm(true) {}

        MeshI(const MeshI& other) { this->clone(other); }
        MeshI& operator=(const MeshI& other) {
            this->clone(other);
            return *this;
        }

        MeshI(MeshI&& other) { this->swap(std::move(other)); }
        MeshI& operator=(MeshI&& other) {
            this->swap(std::move(other));
            return *this;
        }

        ~MeshI() {}

        std::shared_ptr<mesh_t> m() { return mptr; }
        const glm::mat4& m_mat(void) {
            if (regen_mm) regen();
            return mm_forw;
        }
        const glm::mat4& m_inv(void) {
            if (regen_mm) regen();
            return mm_back;
        }
    };

    typedef glm::uvec2 face_l_t;
    typedef glm::uvec3 face_t_t;
    typedef glm::uvec4 face_q_t;

    typedef DOMesh  <face_l_t, Material, glm::vec3, glm::vec4, glm::vec4> LDOMesh;
    typedef DOMesh  <face_t_t, Material, glm::vec2, glm::vec4, glm::vec4> TDOMesh;
    typedef DOMesh  <face_q_t, Material, glm::vec2, glm::vec4, glm::vec4> QDOMesh;
    typedef Mesh    <face_t_t, Material, glm::vec2, glm::vec4, glm::vec4> TMesh;
    typedef Mesh    <face_l_t, Material, glm::vec2, glm::vec4, glm::vec4> LMesh;
    typedef Mesh    <face_q_t, Material, glm::vec2, glm::vec4, glm::vec4> QMesh;

    typedef MeshI   <face_l_t, Material, glm::vec2, glm::vec4, glm::vec4, LDOMesh> LDOMeshI;
    typedef MeshI   <face_t_t, Material, glm::vec2, glm::vec4, glm::vec4, TDOMesh> TDOMeshI;
    typedef MeshI   <face_q_t, Material, glm::vec2, glm::vec4, glm::vec4, QDOMesh> QDOMeshI;
    typedef MeshI   <face_l_t, Material, glm::vec2, glm::vec4, glm::vec4, LMesh> LMeshI;
    typedef MeshI   <face_t_t, Material, glm::vec2, glm::vec4, glm::vec4, TMesh> TMeshI;
    typedef MeshI   <face_q_t, Material, glm::vec2, glm::vec4, glm::vec4, QMesh> QMeshI;
}

#endif
