// Copyright (c) JK Workshop - All rights reserved

#include <JK/Calcubrute/Context.h>
#include <JK/Calcubrute/Memory.h>

static VkBuffer s_hostVisibleBuffer = VK_NULL_HANDLE;
static VkBuffer s_deviceLocalBuffer = VK_NULL_HANDLE;

static inline int
memoryAllocateLocateTypeIndices(struct CcbContext* const p_context,
                                uint32_t* const          p_hostVisibleIndex,
                                uint32_t* const          p_deviceLocalIndex)
{
    // Invalidate
    *p_hostVisibleIndex = UINT32_MAX;
    *p_deviceLocalIndex = UINT32_MAX;

    // Query memory properties
    struct VkPhysicalDeviceMemoryProperties2 prop = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_PROPERTIES_2, nullptr};
    vkGetPhysicalDeviceMemoryProperties2(p_context->physicalDevices[0], &prop);

    // Iteratively check memory flags to retrieve indices, ties broken by smaller index first
    constexpr uint32_t deviceLocalFlag = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    constexpr uint32_t hostVisibleFlag = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    for (int i = prop.memoryProperties.memoryTypeCount - 1; i >= 0; --i) {
        if ((prop.memoryProperties.memoryTypes[i].propertyFlags & deviceLocalFlag) == deviceLocalFlag) {
            *p_deviceLocalIndex = i;
        }
        if ((prop.memoryProperties.memoryTypes[i].propertyFlags & hostVisibleFlag) == hostVisibleFlag) {
            *p_hostVisibleIndex = i;
        }
    }

    // Check and return
    if (*p_hostVisibleIndex == UINT32_MAX) {
        _CCB_LOG_ERROR("[Calcubrute Error] Failed to locate host visible memory type index\n");
        return CCB_MALLOC_ERROR;
    }
    if (*p_deviceLocalIndex == UINT32_MAX) {
        _CCB_LOG_ERROR("[Calcubrute Error] Failed to locate device local memory type index\n");
        return CCB_MALLOC_ERROR;
    }

    return CCB_SUCCESS;
}

static inline int
memoryAllocateCreateBuffers(struct CcbContext* const p_context,
                            uint64_t                 p_hostVisibleSize,
                            uint64_t                 p_deviceLocalSize)
{
    VkResult result;

    // Set up host visible buffer creation infos
    struct VkBufferUsageFlags2CreateInfo bufUsageInfo = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_USAGE_FLAGS_2_CREATE_INFO,
        .pNext = nullptr,
        .usage = VK_BUFFER_USAGE_2_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_2_TRANSFER_DST_BIT | VK_BUFFER_USAGE_2_SHADER_DEVICE_ADDRESS_BIT
    };
    struct VkBufferCreateInfo bufInfo = {
        .sType                 = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .pNext                 = &bufUsageInfo,
        .flags                 = 0u,
        .size                  = p_hostVisibleSize,
        .usage                 = 0u,
        .sharingMode           = VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = 0u,
        .pQueueFamilyIndices   = nullptr
    };
    const struct VkDeviceBufferMemoryRequirements reqInfo = {
        .sType       = VK_STRUCTURE_TYPE_DEVICE_BUFFER_MEMORY_REQUIREMENTS,
        .pNext       = nullptr,
        .pCreateInfo = &bufInfo
    };
    struct VkMemoryRequirements2 req = {VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2, nullptr};

    // Create host visible buffer
    vkGetDeviceBufferMemoryRequirements(p_context->device, &reqInfo, &req);
    p_hostVisibleSize = bufInfo.size = req.memoryRequirements.size;
    result = vkCreateBuffer(p_context->device, &bufInfo, nullptr, &s_hostVisibleBuffer);
    if (result != VK_SUCCESS) {
        _CCB_LOG_ERROR("[Calcubrute Error] Host visible buffer creation failed with VkResult %i\n", result);
        s_hostVisibleBuffer = VK_NULL_HANDLE;
        return CCB_MALLOC_ERROR;
    }

    // Set up device local buffer creation infos
    bufInfo.size = p_deviceLocalSize;
    bufUsageInfo.usage |= VK_BUFFER_USAGE_2_STORAGE_BUFFER_BIT;

    // Create device local buffer
    vkGetDeviceBufferMemoryRequirements(p_context->device, &reqInfo, &req);
    p_deviceLocalSize = bufInfo.size = req.memoryRequirements.size;
    result = vkCreateBuffer(p_context->device, &bufInfo, nullptr, &s_deviceLocalBuffer);
    if (result != VK_SUCCESS) {
        _CCB_LOG_ERROR("[Calcubrute Error] Device local buffer creation failed with VkResult %i\n", result);
        vkDestroyBuffer(p_context->device, s_hostVisibleBuffer, nullptr);
        s_hostVisibleBuffer = VK_NULL_HANDLE;
        s_deviceLocalBuffer = VK_NULL_HANDLE;
        return CCB_MALLOC_ERROR;
    }

    return CCB_SUCCESS;
}

