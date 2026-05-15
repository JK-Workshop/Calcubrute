// Copyright (c) JK Workshop - All rights reserved

#include <JK/Calcubrute/Context.h>

constexpr uint32_t DEVICE_VENDOR_AMD   = 0x1002u;
constexpr uint32_t DEVICE_VENDOR_INTEL = 0x8086u;
constexpr uint32_t DEVICE_VENDOR_NV    = 0x10DEu;
//constexpr uint32_t DEVICE_VENDER_ARM   =

static uint32_t                      s_numExtensions;
static struct VkExtensionProperties* s_extensions = nullptr;

char CcbErrorMessage[CCB_MAX_ERROR_MESSAGE_LENGTH] = "no error";

/**
 * @brief Retrieve the whole extension list from the specified p_physicalDevice
 * 
 * @param p_context CCBContext to query on
 * @return int CCB_SUCCESS on success, CCB_MALLOC_ERROR on failure
 */
static inline int
contextInitCheckExtensions(struct CCBContext* const p_context)
{
    // Query for extensions
    vkEnumerateDeviceExtensionProperties(p_context->physicalDevices[0], nullptr, &s_numExtensions, nullptr);

    // To be freed inside ccbContextInit
    s_extensions = malloc(s_numExtensions * sizeof(struct VkExtensionProperties));
    if (s_extensions == nullptr) {
        sprintf(CcbErrorMessage, "failed to allocate memory for extensions");
        return -1;
    }

    vkEnumerateDeviceExtensionProperties(p_context->physicalDevices[0], nullptr, &s_numExtensions, s_extensions);

    // Check extensions and validate corresponding fields in CCBContext to any nonzero values, one field per extension
    for (uint32_t i = 0u; i < s_numExtensions; ++i) {
        if (strcmp(VK_NV_PUSH_CONSTANT_BANK_EXTENSION_NAME, s_extensions[i].extensionName) == 0) {
            p_context->maxNumPushConstBanks = 1u;
        }
        if (strcmp(VK_NV_SHADER_SM_BUILTINS_EXTENSION_NAME, s_extensions[i].extensionName) == 0) {
            p_context->numStreamingMultiprocessors = 1u;
        }
    }
    return 0;
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
contextInitPickDeviceGroup(struct CCBContext* const p_context,
                           const VkInstance         p_instance,
                           const uint32_t           p_deviceGroupIndex)
{
    // List and check device group properties
    uint32_t n;
    vkEnumeratePhysicalDeviceGroups(p_instance, &n, nullptr);

    if (p_deviceGroupIndex >= n) {
        sprintf(CcbErrorMessage, "invalid device group index");
        return -1;
    }

    struct VkPhysicalDeviceGroupProperties* p;
    p = malloc(n * sizeof(struct VkPhysicalDeviceGroupProperties));
    if (p == nullptr) {
        sprintf(CcbErrorMessage, "failed to allocate memory for device group properties");
        return -1;
    }

    for (uint32_t i = 0u; i < n; ++i) {
        p[i].sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_GROUP_PROPERTIES;
        p[i].pNext = nullptr;
    }

    vkEnumeratePhysicalDeviceGroups(p_instance, &n, p);

    p_context->numPhysicalDevices = p[p_deviceGroupIndex].physicalDeviceCount;
    memcpy(p_context->physicalDevices,
           p[p_deviceGroupIndex].physicalDevices,
           p_context->numPhysicalDevices * sizeof(VkPhysicalDevice));

    free(p);
    return 0;
}

/**
 * @brief 
 * 
 * @param p_context 
 * @return int 
 */
static inline int
contextInitLocateQueueFamilyIndices(struct CCBContext* const p_context)
{
    uint32_t n;
    vkGetPhysicalDeviceQueueFamilyProperties2(p_context->physicalDevices[0], &n, nullptr);

    struct VkQueueFamilyProperties2* p;
    p = malloc(n * sizeof(struct VkQueueFamilyProperties2));
    if (p == nullptr) {
        sprintf(CcbErrorMessage, "failed to allocate memory for queue family properties");
        return -1;
    }

    for (uint32_t i = 0u; i < n; ++i) {
        p[i].sType = VK_STRUCTURE_TYPE_QUEUE_FAMILY_PROPERTIES_2;
        p[i].pNext = nullptr;
    }

    vkGetPhysicalDeviceQueueFamilyProperties2(p_context->physicalDevices[0], &n, p);

    // Invalidate
    p_context->transferQueueFamilyIndex = -1;
    p_context->computeQueueFamilyIndex  = -1;

    // Ties broken by ascending order of index
    for (int i = n - 1; i >= 0; --i) {
        const uint32_t f = p[i].queueFamilyProperties.queueFlags;

        // Try locating dedicated transfer queue family
        if ((f & (VK_QUEUE_TRANSFER_BIT | VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT))
            == VK_QUEUE_TRANSFER_BIT)
        {
            p_context->transferQueueFamilyIndex = i;
        }
        // If failed, then locate any queue family with transfer capability
        else if (p_context->transferQueueFamilyIndex == -1 && (f & VK_QUEUE_TRANSFER_BIT)) {
            p_context->transferQueueFamilyIndex = i;
        }

        // Try locating dedicated compute queue family
        if ((f & (VK_QUEUE_COMPUTE_BIT | VK_QUEUE_GRAPHICS_BIT)) == VK_QUEUE_COMPUTE_BIT) {
            p_context->computeQueueFamilyIndex = i;
        }
        // If failed, then locate any queue family with compute capability
        else if (p_context->computeQueueFamilyIndex == -1 && (f & VK_QUEUE_COMPUTE_BIT)) {
            p_context->computeQueueFamilyIndex = i;
        }
    }

    // Check and return
    if (p_context->transferQueueFamilyIndex == -1) {
        sprintf(CcbErrorMessage, "failed to locate transfer queue family index");
        goto OnError;
    }
    if (p_context->computeQueueFamilyIndex == -1) {
        sprintf(CcbErrorMessage, "failed to locate compute queue family index");
        goto OnError;
    }

    return 0;

OnError:
    free(p);
    return -1;
}

/**
 * @brief 
 * 
 * @param p_context
 */
static inline int
contextInitCreateTimelineSemaphore(struct CCBContext* const p_context)
{
    int result;

    struct VkSemaphoreTypeCreateInfo typeInfo = {
        .sType         = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO,
        .pNext         = nullptr,
        .semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE,
        .initialValue  = 0ull
    };
    struct VkSemaphoreCreateInfo info = {
        .sType         = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
        .pNext         = &typeInfo,
        .flags         = 0u
    };

    result = vkCreateSemaphore(p_context->device, &info, nullptr, &p_context->timelineSemaphore);
    if (result != VK_SUCCESS) {
        p_context->timelineSemaphore = VK_NULL_HANDLE;
        sprintf(CcbErrorMessage, "failed to create timeline semaphore with VkResult %i", result);
        return -1;
    }

    return 0;
}

/**
 * @brief 
 * 
 * @param p_context 
 */
static inline void
contextInitGetTensorCoreProperties(struct CCBContext* const p_context)
{
    bool hasCoopMatKHR = false;
    bool hasCoopMat2NV = false;
    bool hasCoopVecNV  = false;

    uint32_t numProps;
    if (hasCoopMat2NV) {
        //VkCooperativeMatrix2PropertiesNV
    }
    struct VkCooperativeMatrixPropertiesKHR coopMatProps[64];
    vkGetPhysicalDeviceCooperativeMatrixPropertiesKHR(p_context->physicalDevices[0], &numProps, nullptr);
    vkGetPhysicalDeviceCooperativeMatrixPropertiesKHR(p_context->physicalDevices[0], &numProps, coopMatProps);
}

static inline void
contextInitGetPropertiesNV(struct CCBContext* const     p_context,
                           VkPhysicalDeviceProperties2* p_prop,
                           void*                        p_pLast)
{
    struct VkPhysicalDevicePushConstantBankPropertiesNV pcBank
        = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PUSH_CONSTANT_BANK_PROPERTIES_NV};
    struct VkPhysicalDeviceShaderSMBuiltinsPropertiesNV sm
        = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_SM_BUILTINS_PROPERTIES_NV};
    
    // Chain supported structs
    if (p_context->maxNumPushConstBanks) {
        pcBank.pNext = p_pLast;
        p_pLast = &pcBank;
    }
    if (p_context->numStreamingMultiprocessors) {
        sm.pNext = p_pLast;
        p_pLast = &sm;
    }
    p_prop->pNext = p_pLast;

    vkGetPhysicalDeviceProperties2(p_context->physicalDevices[0], p_prop);
    p_context->maxNumPushConstBanks                   = pcBank.maxComputePushConstantBanks;
    p_context->maxNumPushDataBanks                    = pcBank.maxComputePushDataBanks;
    p_context->numStreamingMultiprocessors            = sm.shaderSMCount;
    p_context->numSubgroupsPerStreamingMultiprocessor = sm.shaderWarpsPerSM;
}

