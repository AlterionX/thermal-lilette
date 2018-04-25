#pragma once

#ifndef __PARTICLE_SYS_H__
#define __PARTICLE_SYS_H__

#include "mesh.h"

#include <chrono>
#include <glm/glm.hpp>

namespace psy {
    typedef std::chrono::duration<double> life_time_t;
    struct Particle {
        glm::vec3 velocity;
        glm::vec3 position;
        double mass;
        life_time_t time_alive;
        
        void apply_force(const glm::vec3& force);
        glm::mat4 grab_transform(void) const;
    };
    class ParticleSystem {
        Particle paritcles[1000];
        life_time_t p_life_time;
        geom::TMeshI particle_meshi;
        
        // Get the force at a point
        glm::vec3 g_force(const glm::vec3& pos) const;
        
    public:
        ParticleSystem();
        ParticleSystem(const ParticleSystem& copy) = default;
        ParticleSystem& operator=(const ParticleSystem& copy) = default;
        ParticleSystem(ParticleSystem&& move) = default;
        ParticleSystem& operator=(ParticleSystem&& move) = default;
        ~ParticleSystem() = default;

        // particle system's particle lifetimes
        void s_life_time(life_time_t lt);
        life_time_t g_life_time(void) const { return p_life_time; }
        
        // Advance each particle the provided time elapsed
        void advance(const life_time_t elapsed);

        int g_count(void) { return 1000; }
        bool is_active(void) { return true; }
        geom::TMeshI& g_meshi(void) { return particle_meshi; }
    };
}

#endif
