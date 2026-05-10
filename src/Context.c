// Copyright (c) JK Workshop - All rights reserved

#include <JK/Calcubrute/Context.h>

constexpr uint32_t DEVICE_VENDOR_AMD   = 0x1002u;
constexpr uint32_t DEVICE_VENDOR_INTEL = 0x8086u;
constexpr uint32_t DEVICE_VENDOR_NV    = 0x10DEu;
//constexpr uint32_t DEVICE_VENDER_ARM   =

static uint32_t                      s_numExtensions;
static struct VkExtensionProperties* s_extensions = nullptr;

/**
 * @brief Retrieve the whole extension list from the specified p_physicalDevice
 * 
 * @param p_physicalDevice physical device to be queried on
 * @return int CCB_SUCCESS on success, CCB_MALLOC_ERROR on failure
 */
static inline int
contextInitGetExtensions(const VkPhysicalDevice p_physicalDevice)
{
    VK_CHECK(vkEnumerateDeviceExtensionProperties(p_physicalDevice, nullptr, &s_numExtensions, nullptr));
    CCB_REALLOC(struct VkExtensionProperties, s_extensions, s_numExtensions); // Freed inside ccbInitContext
    VK_CHECK(vkEnumerateDeviceExtensionProperties(p_physicalDevice, nullptr, &s_numExtensions, s_extensions));
    return CCB_SUCCESS;
}

/**
 * @brief Loop through physical device groups and pick the one specified by p_deviceGroupIndex.
 * 
 * @param p_context
 * @param p_instance VkInstance to be queried
 * @param p_deviceGroupIndex index into detected device group list, specifying the group to be picked
 * @return int CCB_SUCCESS on success, CCB_ARGUMENT_ERROR, CCB_MALLOC_ERROR on failure
 */
static inline int
contextInitPickDeviceGroup(struct CcbContext* const p_context,
                           const VkInstance         p_instance,
                           const uint32_t           p_deviceGroupIndex)
{
    // List and check device group properties
    uint32_t numDeviceGroups;
    VK_CHECK(vkEnumeratePhysicalDeviceGroups(p_instance, &numDeviceGroups, nullptr));
    if (p_deviceGroupIndex >= numDeviceGroups) {
        return CCB_ARGUMENT_ERROR;
    }
    CCB_MALLOC(struct VkPhysicalDeviceGroupProperties, deviceGroupProps, numDeviceGroups);
    CCB_INIT_VK_STRUCT_ARRAY(deviceGroupProps,
                             numDeviceGroups,
                             VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_GROUP_PROPERTIES,
                             nullptr);
    VK_CHECK(vkEnumeratePhysicalDeviceGroups(p_instance, &numDeviceGroups, deviceGroupProps));
    p_context->numPhysicalDevices = deviceGroupProps[p_deviceGroupIndex].physicalDeviceCount;
    memcpy(p_context->physicalDevices,
           deviceGroupProps[p_deviceGroupIndex].physicalDevices,
           p_context->numPhysicalDevices * sizeof(VkPhysicalDevice));
    CCB_FREE(deviceGroupProps);
    return CCB_SUCCESS;
}

/**
 * @brief 
 * 
 * @param p_context 
 * @return int 
 */
static inline int
contextInitLocateQueueFamilyIndices(struct CcbContext* const p_context)
{
    uint32_t numQueueFamilies;
    vkGetPhysicalDeviceQueueFamilyProperties2(p_context->physicalDevices[0], &numQueueFamilies, nullptr);
    struct VkQueueFamilyProperties2 queueFamilyProps[8];
    CCB_INIT_VK_STRUCT_ARRAY(queueFamilyProps, 8, VK_STRUCTURE_TYPE_QUEUE_FAMILY_PROPERTIES_2, nullptr);
    vkGetPhysicalDeviceQueueFamilyProperties2(p_context->physicalDevices[0], &numQueueFamilies, queueFamilyProps);

    // Invalidate
    p_context->transferQueueFamilyIndex = -1;
    p_context->computeQueueFamilyIndex  = -1;

    // Ties broken by ascending order of index
    for (int i = numQueueFamilies - 1; i >= 0; --i) {
        const uint32_t flags = queueFamilyProps[i].queueFamilyProperties.queueFlags;

        // Try locating dedicated transfer queue family
        if ((flags &
                (VK_QUEUE_TRANSFER_BIT | VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT)
            ) == VK_QUEUE_TRANSFER_BIT)
        {
            p_context->transferQueueFamilyIndex = i;
        }
        // If failed, then locate any queue family with transfer capability
        else if (p_context->transferQueueFamilyIndex == -1 && (flags & VK_QUEUE_TRANSFER_BIT)) {
            p_context->transferQueueFamilyIndex = i;
        }

        // Locate compute queue family
        if (flags & VK_QUEUE_COMPUTE_BIT) {
            p_context->computeQueueFamilyIndex = i;
        }
    }

    if (p_context->transferQueueFamilyIndex == -1 || p_context->computeQueueFamilyIndex == -1) {
        return CCB_QUEUE_FAMILY_MISS;
    }
    return CCB_SUCCESS;
}

