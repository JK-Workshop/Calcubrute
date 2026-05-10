// Copyright (c) JK Workshop - All rights reserved

#include <JK/Calcubrute/Context.h>

static VkBuffer s_hostVisibleBuffer = VK_NULL_HANDLE;
static VkBuffer s_deviceLocalBuffer = VK_NULL_HANDLE;

struct memoryTypeIndices
{
    uint32_t hostVisibleIndex;
    uint32_t deviceLocalIndex;
};

static inline struct memoryTypeIndices
memoryAllocateLocateTypeIndices(struct CcbContext* const p_context)
{
    struct memoryTypeIndices ids = {UINT32_MAX, UINT32_MAX};

    struct VkPhysicalDeviceMemoryProperties2 prop = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_PROPERTIES_2, nullptr};
    vkGetPhysicalDeviceMemoryProperties2(p_context->physicalDevices[0], &prop);

    // Iteratively check memory flags to retrieve indices, ties broken by smaller index first
    constexpr uint32_t deviceLocalFlag = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    constexpr uint32_t hostVisibleFlag = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
                                       | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    for (int i = prop.memoryProperties.memoryTypeCount - 1; i >= 0; --i) {
        if ((prop.memoryProperties.memoryTypes[i].propertyFlags & deviceLocalFlag) == deviceLocalFlag) {
            ids.deviceLocalIndex = i;
        }
        if ((prop.memoryProperties.memoryTypes[i].propertyFlags & hostVisibleFlag) == hostVisibleFlag) {
            ids.hostVisibleIndex = i;
        }
    }
    return ids;
}

static inline void
memoryAllocateCreateBuffers(struct CcbContext* const p_context,
                            uint64_t                 p_hostVisibleSize,
                            uint64_t                 p_deviceLocalSize)
{
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

    vkGetDeviceBufferMemoryRequirements(p_context->device, &reqInfo, &req);
    p_hostVisibleSize = bufInfo.size = req.memoryRequirements.size;
    VK_CHECK(vkCreateBuffer(p_context->device, &bufInfo, nullptr, &s_hostVisibleBuffer));

    bufInfo.size = p_deviceLocalSize;
    bufUsageInfo.usage |= VK_BUFFER_USAGE_2_STORAGE_BUFFER_BIT;

    vkGetDeviceBufferMemoryRequirements(p_context->device, &reqInfo, &req);
    p_deviceLocalSize = bufInfo.size = req.memoryRequirements.size;
    VK_CHECK(vkCreateBuffer(p_context->device, &bufInfo, nullptr, &s_deviceLocalBuffer));
}

static inline void
memoryAllocateMalloc(struct CcbContext* const       p_context,
                     const uint64_t                 p_hostVisibleSize,
                     const uint64_t                 p_deviceLocalSize,
                     const struct memoryTypeIndices p_typeIndices)
{
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
        .memoryTypeIndex = p_typeIndices.hostVisibleIndex
    };
    VK_CHECK(vkAllocateMemory(p_context->device, &mallocInfo, nullptr, &p_context->hostVisibleMemory));
    mallocInfo.allocationSize  = p_deviceLocalSize;
    mallocInfo.memoryTypeIndex = p_typeIndices.deviceLocalIndex;
    VK_CHECK(vkAllocateMemory(p_context->device, &mallocInfo, nullptr, &p_context->deviceLocalMemory));
}

static inline void
memoryAllocateMmap(struct CcbContext* const p_context)
{
    const struct VkMemoryMapInfo mmapInfo = {
        .sType  = VK_STRUCTURE_TYPE_MEMORY_MAP_INFO,
        .pNext  = nullptr,
        .flags  = 0u,
        .memory = p_context->hostVisibleMemory,
        .offset = 0ull,
        .size   = VK_WHOLE_SIZE
    };
    VK_CHECK(vkMapMemory2(p_context->device, &mmapInfo, (void**)&p_context->hostVisibleHostBase));
}

