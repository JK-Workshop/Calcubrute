// Copyright (c) JK Workshop - All rights reserved

#include <JK/Calcubrute/Context.h>

/**
 * @brief 
 * 
 * @param p_physicalDevice 
 */
static inline void
initContextGetTensorCoreCapabilities(const VkPhysicalDevice p_physicalDevice) noexcept
{
    bool hasCoopMatKHR = false;
    bool hasCoopMat2NV = false;
    bool hasCoopVecNV  = false;

    for (uint32_t i = 0; i < s_numExtensions; ++i) {
        if (strcmp(s_extensions->extensionName, VK_KHR_COOPERATIVE_MATRIX_EXTENSION_NAME) == 0) {
            hasCoopMatKHR = true;
        }
        else if (strcmp(s_extensions->extensionName, VK_NV_COOPERATIVE_MATRIX_2_EXTENSION_NAME) == 0) {
            hasCoopMat2NV = true;
        }
        else if (strcmp(s_extensions->extensionName, VK_NV_COOPERATIVE_VECTOR_EXTENSION_NAME) == 0) {
            hasCoopVecNV = true;
        }
    }

    uint32_t numProps;
    if (hasCoopMat2NV) {
        //VkCooperativeMatrix2PropertiesNV
    }
    VkCooperativeMatrixPropertiesKHR coopMatProps[64];
    VK_CHECK(vkGetPhysicalDeviceCooperativeMatrixPropertiesKHR(p_physicalDevice, &numProps, nullptr));
    VK_CHECK(vkGetPhysicalDeviceCooperativeMatrixPropertiesKHR(p_physicalDevice, &numProps, coopMatProps));
}

/**
 * @brief 
 * 
 * @param p_physicalDevice 
 */
static inline void
initContextSetAttributes(const VkPhysicalDevice p_physicalDevice) noexcept
{
    // Get physical device vender
    VkPhysicalDeviceProperties2 prop {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2,
        .pNext = nullptr
    };
    vkGetPhysicalDeviceProperties2(p_physicalDevice, &prop);
    deviceAttributes.vendorID = prop.properties.vendorID;

    // Get all physical device attributes
    VkPhysicalDeviceVulkan11Properties vk11Prop {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_PROPERTIES,
        .pNext = nullptr
    };
    VkPhysicalDeviceVulkan12Properties vk12Prop {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_PROPERTIES,
        .pNext = &vk11Prop
    };
    VkPhysicalDeviceVulkan13Properties vk13Prop {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_PROPERTIES,
        .pNext = &vk12Prop,
    };
    VkPhysicalDeviceVulkan14Properties vk14Prop {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_4_PROPERTIES,
        .pNext = &vk13Prop
    };
    switch (deviceAttributes.vendorID) {
        case DEVICE_VENDOR_AMD: {
            vkGetPhysicalDeviceProperties2(p_physicalDevice, &prop);
            break;
        }
        case DEVICE_VENDOR_INTEL: {
            vkGetPhysicalDeviceProperties2(p_physicalDevice, &prop);
            break;
        }
        case DEVICE_VENDOR_NV: {
            VkPhysicalDevicePushConstantBankPropertiesNV pcBankProp {
                .sType                        = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PUSH_CONSTANT_BANK_PROPERTIES_NV,
                .pNext                        = &vk14Prop,
                .maxGraphicsPushConstantBanks = UINT32_MAX,
                .maxComputePushConstantBanks  = UINT32_MAX,
                .maxGraphicsPushDataBanks     = UINT32_MAX,
                .maxComputePushDataBanks      = UINT32_MAX
            };
            VkPhysicalDeviceShaderSMBuiltinsPropertiesNV smProp {
                .sType                        = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_SM_BUILTINS_PROPERTIES_NV,
                .pNext                        = &pcBankProp,
                .shaderSMCount                = UINT32_MAX,
                .shaderWarpsPerSM             = UINT32_MAX
            };
            prop.pNext = &smProp;
            vkGetPhysicalDeviceProperties2(p_physicalDevice, &prop);
            deviceAttributes.maxNumPushConstBanks                   = pcBankProp.maxComputePushConstantBanks;
            deviceAttributes.maxNumPushDataBanks                    = pcBankProp.maxComputePushDataBanks;
            deviceAttributes.numStreamingMultiprocessors            = smProp.shaderSMCount;
            deviceAttributes.numSubgroupsPerStreamingMultiprocessor = smProp.shaderWarpsPerSM;
            break;
        }
        default: {
            vkGetPhysicalDeviceProperties2(p_physicalDevice, &prop);
        }
    }
    memcpy(deviceAttributes.deviceName, prop.properties.deviceName, VK_MAX_PHYSICAL_DEVICE_NAME_SIZE * sizeof(char));
    memcpy(deviceAttributes.driverName, vk12Prop.driverName, VK_MAX_DRIVER_NAME_SIZE * sizeof(char));
    memcpy(deviceAttributes.driverInfo, vk12Prop.driverInfo, VK_MAX_DRIVER_INFO_SIZE * sizeof(char));
    deviceAttributes.vulkanVersion                = prop.properties.apiVersion;
    deviceAttributes.maxPushConstSize             = prop.properties.limits.maxPushConstantsSize;
    deviceAttributes.maxUniformBufferSize         = prop.properties.limits.maxUniformBufferRange;
    deviceAttributes.maxWorkgroupMemorySize       = prop.properties.limits.maxComputeSharedMemorySize;
    deviceAttributes.maxMallocSize                = vk11Prop.maxMemoryAllocationSize;
    deviceAttributes.minNumInvocationsPerSubgroup = vk13Prop.minSubgroupSize;
    deviceAttributes.maxNumInvocationsPerSubgroup = vk13Prop.maxSubgroupSize;
}
