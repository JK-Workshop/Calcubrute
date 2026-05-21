// Copyright (c) JK Workshop - All rights reserved

#if !defined(JK_CALCUBRUTE_DEFAULT_FEATURES_H)
#define JK_CALCUBRUTE_DEFAULT_FEATURES_H

static struct VkPhysicalDeviceVulkan11Features s_vk11Feat = {
    VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES,
    nullptr
};
static struct VkPhysicalDeviceVulkan12Features s_vk12Feat = {
    VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES,
    &s_vk11Feat
};
static struct VkPhysicalDeviceVulkan13Features s_vk13Feat = {
    VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES,
    &s_vk12Feat
};
static struct VkPhysicalDeviceVulkan14Features s_vk14Feat = {
    VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_4_FEATURES,
    &s_vk13Feat
};
// VK_KHR_cooperative_matrix
static struct VkPhysicalDeviceCooperativeMatrixFeaturesKHR s_coopMatFeat = {
    VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_COOPERATIVE_MATRIX_FEATURES_KHR,
    &s_vk14Feat
};
// VK_KHR_device_address_commands
static struct VkPhysicalDeviceDeviceAddressCommandsFeaturesKHR s_devAddrCmdFeat = {
    VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DEVICE_ADDRESS_COMMANDS_FEATURES_KHR,
    &s_coopMatFeat
};
// VK_KHR_shader_fma
static struct VkPhysicalDeviceShaderFmaFeaturesKHR s_fmaFeat = {
    VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_FMA_FEATURES_KHR,
    &s_devAddrCmdFeat
};
// VK_NV_cooperative_matrix_2
static struct VkPhysicalDeviceCooperativeMatrix2FeaturesNV s_coopMat2Feat = {
    VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_COOPERATIVE_MATRIX_2_FEATURES_NV, 
    &s_fmaFeat
};
// VK_NV_shader_sm_builtins
static struct VkPhysicalDeviceShaderSMBuiltinsFeaturesNV s_smFeat = {
    VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_SM_BUILTINS_FEATURES_NV,
    &s_coopMat2Feat
};
// VK_NV_push_constant_bank
static struct VkPhysicalDevicePushConstantBankFeaturesNV s_pcBankFeat = {
    VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PUSH_CONSTANT_BANK_FEATURES_NV,
    &s_smFeat,
};
static struct VkPhysicalDeviceFeatures2 s_feat = {
    VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
    &s_pcBankFeat,
};

static const char* s_enabledExtNames[128];
static uint32_t    s_numEnabledExts = 0u;

#endif // JK_CALCUBRUTE_DEFAULT_FEATURES_H
