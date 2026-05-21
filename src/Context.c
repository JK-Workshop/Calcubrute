// Copyright (c) JK Workshop - All rights reserved

#include <JK/Calcubrute/Context.h>
#include "DefaultFeatures.h"

constexpr uint32_t DEVICE_VENDOR_AMD   = 0x1002u;
constexpr uint32_t DEVICE_VENDOR_INTEL = 0x8086u;
constexpr uint32_t DEVICE_VENDOR_NV    = 0x10DEu;
//constexpr uint32_t DEVICE_VENDER_ARM   =

static uint32_t                      s_numExts;
static struct VkExtensionProperties* s_exts = nullptr;

char CcbErrMsg[CCB_MAX_ERROR_MESSAGE_LENGTH] = "no error";

/**
 * @brief Retrieve the whole extension list from the specified p_physicalDevice
 * 
 * @param p_context CCBContext to query on
 * @return int CCB_SUCCESS on success, CCB_MALLOC_ERROR on failure
 */
static inline int
contextInitCheckExtensions(struct CCBContext* const p_context)
{
    // Query for features
    vkGetPhysicalDeviceFeatures2(p_context->physicalDevices[0], &s_feat);

    // Query for extensions
    vkEnumerateDeviceExtensionProperties(p_context->physicalDevices[0], nullptr, &s_numExts, nullptr);
    s_exts = malloc(s_numExts * sizeof(struct VkExtensionProperties)); // to be freed inside ccbContextInit
    if (s_exts == nullptr) {
        sprintf(CcbErrMsg, "failed to allocate memory for extensions");
        return -1;
    }
    vkEnumerateDeviceExtensionProperties(p_context->physicalDevices[0], nullptr, &s_numExts, s_exts);

    // Check extensions and validate corresponding fields in CCBContext to any nonzero values, one field per extension
    for (uint32_t i = 0u; i < s_numExts; ++i) {
        if (strcmp("VK_KHR_cooperative_matrix", s_exts[i].extensionName) == 0) {
            s_enabledExtNames[s_numEnabledExts++] = "VK_KHR_cooperative_matrix";
            s_coopMatFeat.cooperativeMatrixRobustBufferAccess = VK_FALSE;
        }
        if (strcmp("VK_KHR_device_address_commands", s_exts[i].extensionName) == 0) {
            s_enabledExtNames[s_numEnabledExts++] = "VK_KHR_device_address_commands";
        }
        // Depricated if VK_VERSION_1_4
        if (strcmp("VK_KHR_map_memory2", s_exts[i].extensionName) == 0) {
            s_enabledExtNames[s_numEnabledExts++] = "VK_KHR_map_memory2";
        }
        if (strcmp("VK_KHR_shader_fma", s_exts[i].extensionName) == 0) {
            s_enabledExtNames[s_numEnabledExts++] = "VK_KHR_shader_fma";
            s_fmaFeat.shaderFmaFloat64 = VK_FALSE;
        }
        if (strcmp("VK_NV_cooperative_matrix_2", s_exts[i].extensionName) == 0) {
            s_enabledExtNames[s_numEnabledExts++] = "VK_NV_cooperative_matrix_2";
        }
        if (strcmp("VK_NV_push_constant_bank", s_exts[i].extensionName) == 0) {
            s_enabledExtNames[s_numEnabledExts++] = "VK_NV_push_constant_bank";
        }
        if (strcmp("VK_NV_shader_sm_builtins", s_exts[i].extensionName) == 0) {
            s_enabledExtNames[s_numEnabledExts++] = "VK_NV_shader_sm_builtins";
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
        sprintf(CcbErrMsg, "invalid device group index");
        return -1;
    }

    struct VkPhysicalDeviceGroupProperties* p;
    p = malloc(n * sizeof(struct VkPhysicalDeviceGroupProperties));
    if (p == nullptr) {
        sprintf(CcbErrMsg, "failed to allocate memory for device group properties");
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
        sprintf(CcbErrMsg, "failed to allocate memory for queue family properties");
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
        sprintf(CcbErrMsg, "failed to locate transfer queue family index");
        goto OnError;
    }
    if (p_context->computeQueueFamilyIndex == -1) {
        sprintf(CcbErrMsg, "failed to locate compute queue family index");
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
        sprintf(CcbErrMsg, "failed to create timeline semaphore with VkResult %i", result);
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
    if (s_pcBankFeat.pushConstantBank == VK_TRUE) {
        pcBank.pNext = p_pLast;
        p_pLast = &pcBank;
    }
    if (s_smFeat.shaderSMBuiltins == VK_TRUE) {
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

    // The only invalidation for a CCBContext object
    p_context->device = VK_NULL_HANDLE;

    result = contextInitPickDeviceGroup(p_context, p_instance, p_deviceGroupIndex);
    if (result != 0) {
        return -1;
    }
    result = contextInitCheckExtensions(p_context);
    if (result != 0) {
        goto OnPreCreateDeviceError;
    }
    if (s_devAddrCmdFeat.deviceAddressCommands == VK_FALSE) {
        sprintf(CcbErrMsg, "VK_KHR_device_address_commands unsupproted");
        goto OnPreCreateDeviceError;
    }
    if (s_fmaFeat.shaderFmaFloat16 == VK_FALSE) {
        sprintf(CcbErrMsg, "VK_KHR_shader_fma::float16 unsupported");
        goto OnPreCreateDeviceError;
    }
    if (s_fmaFeat.shaderFmaFloat32 == VK_FALSE) {
        sprintf(CcbErrMsg, "VK_KHR_shader_fma::float32 unsupported");
        goto OnPreCreateDeviceError;
    }
    result = contextInitLocateQueueFamilyIndices(p_context);
    if (result != 0) {
        goto OnPreCreateDeviceError;
    }
    contextInitGetProperties(p_context);
    free(s_exts);

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
    struct VkDeviceGroupDeviceCreateInfo deviceGroupInfo = {
        .sType                = VK_STRUCTURE_TYPE_DEVICE_GROUP_DEVICE_CREATE_INFO,
        .pNext                = &s_feat,
        .physicalDeviceCount  = p_context->numPhysicalDevices,
        .pPhysicalDevices     = p_context->physicalDevices
    };
    struct VkDeviceCreateInfo info = {
        .sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .pNext                   = &deviceGroupInfo,
        .flags                   = 0u,
        .queueCreateInfoCount    = 2u,
        .pQueueCreateInfos       = queueInfos,
        .enabledExtensionCount   = s_numEnabledExts,
        .ppEnabledExtensionNames = s_enabledExtNames,
        .pEnabledFeatures        = nullptr
    };
    
    result = vkCreateDevice(p_context->physicalDevices[0], &info, nullptr, &p_context->device);
    if (result != VK_SUCCESS) {
        sprintf(CcbErrMsg, "failed to create logical device with VkResult %i", result);
        goto OnCreateDeviceError;
    }
    volkLoadDevice(p_context->device);

    // Retrieve compute queue
    struct VkDeviceQueueInfo2 deviceQueueInfo = {
        .sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_INFO_2,
        .pNext            = nullptr,
        .flags            = 0u,
        .queueFamilyIndex = p_context->computeQueueFamilyIndex,
        .queueIndex       = 0u
    };
    vkGetDeviceQueue2(p_context->device, &deviceQueueInfo, &p_context->computeQueue);

    result = contextInitCreateTimelineSemaphore(p_context);
    if (result != VK_SUCCESS) {
        goto OnCreateSemaphoreError;
    }

    uint64_t value;
    result = vkGetSemaphoreCounterValue(p_context->device, p_context->timelineSemaphore, &value);
    if (result != VK_SUCCESS) {
        sprintf(CcbErrMsg, "failed to get timeline semaphore value");
        goto OnGetSemaphoreCounterValueError;
    }

    printf("[Calcubrute Info] Timeline semaphore initialized to %llu\n", value);

    return 0;

OnGetSemaphoreCounterValueError:
    vkDestroySemaphore(p_context->device, p_context->timelineSemaphore, nullptr);

OnCreateSemaphoreError:
    vkDestroyDevice(p_context->device, nullptr);

OnCreateDeviceError:
    // Invalidate again, prevent future free on nullptr
    p_context->device = VK_NULL_HANDLE;

OnPreCreateDeviceError:
    free(s_exts);
    return -1;
}

inline void
ccbContextDestroy(struct CCBContext* const p_context)
{
    if (p_context->device == VK_NULL_HANDLE) {
        return;
    }

    vkDestroySemaphore(p_context->device, p_context->timelineSemaphore, nullptr);
    vkDestroyDevice(p_context->device, nullptr);
}

static inline void
contextPrintEnabledFeatures(FILE* p_fp)
{
    fprintf(p_fp, "|__ VK_KHR_cooperative_matrix\n"
                  "|   |__ cooperative matrix            %s\n"
                  "|   |__ robust buffer access          %s\n"
                  "|__ VK_KHR_device_address_commands    %s\n"
                  "|__ VK_KHR_shader_fma\n"
                  "|   |__ float16                       %s\n"
                  "|   |__ float32                       %s\n"
                  "|   |__ float64                       %s\n"
                  "|__ VK_NV_cooperative_matrix_2\n"
                  "|   |__ workgroup scope               %s\n"
                  "|   |__ flexible dimensions           %s\n"
                  "|   |__ reductions                    %s\n"
                  "|   |__ conversions                   %s\n"
                  "|   |__ per element operations        %s\n"
                  "|   |__ tensor addressing             %s\n"
                  "|   |__ block loads                   %s\n"
                  "|__ VK_NV_shader_sm_builtins          %s\n"
                  "|__ VK_NV_push_constant_bank          %s\n",
                  s_coopMatFeat.cooperativeMatrix                      ? "ON" : "OFF",
                  s_coopMatFeat.cooperativeMatrixRobustBufferAccess    ? "ON" : "OFF",
                  s_devAddrCmdFeat.deviceAddressCommands               ? "ON" : "OFF",
                  s_fmaFeat.shaderFmaFloat16                           ? "ON" : "OFF",
                  s_fmaFeat.shaderFmaFloat32                           ? "ON" : "OFF",
                  s_fmaFeat.shaderFmaFloat64                           ? "ON" : "OFF",
                  s_coopMat2Feat.cooperativeMatrixWorkgroupScope       ? "ON" : "OFF",
                  s_coopMat2Feat.cooperativeMatrixFlexibleDimensions   ? "ON" : "OFF",
                  s_coopMat2Feat.cooperativeMatrixReductions           ? "ON" : "OFF",
                  s_coopMat2Feat.cooperativeMatrixConversions          ? "ON" : "OFF",
                  s_coopMat2Feat.cooperativeMatrixPerElementOperations ? "ON" : "OFF",
                  s_coopMat2Feat.cooperativeMatrixTensorAddressing     ? "ON" : "OFF",
                  s_coopMat2Feat.cooperativeMatrixBlockLoads           ? "ON" : "OFF",
                  s_smFeat.shaderSMBuiltins                            ? "ON" : "OFF",
                  s_pcBankFeat.pushConstantBank                        ? "ON" : "OFF");
}

inline void
ccbContextPrint(const struct CCBContext* const p_context,
                FILE*                          p_fp)
{
    fprintf(p_fp, "[Calcubrute Info] Context:\n"
                  "|__ [%s %s %u.%u.%u] * %u\n",
                  p_context->deviceName, p_context->driverInfo,
                  VK_API_VERSION_MAJOR(p_context->vulkanVersion),
                  VK_API_VERSION_MINOR(p_context->vulkanVersion),
                  VK_API_VERSION_PATCH(p_context->vulkanVersion),
                  p_context->numPhysicalDevices);
    contextPrintEnabledFeatures(p_fp);
    fprintf(p_fp, "|__ Max Push Constant Size: 0x%x\n"
                  "|__ Max Uniform Buffer Size: 0x%x\n"
                  "|__ Max Workgroup Memory Size: 0x%x\n"
                  "|__ Max Memory Allocation Size: 0x%llx\n"
                  "|__ Min Number of Invocations per Subgroup: %u\n"
                  "|__ Max Number of Invocations per Subgroup: %u\n"
                  "|__ Transfer Queue Family Index: %u\n"
                  "|__ Compute Queue Family Index: %u\n",
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
            if (s_pcBankFeat.pushConstantBank == VK_TRUE) {
                fprintf(p_fp, "|__ Max Number of Push Constant Banks: %u\n"
                              "|__ Max Number of Push Data Banks: %u\n",
                              p_context->maxNumPushConstBanks,
                              p_context->maxNumPushDataBanks);
            }
            // VK_NV_shader_sm_builtins properties
            if (s_smFeat.shaderSMBuiltins == VK_TRUE) {
                fprintf(p_fp, "|__ Number of Streaming Multiprocessors: %u\n"
                              "|__ Number of Subgroups per Streaming Multiprocessor: %u\n",
                              p_context->numStreamingMultiprocessors,
                              p_context->numSubgroupsPerStreamingMultiprocessor);
            }
            break;
    }
}
