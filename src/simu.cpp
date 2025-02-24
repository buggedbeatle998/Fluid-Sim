#include "simu.h"
#include <vk_pipelines.h>

#include <glm/gtx/transform.hpp>
#include <glm/gtx/quaternion.hpp>

#pragma region Helpers
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
#pragma endregion Helpers


#pragma region Fluid
#pragma region Inits
void Fluid::init(VulkanEngine *_engine) {
    engine = _engine;

    init_buffers();

    init_bounding_area(4, -3, -4, 3);
    init_particles_square(0.1f);
}

void Fluid::init_bounding_area(float right, float up, float left, float down) {
    bounding_area[0] = right;
    bounding_area[1] = up;
    bounding_area[2] = left;
    bounding_area[3] = down;
}

void Fluid::init_buffers() {
    std::vector<DescriptorAllocatorGrowable::PoolSizeRatio> sizes =
    {
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1 },
        { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 2 },
    };
    fluid_allocator.init(engine->_device, 12, sizes);

    /*{
        DescriptorLayoutBuilder builder;
        builder.add_binding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
        _physics_config_descriptor_layout = builder.build(engine->_device, VK_SHADER_STAGE_COMPUTE_BIT);
    }*/
    {
        DescriptorLayoutBuilder builder;
        builder.add_binding(0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
        _physics_input_descriptor_layout = builder.build(engine->_device, VK_SHADER_STAGE_COMPUTE_BIT);
    }
    {
        DescriptorLayoutBuilder builder;
        builder.add_binding(1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
        _physics_output_descriptor_layout = builder.build(engine->_device, VK_SHADER_STAGE_COMPUTE_BIT);
    }

    //_config_buffer = engine->create_buffer(sizeof(Config), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
    _input_buffer = engine->create_buffer(sizeof(Particle) * PARTICLE_NUM * 2, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
    _output_buffer = engine->create_buffer(sizeof(Particle) * PARTICLE_NUM * 2, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VMA_MEMORY_USAGE_GPU_TO_CPU);

    engine->_mainDeletionQueue.push_function([&]() {
        fluid_allocator.destroy_pools(engine->_device);

        //vkDestroyDescriptorSetLayout(engine->_device, _drawImageDescriptorLayout, nullptr);
        vkDestroyDescriptorSetLayout(engine->_device, _physics_input_descriptor_layout, nullptr);
        vkDestroyDescriptorSetLayout(engine->_device, _physics_output_descriptor_layout, nullptr);

        engine->destroy_buffer(_input_buffer);
        engine->destroy_buffer(_output_buffer);
    });
}

void Fluid::init_physics_pipeline()
{
    VkDescriptorSetLayout setLayouts[] = { _physics_input_descriptor_layout,
                                           _physics_output_descriptor_layout };

    VkPipelineLayoutCreateInfo computeLayout{};
    computeLayout.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    computeLayout.pNext = nullptr;
    computeLayout.pSetLayouts = setLayouts;
    computeLayout.setLayoutCount = 1;

    VkPushConstantRange pushConstant{};
    pushConstant.offset = 0;
    pushConstant.size = sizeof(Config);
    pushConstant.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    computeLayout.pPushConstantRanges = &pushConstant;
    computeLayout.pushConstantRangeCount = 1;

    VK_CHECK(vkCreatePipelineLayout(engine->_device, &computeLayout, nullptr, &_physics_pipeline_layout));


    VkShaderModule physics_step_shader;
    if (!vkutil::load_shader_module("../../shaders/physics_step.comp.spv", engine->_device, &physics_step_shader)) {
        fmt::print("Error when building the compute shader \n");
    }

    VkPipelineShaderStageCreateInfo stageinfo{};
    stageinfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stageinfo.pNext = nullptr;
    stageinfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    stageinfo.module = physics_step_shader;
    stageinfo.pName = "main";

    VkComputePipelineCreateInfo computePipelineCreateInfo{};
    computePipelineCreateInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    computePipelineCreateInfo.pNext = nullptr;
    computePipelineCreateInfo.layout = _physics_pipeline_layout;
    computePipelineCreateInfo.stage = stageinfo;

    VK_CHECK(vkCreateComputePipelines(engine->_device, VK_NULL_HANDLE, 1, &computePipelineCreateInfo, nullptr, &_physics_pipeline));

    //destroy structures properly
    vkDestroyShaderModule(engine->_device, physics_step_shader, nullptr);
    engine->_mainDeletionQueue.push_function([=]() {
        vkDestroyPipelineLayout(engine->_device, _physics_pipeline_layout, nullptr);
        vkDestroyPipeline(engine->_device, _physics_pipeline, nullptr);
    });

}

void Fluid::init_particles_square(float spacing) {
    uint16_t edge_len = isqrt(particle_num);
    float topleft = (-edge_len / 2);
	for (uint32_t i = 0; i < particle_num; i++) {
		Particle* new_part = new Particle{};
        new_part->pos = glm::vec2((topleft + (i % edge_len)) * spacing,
                                  (topleft + ((int)i / edge_len)) * spacing);
        new_part->velocity = glm::vec2(0, 0);
        
        particles[i] = new_part;
	}
}

void Fluid::set_particles_square(float spacing) {
    uint16_t edge_len = isqrt(particle_num);
    float topleft = (-edge_len / 2);
    for (uint32_t i = 0; i < particle_num; i++) {
        Particle* new_part = particles[i];
        new_part->pos = glm::vec2((topleft + (i % edge_len)) * spacing,
                                  (topleft + ((int)i / edge_len)) * spacing);
        new_part->velocity = glm::vec2(0, 0);
    }
}

#pragma endregion Inits

#pragma region Physics
void Fluid::physics_step(uint32_t particle_count, float delta_time) {

    Config config_data{};
    config_data.particle_count = particle_count;
    config_data.delta_time = delta_time;
    //memcpy(_config_buffer.info.pMappedData, &config_data, sizeof(Config));

    /*_physics_config_descriptors = fluid_allocator.allocate(engine->_device, _physics_config_descriptor_layout);
    {
        DescriptorWriter writer;
        writer.write_buffer(0, _config_buffer.buffer, sizeof(Config), 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
        writer.update_set(engine->_device, _physics_config_descriptors);
    }*/

    void* particle_data;
    vmaMapMemory(engine->_allocator, _input_buffer.allocation, &particle_data);

    Particle* _input = (Particle*)particle_data;

    for (uint32_t i = 0; i < PARTICLE_NUM; i++)
    {
        _input[i + i] = *particles[i];
    }

    vmaUnmapMemory(engine->_allocator, _input_buffer.allocation);

    _physics_input_descriptors = fluid_allocator.allocate(engine->_device, _physics_input_descriptor_layout);
    {
        DescriptorWriter writer;
        writer.write_buffer(0, _input_buffer.buffer, sizeof(Particle) * PARTICLE_NUM * 2, 0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
        writer.update_set(engine->_device, _physics_input_descriptors);
    }

    _physics_output_descriptors = fluid_allocator.allocate(engine->_device, _physics_output_descriptor_layout);

    vkCmdBindPipeline(engine->get_current_frame()._mainCommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, _physics_pipeline);

    vkCmdBindDescriptorSets(engine->get_current_frame()._mainCommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, _physics_pipeline_layout, 0, 1, &_physics_input_descriptors, 0, nullptr);
    vkCmdBindDescriptorSets(engine->get_current_frame()._mainCommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, _physics_pipeline_layout, 0, 1, &_physics_output_descriptors, 0, nullptr);

    vkCmdPushConstants(engine->get_current_frame()._mainCommandBuffer, _physics_pipeline_layout, VK_PIPELINE_BIND_POINT_COMPUTE, 0, sizeof(Config), &config_data);
}
#pragma endregion Physics

void Fluid::cleanup() {
    for (auto& particle : particles) {
        delete particle;
    }
	//delete[] particles;
}
#pragma endregion Fluid