/**
 * @brief 
 * 
 * @param p_context
 */
static inline void
contextInitCreateTimelineSemaphore(struct CcbContext* const p_context)
{
    struct VkSemaphoreTypeCreateInfo semaphoreTypeInfo = {
        .sType         = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO,
        .pNext         = nullptr,
        .semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE,
        .initialValue  = 0ull
    };
    struct VkSemaphoreCreateInfo semaphoreInfo = {
        .sType         = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
        .pNext         = &semaphoreTypeInfo,
        .flags         = 0u
    };
    VK_CHECK(vkCreateSemaphore(p_context->device, &semaphoreInfo, nullptr, &p_context->timelineSemaphore));
}

/**
 * @brief 
 * 
 * @param p_context 
 */
static inline void
contextInitGetTensorCoreProperties(struct CcbContext* const p_context)
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
    struct VkCooperativeMatrixPropertiesKHR coopMatProps[64];
    VK_CHECK(vkGetPhysicalDeviceCooperativeMatrixPropertiesKHR(p_context->physicalDevices[0], &numProps, nullptr));
    VK_CHECK(vkGetPhysicalDeviceCooperativeMatrixPropertiesKHR(p_context->physicalDevices[0], &numProps, coopMatProps));
}

static inline void
contextInitGetPropertiesNV(struct CcbContext* const            p_context,
                           VkPhysicalDeviceProperties2*        p_prop,
                           VkPhysicalDeviceVulkan14Properties* p_vk14Prop)
{
    struct VkPhysicalDevicePushConstantBankPropertiesNV pcBankProp = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PUSH_CONSTANT_BANK_PROPERTIES_NV, p_vk14Prop};
    struct VkPhysicalDeviceShaderSMBuiltinsPropertiesNV smProp = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_SM_BUILTINS_PROPERTIES_NV, &pcBankProp};
    p_prop->pNext = &smProp;
    vkGetPhysicalDeviceProperties2(p_context->physicalDevices[0], p_prop);
    p_context->maxNumPushConstBanks                   = pcBankProp.maxComputePushConstantBanks;
    p_context->maxNumPushDataBanks                    = pcBankProp.maxComputePushDataBanks;
    p_context->numStreamingMultiprocessors            = smProp.shaderSMCount;
    p_context->numSubgroupsPerStreamingMultiprocessor = smProp.shaderWarpsPerSM;
}

/**
 * @brief 
 * 
 * @param p_context
 */
static inline void
contextInitGetProperties(struct CcbContext* const p_context)
{
    // Get physical device vender
    struct VkPhysicalDeviceProperties2 prop = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2, nullptr};
    vkGetPhysicalDeviceProperties2(p_context->physicalDevices[0], &prop);
    p_context->vendorID = prop.properties.vendorID;

    // Get physical device other properties
    struct VkPhysicalDeviceVulkan11Properties vk11Prop = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_PROPERTIES, nullptr};
    struct VkPhysicalDeviceVulkan12Properties vk12Prop = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_PROPERTIES, &vk11Prop};
    struct VkPhysicalDeviceVulkan13Properties vk13Prop = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_PROPERTIES, &vk12Prop};
    struct VkPhysicalDeviceVulkan14Properties vk14Prop = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_4_PROPERTIES, &vk13Prop};
    switch (p_context->vendorID) {
        case DEVICE_VENDOR_AMD:
            vkGetPhysicalDeviceProperties2(p_context->physicalDevices[0], &prop);
            break;
        case DEVICE_VENDOR_INTEL:
            vkGetPhysicalDeviceProperties2(p_context->physicalDevices[0], &prop);
            break;
        case DEVICE_VENDOR_NV:
            contextInitGetPropertiesNV(p_context, &prop, &vk14Prop);
            break;
        default:
            vkGetPhysicalDeviceProperties2(p_context->physicalDevices[0], &prop);
    }
    memcpy(p_context->deviceName, prop.properties.deviceName, VK_MAX_PHYSICAL_DEVICE_NAME_SIZE * sizeof(char));
    memcpy(p_context->driverName, vk12Prop.driverName, VK_MAX_DRIVER_NAME_SIZE * sizeof(char));
    memcpy(p_context->driverInfo, vk12Prop.driverInfo, VK_MAX_DRIVER_INFO_SIZE * sizeof(char));
    p_context->vulkanVersion                = prop.properties.apiVersion;
    p_context->maxPushConstSize             = prop.properties.limits.maxPushConstantsSize;
    p_context->maxUniformBufferSize         = prop.properties.limits.maxUniformBufferRange;
    p_context->maxWorkgroupMemorySize       = prop.properties.limits.maxComputeSharedMemorySize;
    p_context->maxMallocSize                = vk11Prop.maxMemoryAllocationSize;
    p_context->minNumInvocationsPerSubgroup = vk13Prop.minSubgroupSize;
    p_context->maxNumInvocationsPerSubgroup = vk13Prop.maxSubgroupSize;
}

