#pragma once

constexpr uint32_t PARTICLE_NUM = 2500;

#include "VkBootstrap.h"
#include <vk_engine.h>



class VulkanEngine;

struct Config {
	uint32_t particle_count{ 0 };
	float delta_time{ 0.f };
};

struct Particle {
	glm::vec2 pos{ 0, 0 };
	glm::vec2 velocity{ 0, 0 };
	float mass{ 1 };
};

struct FluidDeletionQueue
{
	std::deque<std::function<void()>> deletors;

	void push_function(std::function<void()>&& function) {
		deletors.push_back(function);
	}

	void flush() {
		// reverse iterate the deletion queue to execute all the functions
		for (auto it = deletors.rbegin(); it != deletors.rend(); it++) {
			(*it)(); //call functors
		}

		deletors.clear();
	}
};

struct FluidFrameData {

	VkCommandPool _commandPool;
	VkCommandBuffer _mainCommandBuffer;

	VkSemaphore _computeSemaphore;
	VkFence _computeFence;

	FluidDeletionQueue _deletionQueue;
	DescriptorAllocatorGrowable _frameDescriptors;

	AllocatedBuffer _input_buffer;
	AllocatedBuffer _output_buffer;
};


class Fluid {
private:

	VulkanEngine *engine;

	FluidDeletionQueue _mainDeletionQueue;

	FluidFrameData _frames[FRAME_OVERLAP]{};
	FluidFrameData& get_current_frame();

	VkCommandPool _immCommandPool;
	VkCommandBuffer _immCommandBuffer;
	VkFence _immFence;

	DescriptorAllocatorGrowable fluid_allocator;

	VkPipelineLayout _physics_pipeline_layout;
	VkPipeline _physics_pipeline;

	/*AllocatedBuffer _config_buffer;
	VkDescriptorSet _physics_config_descriptors;
	VkDescriptorSetLayout _physics_config_descriptor_layout;*/

	VkDescriptorSet _physics_input_descriptors;
	VkDescriptorSetLayout _physics_input_descriptor_layout;

	VkDescriptorSet _physics_output_descriptors;
	VkDescriptorSetLayout _physics_output_descriptor_layout;
	
	void init_commands();
	void init_syncs();
	void init_buffers();
	void init_physics_pipeline();
	void init_bounding_area(float right, float up, float left, float down);
	void init_particles_square(float spacing);

	void physics_step(uint32_t particle_count, float delta_time);
	
public:
	void init(VulkanEngine *_engine);
	void set_particles_square(float spacing);

	void step(uint32_t particle_count, float delta_time);

	void cleanup();

	static const uint32_t particle_num = PARTICLE_NUM;
	Particle* particles[particle_num];

	float bounding_area[4];
};