/**
 * @brief 
 * 
 * @param p_context
 */
static inline void
contextInitGetProperties(struct CCBContext* const p_context)
{
    // Get physical device vender
    struct VkPhysicalDeviceProperties2 p
        = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2, nullptr};
    vkGetPhysicalDeviceProperties2(p_context->physicalDevices[0], &p);
    p_context->vendorID = p.properties.vendorID;

    // Get physical device other properties
    struct VkPhysicalDeviceVulkan11Properties vk11
        = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_PROPERTIES, nullptr};
    struct VkPhysicalDeviceVulkan12Properties vk12
        = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_PROPERTIES, &vk11};
    struct VkPhysicalDeviceVulkan13Properties vk13
        = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_PROPERTIES, &vk12};
    struct VkPhysicalDeviceVulkan14Properties vk14
        = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_4_PROPERTIES, &vk13};

    switch (p_context->vendorID) {
        case DEVICE_VENDOR_AMD:
            p.pNext = &vk14;
            vkGetPhysicalDeviceProperties2(p_context->physicalDevices[0], &p);
            break;
        case DEVICE_VENDOR_INTEL:
            p.pNext = &vk14;
            vkGetPhysicalDeviceProperties2(p_context->physicalDevices[0], &p);
            break;
        case DEVICE_VENDOR_NV:
            contextInitGetPropertiesNV(p_context, &p, &vk14);
            break;
        default:
            p.pNext = &vk14;
            vkGetPhysicalDeviceProperties2(p_context->physicalDevices[0], &p);
    }

    memcpy(p_context->deviceName, p.properties.deviceName, VK_MAX_PHYSICAL_DEVICE_NAME_SIZE * sizeof(char));
    memcpy(p_context->driverName, vk12.driverName, VK_MAX_DRIVER_NAME_SIZE * sizeof(char));
    memcpy(p_context->driverInfo, vk12.driverInfo, VK_MAX_DRIVER_INFO_SIZE * sizeof(char));
    p_context->vulkanVersion                = p.properties.apiVersion;
    p_context->maxPushConstSize             = p.properties.limits.maxPushConstantsSize;
    p_context->maxUniformBufferSize         = p.properties.limits.maxUniformBufferRange;
    p_context->maxWorkgroupMemorySize       = p.properties.limits.maxComputeSharedMemorySize;
    p_context->maxMallocSize                = vk11.maxMemoryAllocationSize;
    p_context->minNumInvocationsPerSubgroup = vk13.minSubgroupSize;
    p_context->maxNumInvocationsPerSubgroup = vk13.maxSubgroupSize;
}

