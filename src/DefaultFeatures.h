// Copyright (c) JK Workshop - All rights reserved

#if !defined(JK_CALCUBRUTE_DEFAULT_FEATURES_H)
#define JK_CALCUBRUTE_DEFAULT_FEATURES_H

static struct VkPhysicalDeviceVulkan11Features s_vk11Feat = {
    .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES,
    .pNext = nullptr,
};
static struct VkPhysicalDeviceVulkan12Features s_vk12Feat = {
    .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES,
    .pNext = &s_vk11Feat,
    .shaderFloat16     = VK_TRUE,
    .vulkanMemoryModel = VK_TRUE,
};
static struct VkPhysicalDeviceVulkan13Features s_vk13Feat = {
    .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES,
    .pNext = &s_vk12Feat,
};
static struct VkPhysicalDeviceVulkan14Features s_vk14Feat = {
    .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_4_FEATURES,
    .pNext = &s_vk13Feat,
};
// VK_KHR_cooperative_matrix
static struct VkPhysicalDeviceCooperativeMatrixFeaturesKHR s_coopMatFeat = {
    .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_COOPERATIVE_MATRIX_FEATURES_KHR,
    .pNext = &s_vk14Feat,
    .cooperativeMatrix                   = VK_FALSE,
    .cooperativeMatrixRobustBufferAccess = VK_FALSE
};
// VK_KHR_device_address_commands
static struct VkPhysicalDeviceDeviceAddressCommandsFeaturesKHR s_devAddrCmdFeat = {
    .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DEVICE_ADDRESS_COMMANDS_FEATURES_KHR,
    .pNext = &s_coopMatFeat,
    .deviceAddressCommands = VK_FALSE
};
// VK_KHR_shader_fma
static struct VkPhysicalDeviceShaderFmaFeaturesKHR s_fmaFeat = {
    .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_FMA_FEATURES_KHR,
    .pNext = &s_devAddrCmdFeat,
    .shaderFmaFloat16 = VK_FALSE,
    .shaderFmaFloat32 = VK_FALSE,
    .shaderFmaFloat64 = VK_FALSE
};
// VK_NV_shader_sm_builtins
static struct VkPhysicalDeviceShaderSMBuiltinsFeaturesNV s_smFeat = {
    .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_SM_BUILTINS_FEATURES_NV,
    .pNext = &s_fmaFeat,
    .shaderSMBuiltins = VK_FALSE
};
// VK_NV_push_constant_bank
static struct VkPhysicalDevicePushConstantBankFeaturesNV s_pcBankFeat = {
    .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PUSH_CONSTANT_BANK_FEATURES_NV,
    .pNext = &s_smFeat,
    .pushConstantBank = VK_FALSE
};
static struct VkPhysicalDeviceFeatures2 s_feat = {
    .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
    .pNext = &s_pcBankFeat,
    .features = {0}
};

static const char* s_enabledExtNames[128];
static uint32_t    s_numEnabledExts = 0u;

#endif // JK_CALCUBRUTE_DEFAULT_FEATURES_H