static inline void
memoryAllocateBindBuffers(struct CcbContext* const p_context)
{
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
    VK_CHECK(vkBindBufferMemory2(p_context->device, 2u, bindBufInfos));
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
    if (p_hostVisibleSize < p_deviceLocalSize) {
        _CCB_LOG_ERROR("[Calcubrute Error] Host visible size must be at least device local size\n");
        return CCB_ARGUMENT_ERROR;
    }

    struct memoryTypeIndices ids = memoryAllocateLocateTypeIndices(p_context);
    if (ids.hostVisibleIndex == UINT32_MAX) {
        _CCB_LOG_ERROR("[Calcubrute Error] Missing valid host visible memory type index\n");
        return -1;
    }
    if (ids.deviceLocalIndex == UINT32_MAX) {
        _CCB_LOG_ERROR("[Calcubrute Error] Missing valid device local memory type index\n");
        return -1;
    }

    _CCB_LOG_INFO("[Calcubrute Info] Host visible index = %u\n", ids.hostVisibleIndex);
    _CCB_LOG_INFO("[Calcubrute Info] Device local index = %u\n", ids.deviceLocalIndex);

    memoryAllocateCreateBuffers(p_context, p_hostVisibleSize, p_deviceLocalSize);

    _CCB_LOG_INFO("[Calcubrute Info] Host visible size = 0x%llx\n", p_hostVisibleSize);
    _CCB_LOG_INFO("[Calcubrute Info] Device local size = 0x%llx\n", p_deviceLocalSize);

    memoryAllocateMalloc(p_context, p_hostVisibleSize, p_deviceLocalSize, ids);
    memoryAllocateMmap(p_context);

    _CCB_LOG_INFO("[Calcubrute Info] Mapped host visible memory to 0x%p\n", p_context->hostVisibleHostBase);

    memoryAllocateBindBuffers(p_context);
    memoryAllocateRetriveAddresses(p_context);

    _CCB_LOG_INFO("[Calcubrute Info] Host visible memory starts at device address 0x%llx\n", p_context->hostVisibleDeviceBase);
    _CCB_LOG_INFO("[Calcubrute Info] Device local memory starts at device address 0x%llx\n", p_context->deviceLocalDeviceBase);

    return CCB_SUCCESS;
}

inline void
ccbMemoryFree(struct CcbContext* const p_context)
{
    // Destory both global buffers
    if (s_hostVisibleBuffer != VK_NULL_HANDLE) {
        vkDestroyBuffer(p_context->device, s_hostVisibleBuffer, nullptr);
        s_hostVisibleBuffer = VK_NULL_HANDLE;
    }
    if (s_deviceLocalBuffer != VK_NULL_HANDLE) {
        vkDestroyBuffer(p_context->device, s_deviceLocalBuffer, nullptr);
        s_deviceLocalBuffer = VK_NULL_HANDLE;
    }

    // Free host visible and device local memories
    if (p_context->hostVisibleMemory != VK_NULL_HANDLE) {
        // Unmap host visible memory before freeing it
        if (p_context->hostVisibleHostBase != nullptr) {
            struct VkMemoryUnmapInfo munmapInfo = {
                .sType  = VK_STRUCTURE_TYPE_MEMORY_UNMAP_INFO,
                .pNext  = nullptr,
                .flags  = 0u,
                .memory = p_context->hostVisibleMemory
            };
            VK_CHECK(vkUnmapMemory2(p_context->device, &munmapInfo));
            p_context->hostVisibleHostBase = nullptr;
        }
        vkFreeMemory(p_context->device, p_context->hostVisibleMemory, nullptr);
        p_context->hostVisibleMemory = VK_NULL_HANDLE;
    }
    if (p_context->deviceLocalMemory != VK_NULL_HANDLE) {
        vkFreeMemory(p_context->device, p_context->deviceLocalMemory, nullptr);
        p_context->deviceLocalMemory = VK_NULL_HANDLE;
    }
}