static inline int
memoryAllocateMalloc(struct CcbContext* const       p_context,
                     const uint64_t                 p_hostVisibleSize,
                     const uint64_t                 p_deviceLocalSize,
                     const uint32_t                 p_hostVisibleIndex,
                     const uint32_t                 p_deviceLocalIndex)
{
    VkResult result;

    // Set up host visible memory allocation infos
    const struct VkMemoryAllocateFlagsInfo mallocFlagsInfo = {
        .sType      = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO,
        .pNext      = nullptr,
        .flags      = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT,
        .deviceMask = 0u
    };
    struct VkMemoryAllocateInfo mallocInfo = {
        .sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .pNext           = &mallocFlagsInfo,
        .allocationSize  = p_hostVisibleSize,
        .memoryTypeIndex = p_hostVisibleIndex
    };

    // Allocate host visible memory
    result = vkAllocateMemory(p_context->device, &mallocInfo, nullptr, &p_context->hostVisibleMemory);
    if (result != VK_SUCCESS) {
        _CCB_LOG_ERROR("[Calcubrute Error] Host visible memory allocation failed with VkResult %i\n", result);
        p_context->hostVisibleMemory = VK_NULL_HANDLE;
        return CCB_MALLOC_ERROR;
    }

    // Set up device local memory allocation infos
    mallocInfo.allocationSize  = p_deviceLocalSize;
    mallocInfo.memoryTypeIndex = p_deviceLocalIndex;

    // Allocate device local memory
    result = vkAllocateMemory(p_context->device, &mallocInfo, nullptr, &p_context->deviceLocalMemory);
    if (result != VK_SUCCESS) {
        _CCB_LOG_ERROR("[Calcubrute Error] Device local memory allocation failed with VkResult %i\n", result);
        vkFreeMemory(p_context->device, p_context->hostVisibleMemory, nullptr);
        p_context->hostVisibleMemory = VK_NULL_HANDLE;
        p_context->deviceLocalMemory = VK_NULL_HANDLE;
        return CCB_MALLOC_ERROR;
    }

    return CCB_SUCCESS;
}

static inline int
memoryAllocateMmap(struct CcbContext* const p_context)
{
    VkResult result;

    const struct VkMemoryMapInfo mmapInfo = {
        .sType  = VK_STRUCTURE_TYPE_MEMORY_MAP_INFO,
        .pNext  = nullptr,
        .flags  = 0u,
        .memory = p_context->hostVisibleMemory,
        .offset = 0ull,
        .size   = VK_WHOLE_SIZE
    };

    result = vkMapMemory2(p_context->device, &mmapInfo, (void**)&p_context->hostVisibleHostBase);
    if (result != VK_SUCCESS) {
        _CCB_LOG_ERROR("[Calcubrute Error] Host visible memory map failed with VkResult %i\n", result);
        p_context->hostVisibleHostBase = nullptr;
        return CCB_MALLOC_ERROR;
    }

    return CCB_SUCCESS;
}

static inline int
memoryAllocateBindBuffers(struct CcbContext* const p_context)
{
    VkResult result;

    struct VkBindBufferMemoryInfo bindBufInfos[2] = {
        {
            .sType        = VK_STRUCTURE_TYPE_BIND_BUFFER_MEMORY_INFO,
            .pNext        = nullptr,
            .buffer       = s_hostVisibleBuffer,
            .memory       = p_context->hostVisibleMemory,
            .memoryOffset = 0ull
        },
        {
            .sType        = VK_STRUCTURE_TYPE_BIND_BUFFER_MEMORY_INFO,
            .pNext        = nullptr,
            .buffer       = s_deviceLocalBuffer,
            .memory       = p_context->deviceLocalMemory,
            .memoryOffset = 0ull
        }
    };

    result = vkBindBufferMemory2(p_context->device, 2u, bindBufInfos);
    if (result != VK_SUCCESS) {
        _CCB_LOG_ERROR("[Calcubrute Error] Buffer binding failed with VkResult %i\n", result);
        return CCB_MALLOC_ERROR;
    }

    return CCB_SUCCESS;
}

static inline void
memoryAllocateRetriveAddresses(struct CcbContext* const p_context)
{
    struct VkBufferDeviceAddressInfo info = {
        .sType  = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
        .pNext  = nullptr,
        .buffer = s_hostVisibleBuffer
    };
    p_context->hostVisibleDeviceBase = vkGetBufferDeviceAddress(p_context->device, &info);
    info.buffer = s_deviceLocalBuffer;
    p_context->deviceLocalDeviceBase = vkGetBufferDeviceAddress(p_context->device, &info);
}

