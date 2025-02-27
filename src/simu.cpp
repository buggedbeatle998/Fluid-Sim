#include "simu.h"
#include <vk_pipelines.h>
#include <vk_initializers.h>

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

void Fluid::init_commands()
{
    //create a command pool for commands submitted to the graphics queue.
    //we also want the pool to allow for resetting of individual command buffers
    VkCommandPoolCreateInfo commandPoolInfo = vkinit::command_pool_create_info(engine->_computeQueueFamily, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

    for (int i = 0; i < FRAME_OVERLAP; i++) {

        VK_CHECK(vkCreateCommandPool(engine->_device, &commandPoolInfo, nullptr, &_frames[i]._commandPool));

        // allocate the default command buffer that we will use for rendering
        VkCommandBufferAllocateInfo cmdAllocInfo = vkinit::command_buffer_allocate_info(_frames[i]._commandPool, 1);

        VK_CHECK(vkAllocateCommandBuffers(engine->_device, &cmdAllocInfo, &_frames[i]._mainCommandBuffer));
    }

    VK_CHECK(vkCreateCommandPool(engine->_device, &commandPoolInfo, nullptr, &_immCommandPool));

    // allocate the command buffer for immediate submits
    VkCommandBufferAllocateInfo cmdAllocInfo = vkinit::command_buffer_allocate_info(_immCommandPool, 1);

    VK_CHECK(vkAllocateCommandBuffers(engine->_device, &cmdAllocInfo, &_immCommandBuffer));

    _mainDeletionQueue.push_function([=]() {
        vkDestroyCommandPool(engine->_device, _immCommandPool, nullptr);
    });
}

void Fluid::init_syncs() {
    VkFenceCreateInfo fenceCreateInfo = vkinit::fence_create_info(VK_FENCE_CREATE_SIGNALED_BIT);
    VkSemaphoreCreateInfo semaphoreCreateInfo = vkinit::semaphore_create_info();

    for (int i = 0; i < FRAME_OVERLAP; i++) {
        VK_CHECK(vkCreateFence(engine->_device, &fenceCreateInfo, nullptr, &_frames[i]._computeFence));

        VK_CHECK(vkCreateSemaphore(engine->_device, &semaphoreCreateInfo, nullptr, &_frames[i]._computeSemaphore));
    }

    VK_CHECK(vkCreateFence(engine->_device, &fenceCreateInfo, nullptr, &_immFence));
    _mainDeletionQueue.push_function([=]() {
        vkDestroyFence(engine->_device, _immFence, nullptr);
    });
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

    for (int i = 0; i < FRAME_OVERLAP; i++) {
        _frames[i]._input_buffer = engine->create_buffer(sizeof(Particle) * PARTICLE_NUM * 2, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
        _frames[i]._output_buffer = engine->create_buffer(sizeof(Particle) * PARTICLE_NUM * 2, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VMA_MEMORY_USAGE_GPU_TO_CPU);

        _mainDeletionQueue.push_function([&, i]() {
            engine->destroy_buffer(_frames[i]._input_buffer);
            engine->destroy_buffer(_frames[i]._output_buffer);
        });
    }

    _mainDeletionQueue.push_function([&]() {
        fluid_allocator.destroy_pools(engine->_device);

        //vkDestroyDescriptorSetLayout(engine->_device, _drawImageDescriptorLayout, nullptr);
        vkDestroyDescriptorSetLayout(engine->_device, _physics_input_descriptor_layout, nullptr);
        vkDestroyDescriptorSetLayout(engine->_device, _physics_output_descriptor_layout, nullptr);
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

#pragma region Step
void Fluid::step(uint32_t particle_count, float delta_time) {
    get_current_frame()._deletionQueue.flush();
    get_current_frame()._frameDescriptors.clear_pools(engine->_device);

    VkCommandBuffer cmd = get_current_frame()._mainCommandBuffer;

    VK_CHECK(vkResetCommandBuffer(cmd, 0));
    
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

    VK_CHECK(vkBeginCommandBuffer(cmd, &beginInfo));

    physics_step(cmd, particle_count, delta_time);

    VK_CHECK(vkEndCommandBuffer(cmd));

    VkCommandBufferSubmitInfo cmdinfo = vkinit::command_buffer_submit_info(cmd);

    VkSemaphoreSubmitInfo signalInfo = vkinit::semaphore_submit_info(VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, get_current_frame()._computeSemaphore);

    VkSubmitInfo2 submit = vkinit::submit_info(&cmdinfo, &signalInfo, &waitInfo);

    VK_CHECK(vkQueueSubmit2(engine->_computeQueue, 1, &submit, nullptr));
}

void Fluid::physics_step(VkCommandBuffer &cmd, uint32_t particle_count, float delta_time) {

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
    vmaMapMemory(engine->_allocator, get_current_frame()._input_buffer.allocation, &particle_data);

    Particle* _input = (Particle*)particle_data;

    for (uint32_t i = 0; i < PARTICLE_NUM; i++)
    {
        _input[i + i] = *particles[i];
    }

    vmaUnmapMemory(engine->_allocator, get_current_frame()._input_buffer.allocation);

    _physics_input_descriptors = fluid_allocator.allocate(engine->_device, _physics_input_descriptor_layout);
    {
        DescriptorWriter writer;
        writer.write_buffer(0, get_current_frame()._input_buffer.buffer, sizeof(Particle) * PARTICLE_NUM * 2, 0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
        writer.update_set(engine->_device, _physics_input_descriptors);
    }

    _physics_output_descriptors = fluid_allocator.allocate(engine->_device, _physics_output_descriptor_layout);

    vkCmdBindPipeline(get_current_frame()._mainCommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, _physics_pipeline);

    vkCmdBindDescriptorSets(get_current_frame()._mainCommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, _physics_pipeline_layout, 0, 1, &_physics_input_descriptors, 0, nullptr);
    vkCmdBindDescriptorSets(get_current_frame()._mainCommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, _physics_pipeline_layout, 0, 1, &_physics_output_descriptors, 0, nullptr);

    vkCmdPushConstants(get_current_frame()._mainCommandBuffer, _physics_pipeline_layout, VK_PIPELINE_BIND_POINT_COMPUTE, 0, sizeof(Config), &config_data);

    int groupcount = ((PARTICLE_NUM + 255) >> 8);

    vkCmdDispatch(get_current_frame()._mainCommandBuffer, groupcount, 1, 1);



}
#pragma endregion Step

void Fluid::cleanup() {
    for (auto& particle : particles) {
        delete particle;
    }
}
#pragma endregion Fluid