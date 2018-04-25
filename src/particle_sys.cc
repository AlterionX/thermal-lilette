#include "particle_sys.h"
#include "prim_mesh.h"

namespace psy {
    void Particle::apply_force(const glm::vec3& force) {
        velocity += force / float(mass);
    }
    glm::mat4 Particle::grab_transform(void) const {
        return glm::translate(position);
    }
    ParticleSystem::ParticleSystem() : particle_meshi(geom::c_cube()) {}
    // Get the force at a point
    glm::vec3 ParticleSystem::g_force(const glm::vec3& pos) const {
        // TODO
    }
    // particle system's particle lifetimes
    void ParticleSystem::s_life_time(life_time_t lt) {
        p_life_time = lt;
    }
    // Advance each particle the provided time elapsed
    void advance(const life_time_t elapsed) {
        // TODO
    }
}