inline int
ccbContextInit(struct CcbContext* const p_context,
               const VkInstance         p_instance,
               const uint32_t           p_deviceGroupIndex)
{
    // Invalidate
    p_context->device            = VK_NULL_HANDLE;
    p_context->timelineSemaphore = VK_NULL_HANDLE;

    CCB_RETURN_ON_FAIL(contextInitPickDeviceGroup(p_context, p_instance, p_deviceGroupIndex));
    CCB_RETURN_ON_FAIL(contextInitLocateQueueFamilyIndices(p_context));

    const float priority = 1.0f;
    struct VkDeviceQueueCreateInfo queueInfos[2] = {
        {
            .sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
            .pNext            = nullptr,
            .flags            = 0,
            .queueFamilyIndex = p_context->transferQueueFamilyIndex,
            .queueCount       = 1,
            .pQueuePriorities = &priority
        },
        {
            .sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
            .pNext            = nullptr,
            .flags            = 0,
            .queueFamilyIndex = p_context->computeQueueFamilyIndex,
            .queueCount       = 1,
            .pQueuePriorities = &priority
        }
    };
    struct VkDeviceGroupDeviceCreateInfo deviceGroupInfo = {
        .sType                = VK_STRUCTURE_TYPE_DEVICE_GROUP_DEVICE_CREATE_INFO,
        .pNext                = nullptr,
        .physicalDeviceCount  = p_context->numPhysicalDevices,
        .pPhysicalDevices     = p_context->physicalDevices
    };
    struct VkDeviceCreateInfo deviceInfo = {
        .sType                = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .pNext                = &deviceGroupInfo,
        .flags                = 0,
        .queueCreateInfoCount = 2,
        .pQueueCreateInfos    = queueInfos
    };
    VK_CHECK(vkCreateDevice(p_context->physicalDevices[0], &deviceInfo, nullptr, &p_context->device));
    volkLoadDevice(p_context->device);

    contextInitGetProperties(p_context);
    contextInitCreateTimelineSemaphore(p_context);

    CCB_FREE(s_extensions);
    return CCB_SUCCESS;
}

inline void
ccbContextDestroy(struct CcbContext* const p_context)
{
    if (p_context->hostVisibleMemory != VK_NULL_HANDLE) {
        ccbMemoryFree(p_context);
    }
    if (p_context->timelineSemaphore != VK_NULL_HANDLE) {
        vkDestroySemaphore(p_context->device, p_context->timelineSemaphore, nullptr);
        p_context->timelineSemaphore = VK_NULL_HANDLE;
    }
    if (p_context->device != VK_NULL_HANDLE) {
        vkDestroyDevice(p_context->device, nullptr);
        p_context->device = VK_NULL_HANDLE;
    }
}

inline void
ccbContextPrint(const struct CcbContext* const p_context,
                FILE*                          p_fp)
{
    fprintf(p_fp,
            "Device: (%s)x%u\n"
            "Driver: %s %s\n"
            "Vulkan Version: %u.%u.%u\n"
            "Max Push Constant Size: 0x%x\n"
            "Max Uniform Buffer Size: 0x%x\n"
            "Max Workgroup Memory Size: 0x%x\n"
            "Max Memory Allocation Size: 0x%llx\n"
            "Min Number of Invocations per Subgroup: %u\n"
            "Max Number of Invocations per Subgroup: %u\n"
            "Transfer Queue Family Index: %u\n"
            "Compute Queue Family Index: %u\n",
            p_context->deviceName, p_context->numPhysicalDevices,
            p_context->driverName, p_context->driverInfo,
            VK_API_VERSION_MAJOR(p_context->vulkanVersion),
            VK_API_VERSION_MINOR(p_context->vulkanVersion),
            VK_API_VERSION_PATCH(p_context->vulkanVersion),
            p_context->maxPushConstSize,
            p_context->maxUniformBufferSize,
            p_context->maxWorkgroupMemorySize,
            p_context->maxMallocSize,
            p_context->minNumInvocationsPerSubgroup,
            p_context->maxNumInvocationsPerSubgroup,
            p_context->transferQueueFamilyIndex,
            p_context->computeQueueFamilyIndex);
    switch (p_context->vendorID) {
        case DEVICE_VENDOR_AMD:
            break;
        case DEVICE_VENDOR_INTEL:
            break;
        case DEVICE_VENDOR_NV:
            fprintf(p_fp,
                    "Max Number of Push Constant Banks: %u\n"
                    "Max Number of Push Data Banks: %u\n"
                    "Number of Streaming Multiprocessors: %u\n"
                    "Number of Subgroups per Streaming Multiprocessor: %u\n",
                    p_context->maxNumPushConstBanks,
                    p_context->maxNumPushDataBanks,
                    p_context->numStreamingMultiprocessors,
                    p_context->numSubgroupsPerStreamingMultiprocessor);
            break;
    }
}
