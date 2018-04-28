#include "particle_sys.h"
#include "prim_mesh.h"

#include <iostream>
#include <glm/gtx/string_cast.hpp>
#include <glm/gtx/io.hpp>

namespace psy {
    void Particle::push(const glm::vec3& force, const lt_t& duration) {
        // (kg * m / s^2) / 1000 => g * m / s^2
        // (g * m / s^2) * 1000000 => g * m / ms^2
        // (g * m / ms^2) / g => m / ms^2
        // (m / ms^2) * ms = m / ms
        v_ += force * 1000.0f / m_ * duration.count();
    }
    void Particle::simulate(lt_t elapsed) {
        // (m / ms) * ms = m
        p_ += v_ * elapsed.count();
        alive_ += elapsed;
    }
    glm::mat4 Particle::g_mm(void) const {
        return glm::translate(p_);
    }
    
    ParticleSystem::ParticleSystem(int cnt)
    : ps_(cnt), pmm_(cnt), pmi_(geom::c_cube()) {
        // prime everything
        reset_ps();
        for (int i = 0; i < int(g_pcnt()); ++i) {
            ps_[i].p_ = glm::vec3(0.0f, i, 0.0f);
            pmm_[i] = ps_[i].g_mm();
        }
        std::cout << glm::to_string(pmm_[5]) << std::endl;
    }

    void ParticleSystem::reset_p(Particle& p) {
        // std::cout << "reset" << std::endl;
        p.v_ = glm::vec3(); // TODO insert initial velocity
        p.p_ = glm::vec3(); // TODO insert arbitrary start
        p.m_ = 1.0f; // TODO insert variable/random mass
        p.alive_ = lt_t(0);
    }
    void ParticleSystem::reset_ps(void) {
        for (auto& p : ps_) reset_p(p);
    }
    glm::vec3 ParticleSystem::force_at(const glm::vec3& pos) const {
        if (grav_) {
            return glm::vec3(0.0f, -9.80665f, 0.0f);
        } else {
            return glm::vec3(); // TODO add additional forces beyond gravity
        }
    }
    // particle system's particle lifetimes
    void ParticleSystem::s_plt(const lt_t& lt) {
        plt_ = lt;
    }

    // Advance each particle the provided time elapsed
    void ParticleSystem::step(const lt_t& elapsed) {
        // TODO more accurate simulation
        for (int i = 0; i < int(g_pcnt()); i++) {
            if (ps_[i].alive_ > plt_) {
                reset_p(ps_[i]);
            } else {
                ps_[i].push(force_at(ps_[i].p_), elapsed);
                ps_[i].simulate(elapsed);
                pmm_[i] = ps_[i].g_mm();
            }
        }
    }
}