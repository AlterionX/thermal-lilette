#pragma once

#ifndef __PARTICLE_SYS_H__
#define __PARTICLE_SYS_H__

#include "mesh.h"

#include <chrono>
#include <ratio>

#include <glm/glm.hpp>

namespace psy {
    typedef std::chrono::duration<float, std::milli> lt_t;
    
    struct Particle {
        glm::vec3 v_; // velocity, m /ms
        glm::vec3 p_; // position, m
        float m_; // mass, g
        lt_t alive_; // alive, ms
        
        // force: kg * m / s^2 | duration: ms
        void push(const glm::vec3& force, const lt_t& duration);
        // elapsed: ms
        void simulate(lt_t elapsed);

        glm::mat4 g_mm(void) const;
    };

    class ParticleSystem {
        // basic simulation data
        std::vector<Particle> ps_;
        std::vector<glm::mat4> pmm_;
        geom::TMeshI pmi_;
        lt_t plt_; // ms

        // system management
        bool active_;
        bool grav_;
        
        void reset_p(Particle& p);
        void reset_ps(void);
        // pos: m
        glm::vec3 force_at(const glm::vec3& pos) const;
        
    public:
        ParticleSystem(int cnt);

        ParticleSystem() : ParticleSystem(1024) {}
        ParticleSystem(const ParticleSystem& copy) = default;
        ParticleSystem& operator=(const ParticleSystem& copy) = default;
        ParticleSystem(ParticleSystem&& move) = default;
        ParticleSystem& operator=(ParticleSystem&& move) = default;
        ~ParticleSystem() = default;

        // primary mathod, advances the simulation to the next step
        // elapsed: ms
        void step(const lt_t& elapsed);

        // set particle data
        // lt: ms
        void s_plt(const lt_t& lt);

        // set system config
        void toggle_active(void) { active_ = !active_; }
        void s_active(bool active) { active_ = active; }
        void toggle_grav(void) { grav_ = !grav_; }
        void s_grav(bool grav) { grav_ = grav; }

        // grab particle data
        size_t g_pcnt(void) const { return ps_.size(); }
        const std::vector<glm::mat4>& g_pmm(void) const { return pmm_; }
        const lt_t& g_plt(void) const { return plt_; }
        geom::TMeshI& g_pmi(void) { return pmi_; }

        // grab system config
        bool is_active(void) { return active_; }
        bool is_grav(void) { return grav_; }
    };
}

#endif
