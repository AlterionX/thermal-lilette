#pragma once

#ifndef __PARTICLE_SYS_H__
#define __PARTICLE_SYS_H__

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
        
        // Get the force at a point
        glm::vec3 g_force(const glm::vec3& pos) const;
        
    public:
        // particle system's particle lifetimes
        void s_life_time(life_time_t lt);
        double g_life_time(void) const { return p_life_time; }
        
        // Advance each particle the provided time elapsed
        void advance(const life_time_t elapsed);
    };
}

#endif
