#include "VkBootstrap.h"
#include <vk_engine.h>

#define PARTICLE_NUM 2500


class VulkanEngine;

//class DescriptorAllocatorGrowable;
//class VkDescriptorSet;
//class VkDescriptorSetLayout;

struct Config {
	uint32_t particle_count{ 0 };
	float delta_time{ 0.f };
};

struct Particle {
	glm::vec2 pos{ 0, 0 };
	glm::vec2 velocity{ 0, 0 };
	float mass{ 1 };
};

class Fluid {
private:

	VulkanEngine *engine;

	DescriptorAllocatorGrowable fluid_allocator;

	VkPipelineLayout _physics_pipeline_layout;
	VkPipeline _physics_pipeline;

	/*AllocatedBuffer _config_buffer;
	VkDescriptorSet _physics_config_descriptors;
	VkDescriptorSetLayout _physics_config_descriptor_layout;*/

	AllocatedBuffer _input_buffer;
	VkDescriptorSet _physics_input_descriptors;
	VkDescriptorSetLayout _physics_input_descriptor_layout;

	AllocatedBuffer _output_buffer;
	VkDescriptorSet _physics_output_descriptors;
	VkDescriptorSetLayout _physics_output_descriptor_layout;

	void init_buffers();
	void init_physics_pipeline();
	void init_bounding_area(float right, float up, float left, float down);
	void init_particles_square(float spacing);
	
public:
	void init(VulkanEngine *_engine);
	void set_particles_square(float spacing);

	void step(VkCommandBuffer &cmd, uint32_t particle_count, float delta_time);
	void physics_step(VkCommandBuffer &cmd, uint32_t particle_count, float delta_time);

	void cleanup();

	static const uint32_t particle_num = PARTICLE_NUM;
	Particle* particles[particle_num];

	float bounding_area[4];
};