inline int
ccbMemoryAllocate(struct CcbContext* const p_context,
                  const uint64_t           p_hostVisibleSize,
                  const uint64_t           p_deviceLocalSize)
{
    int result;

    if (p_hostVisibleSize < p_deviceLocalSize) {
        _CCB_LOG_ERROR("[Calcubrute Error] Host visible size must be at least device local size\n");
        return CCB_ARGUMENT_ERROR;
    }

    uint32_t hostVisibleIndex;
    uint32_t deviceLocalIndex;
    result = memoryAllocateLocateTypeIndices(p_context, &hostVisibleIndex, &deviceLocalIndex);
    if (result != CCB_SUCCESS) {
        goto OnPreMallocError;
    }

    _CCB_LOG_INFO("[Calcubrute Info] Host visible index = %u\n", hostVisibleIndex);
    _CCB_LOG_INFO("[Calcubrute Info] Device local index = %u\n", deviceLocalIndex);

    result = memoryAllocateCreateBuffers(p_context, p_hostVisibleSize, p_deviceLocalSize);
    if (result != CCB_SUCCESS) {
        goto OnPreMallocError;
    }

    _CCB_LOG_INFO("[Calcubrute Info] Host visible size = 0x%llx\n", p_hostVisibleSize);
    _CCB_LOG_INFO("[Calcubrute Info] Device local size = 0x%llx\n", p_deviceLocalSize);

    result = memoryAllocateMalloc(p_context, p_hostVisibleSize, p_deviceLocalSize, hostVisibleIndex, deviceLocalIndex);
    if (result != CCB_SUCCESS) {
        goto OnMallocError;
    }

    result = memoryAllocateMmap(p_context);
    if (result != CCB_SUCCESS) {
        goto OnMmapError;
    }

    _CCB_LOG_INFO("[Calcubrute Info] Mapped host visible memory to 0x%p\n", p_context->hostVisibleHostBase);

    result = memoryAllocateBindBuffers(p_context);
    if (result != CCB_SUCCESS) {
        goto OnBindBufferError;
    }

    memoryAllocateRetriveAddresses(p_context);

    _CCB_LOG_INFO("[Calcubrute Info] Host visible memory starts at device address 0x%llx\n", p_context->hostVisibleDeviceBase);
    _CCB_LOG_INFO("[Calcubrute Info] Device local memory starts at device address 0x%llx\n", p_context->deviceLocalDeviceBase);

    return CCB_SUCCESS;

OnBindBufferError:
    const struct VkMemoryUnmapInfo munmapInfo = {
        .sType  = VK_STRUCTURE_TYPE_MEMORY_UNMAP_INFO,
        .pNext  = nullptr,
        .flags  = 0u,
        .memory = p_context->hostVisibleMemory
    };
    vkUnmapMemory2(p_context->device, &munmapInfo);
OnMmapError:
    vkFreeMemory(p_context->device, p_context->hostVisibleMemory, nullptr);
    p_context->hostVisibleMemory = VK_NULL_HANDLE;
    vkFreeMemory(p_context->device, p_context->deviceLocalMemory, nullptr);
    p_context->deviceLocalMemory = VK_NULL_HANDLE;
OnMallocError:
    vkDestroyBuffer(p_context->device, s_hostVisibleBuffer, nullptr);
    s_hostVisibleBuffer = VK_NULL_HANDLE;
    vkDestroyBuffer(p_context->device, s_deviceLocalBuffer, nullptr);
    s_deviceLocalBuffer = VK_NULL_HANDLE;
OnPreMallocError:
    return result;
}

inline void
ccbMemoryFree(struct CcbContext* const p_context)
{
    VkResult result;

    // Destory host visible buffer
    if (s_hostVisibleBuffer != VK_NULL_HANDLE) {
        vkDestroyBuffer(p_context->device, s_hostVisibleBuffer, nullptr);
        s_hostVisibleBuffer = VK_NULL_HANDLE;
    }

    // Destroy device local buffer
    if (s_deviceLocalBuffer != VK_NULL_HANDLE) {
        vkDestroyBuffer(p_context->device, s_deviceLocalBuffer, nullptr);
        s_deviceLocalBuffer = VK_NULL_HANDLE;
    }

    // Free host visible memory
    if (p_context->hostVisibleMemory != VK_NULL_HANDLE) {
        // Unmap before freeing
        if (p_context->hostVisibleHostBase != nullptr) {
            const struct VkMemoryUnmapInfo munmapInfo = {
                .sType  = VK_STRUCTURE_TYPE_MEMORY_UNMAP_INFO,
                .pNext  = nullptr,
                .flags  = 0u,
                .memory = p_context->hostVisibleMemory
            };
            result = vkUnmapMemory2(p_context->device, &munmapInfo);
            if (result != VK_SUCCESS) {
                _CCB_LOG_ERROR("[Calcubrute Error] Host visible memory unmap failed with VkResult %i\n", result);
            }
            p_context->hostVisibleHostBase = nullptr;
        }
        vkFreeMemory(p_context->device, p_context->hostVisibleMemory, nullptr);
        p_context->hostVisibleMemory = VK_NULL_HANDLE;
    }

    // Free device local memory
    if (p_context->deviceLocalMemory != VK_NULL_HANDLE) {
        vkFreeMemory(p_context->device, p_context->deviceLocalMemory, nullptr);
        p_context->deviceLocalMemory = VK_NULL_HANDLE;
    }
}