inline int
ccbContextInit(struct CCBContext* const p_context,
               const VkInstance         p_instance,
               const uint32_t           p_deviceGroupIndex)
{
    int result;
    p_context->maxNumPushConstBanks        = 0u; // VK_NV_push_constant_bank
    p_context->numStreamingMultiprocessors = 0u; // VK_NV_shader_sm_builtins

    result = contextInitPickDeviceGroup(p_context, p_instance, p_deviceGroupIndex);
    if (result != 0) {
        return -1;
    }
    result = contextInitCheckExtensions(p_context);
    if (result != 0) {
        goto OnPreCreateDeviceError;
    }
    result = contextInitLocateQueueFamilyIndices(p_context);
    if (result != 0) {
        goto OnPreCreateDeviceError;
    }
    contextInitGetProperties(p_context);

    // Create the logical device
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
    
    // Extensions and features
    const char* extensionNames[] = {
        VK_KHR_DEVICE_ADDRESS_COMMANDS_EXTENSION_NAME
    };
    struct VkPhysicalDeviceDeviceAddressCommandsFeaturesKHR deviceAddressFeat
        = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DEVICE_ADDRESS_COMMANDS_FEATURES_KHR,
           nullptr, VK_TRUE};
    struct VkPhysicalDeviceFeatures2 feat = {
        .sType    = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
        .pNext    = &deviceAddressFeat,
        .features = {0}
    };

    struct VkDeviceGroupDeviceCreateInfo deviceGroupInfo = {
        .sType                = VK_STRUCTURE_TYPE_DEVICE_GROUP_DEVICE_CREATE_INFO,
        .pNext                = &feat,
        .physicalDeviceCount  = p_context->numPhysicalDevices,
        .pPhysicalDevices     = p_context->physicalDevices
    };
    struct VkDeviceCreateInfo info = {
        .sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .pNext                   = &deviceGroupInfo,
        .flags                   = 0,
        .queueCreateInfoCount    = 2,
        .pQueueCreateInfos       = queueInfos,
        .enabledExtensionCount   = 1u,
        .ppEnabledExtensionNames = extensionNames,
        .pEnabledFeatures        = nullptr
    };
    
    result = vkCreateDevice(p_context->physicalDevices[0], &info, nullptr, &p_context->device);
    if (result != VK_SUCCESS) {
        sprintf(CcbErrorMessage, "failed to create logical device with VkResult %i", result);
        goto OnCreateDeviceError;
    }
    volkLoadDevice(p_context->device);

    // Retrieve transfer and compute queues
    struct VkDeviceQueueInfo2 deviceQueueInfo = {
        .sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_INFO_2,
        .pNext            = nullptr,
        .flags            = 0u,
        .queueFamilyIndex = p_context->transferQueueFamilyIndex,
        .queueIndex       = 0u
    };
    vkGetDeviceQueue2(p_context->device, &deviceQueueInfo, &p_context->transferQueue);
    deviceQueueInfo.queueFamilyIndex = p_context->computeQueueFamilyIndex;
    vkGetDeviceQueue2(p_context->device, &deviceQueueInfo, &p_context->computeQueue);

    result = contextInitCreateTimelineSemaphore(p_context);
    if (result != VK_SUCCESS) {
        goto OnCreateSemaphoreError;
    }

    uint64_t value;
    result = vkGetSemaphoreCounterValue(p_context->device, p_context->timelineSemaphore, &value);
    if (result != VK_SUCCESS) {
        sprintf(CcbErrorMessage, "failed to get timeline semaphore value");
        goto OnGetSemaphoreCounterValueError;
    }

    printf("[Calcubrute Info] Timeline semaphore initialized to %llu\n", value);

    return 0;

