#include "simu.h"

#include <glm/gtx/transform.hpp>
#include <glm/gtx/quaternion.hpp>

uint16_t isqrt(uint32_t num) {
    short res = 0;
    short bit = 1 << 14; // The second-to-top bit is set: 1L<<30 for long

    // "bit" starts at the highest power of four <= the argument.
    while (bit > num)
        bit >>= 2;

    while (bit != 0) {
        if (num >= res + bit) {
            num -= res + bit;
            res = (res >> 1) + bit;
        }
        else
            res >>= 1;
        bit >>= 2;
    }
    return res;
}

#pragma region Fluid
#pragma region Inits
void Fluid::init() {
    init_particles_square(1);
}

void Fluid::init_particles_square(float spacing) {
    spacing *= thing2;
    uint16_t edge_len = isqrt(particle_num);
    float topleft = (-edge_len / 2) * thing1;
	for (uint32_t i = 0; i < particle_num; i++) {
		Particle* new_part = new Particle{};
        new_part->pos = glm::vec2((topleft + (i % edge_len)) * spacing,
                                  (topleft + ((int)i / edge_len)) * spacing);
        new_part->velocity = glm::vec2(0, 0);
        
        particles[i] = new_part;
	}
}

void Fluid::set_particles_square(float spacing) {
    spacing *= thing2;
    uint16_t edge_len = isqrt(particle_num);
    float topleft = (-edge_len / 2) * thing1;
    for (uint32_t i = 0; i < particle_num; i++) {
        Particle* new_part = particles[i];
        new_part->pos = glm::vec2((topleft + (i % edge_len)) * spacing,
            (topleft + ((int)i / edge_len)) * spacing);
        new_part->velocity = glm::vec2(0, 0);
    }
}
#pragma endregion Inits

void Fluid::cleanup() {
    for (auto& particle : particles) {
        delete particle;
    }
	//delete[] particles;
}
#pragma endregion Fluid