﻿#pragma once 
#include <vk_types.h>

namespace vkutil {

    load_shader_module(const char* filePath,
        VkDevice device,
        VkShaderModule* outShaderModule);
};