OnGetSemaphoreCounterValueError:
    vkDestroySemaphore(p_context->device, p_context->timelineSemaphore, nullptr);

OnCreateSemaphoreError:
    vkDestroyDevice(p_context->device, nullptr);

OnCreateDeviceError:
    p_context->device = VK_NULL_HANDLE;

OnPreCreateDeviceError:
    free(s_extensions);
    return -1;
}

inline void
ccbContextDestroy(struct CCBContext* const p_context)
{
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
ccbContextPrint(const struct CCBContext* const p_context,
                FILE*                          p_fp)
{
    fprintf(p_fp, "Device: (%s)x%u\n"
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
            // VK_NV_push_constant_bank properties
            if (p_context->maxNumPushConstBanks) {
                fprintf(p_fp, "Max Number of Push Constant Banks: %u\n"
                              "Max Number of Push Data Banks: %u\n",
                              p_context->maxNumPushConstBanks,
                              p_context->maxNumPushDataBanks);
            }
            else {
                fputs("Max Number of Push Constant Banks: -\n"
                      "Max Number of Push Data Banks: -\n", p_fp);
            }

            // VK_NV_shader_sm_builtins properties
            if (p_context->numStreamingMultiprocessors) {
                fprintf(p_fp, "Number of Streaming Multiprocessors: %u\n"
                              "Number of Subgroups per Streaming Multiprocessor: %u\n",
                              p_context->numStreamingMultiprocessors,
                              p_context->numSubgroupsPerStreamingMultiprocessor);
            }
            else {
                fputs("Number of Streaming Multiprocessors: -\n"
                      "Number of Subgroups per Streaming Multiprocessor: -\n", p_fp);
            }

            break;
    }
}
