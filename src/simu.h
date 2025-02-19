#define PARTICLE_NUM 16


struct Particle {
	glm::vec2 pos{ 0, 0 };
	glm::vec2 velocity{ 0, 0 };
	float mass{ 1 };
};

class Fluid {
private:
	void init_particles_square(float spacing);
	
public:
	void init();
	void set_particles_square(float spacing);

	void cleanup();

	static const uint32_t particle_num = PARTICLE_NUM;
	Particle* particles[particle_num];
	float thing1 = 1;
	float thing2 = 1;
};