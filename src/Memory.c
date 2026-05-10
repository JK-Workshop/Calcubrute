// Copyright (c) JK Workshop - All rights reserved

#include <JK/Calcubrute/Context.h>

static VkBuffer s_hostVisibleBuffer = VK_NULL_HANDLE;
static VkBuffer s_deviceLocalBuffer = VK_NULL_HANDLE;

inline int
ccbMalloc(struct CcbContext* const p_context,
          const size_t             p_hostVisibleSize,
          const size_t             p_deviceLocalSize)
{
    if (p_hostVisibleSize < p_deviceLocalSize) {
        return CCB_ARGUMENT_ERROR;
    }

    // Invalidate
    uint32_t hostVisibleIndex = UINT32_MAX;
    uint32_t deviceLocalIndex = UINT32_MAX;

    // Query memory properties
    struct VkPhysicalDeviceMemoryProperties2 memProp = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_PROPERTIES_2,
        .pNext = nullptr
    };
    vkGetPhysicalDeviceMemoryProperties2(p_context->physicalDevices[0], &memProp);

    // Iteratively check memory flags to retrieve indices, ties broken by smaller index first
    constexpr uint32_t deviceLocalFlag = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    constexpr uint32_t hostVisibleFlag = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
                                       | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    for (int i = memProp.memoryProperties.memoryTypeCount - 1; i >= 0; --i) {
        if ((memProp.memoryProperties.memoryTypes[i].propertyFlags & deviceLocalFlag) == deviceLocalFlag) {
            deviceLocalIndex = i;
        }
        if ((memProp.memoryProperties.memoryTypes[i].propertyFlags & hostVisibleFlag) == hostVisibleFlag) {
            hostVisibleIndex = i;
        }
    }

    // Check memory type indices
    if (hostVisibleIndex == UINT32_MAX || deviceLocalIndex == UINT32_MAX) {
        return -1;
    }

    printf("[Calcubrute] Host visible index = %u\n", hostVisibleIndex);
    printf("[Calcubrute] Device local index = %u\n", deviceLocalIndex);

    // Create buffers for both memories
    struct VkBufferCreateInfo bufferInfo = {
        .sType                 = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .pNext                 = nullptr,
        .flags                 = 0u,
        .size                  = p_hostVisibleSize,
        .usage                 = VK_BUFFER_USAGE_TRANSFER_SRC_BIT
                               | VK_BUFFER_USAGE_TRANSFER_DST_BIT
                               | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
        .sharingMode           = VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = 0u,
        .pQueueFamilyIndices   = nullptr
    };
    VK_CHECK(vkCreateBuffer(p_context->device, &bufferInfo, nullptr, &s_hostVisibleBuffer));
    bufferInfo.size = p_deviceLocalSize;
    bufferInfo.usage |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    VK_CHECK(vkCreateBuffer(p_context->device, &bufferInfo, nullptr, &s_deviceLocalBuffer));

    // Get required memory size for buffers
    uint64_t hostVisibleSize = 0ull;
    uint64_t deviceLocalSize = 0ull;
    struct VkBufferMemoryRequirementsInfo2 memoryReqInfo = {
        .sType  = VK_STRUCTURE_TYPE_BUFFER_MEMORY_REQUIREMENTS_INFO_2,
        .pNext  = nullptr,
        .buffer = s_hostVisibleBuffer
    };
    struct VkMemoryRequirements2 memoryReq = {
        .sType              = VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2,
        .pNext              = nullptr,
        .memoryRequirements = {}
    };
    vkGetBufferMemoryRequirements2(p_context->device, &memoryReqInfo, &memoryReq);
    hostVisibleSize = memoryReq.memoryRequirements.size;
    memoryReqInfo.buffer = s_deviceLocalBuffer;
    vkGetBufferMemoryRequirements2(p_context->device, &memoryReqInfo, &memoryReq);
    deviceLocalSize = memoryReq.memoryRequirements.size;

    printf("[Calcubrute] Host visible size = 0x%llx\n", hostVisibleSize);
    printf("[Calcubrute] Device local size = 0x%llx\n", deviceLocalSize);

    // Allocate host visible and device local memories
    struct VkMemoryAllocateFlagsInfo mallocFlagsInfo = {
        .sType      = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO,
        .pNext      = nullptr,
        .flags      = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT,
        .deviceMask = 0u
    };
    struct VkMemoryAllocateInfo mallocInfo = {
        .sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .pNext           = &mallocFlagsInfo,
        .allocationSize  = hostVisibleSize,
        .memoryTypeIndex = hostVisibleIndex
    };
    VK_CHECK(vkAllocateMemory(p_context->device, &mallocInfo, nullptr, &p_context->hostVisibleMemory));
    mallocInfo.allocationSize  = deviceLocalSize;
    mallocInfo.memoryTypeIndex = deviceLocalIndex;
    VK_CHECK(vkAllocateMemory(p_context->device, &mallocInfo, nullptr, &p_context->deviceLocalMemory));

    // Map host visible memory
    p_context->hostVisibleHostBase = nullptr;
    struct VkMemoryMapInfo mmapInfo = {
        .sType  = VK_STRUCTURE_TYPE_MEMORY_MAP_INFO,
        .pNext  = nullptr,
        .flags  = 0u,
        .memory = p_context->hostVisibleMemory,
        .offset = 0ull,
        .size   = hostVisibleSize
    };
    VK_CHECK(vkMapMemory2(p_context->device, &mmapInfo, (void**)&p_context->hostVisibleHostBase));

    printf("[Calcubrute] Mapped host visible memory to virtual RAM at 0x%p\n", p_context->hostVisibleHostBase);

    // Attach buffer to memories
    struct VkBindBufferMemoryInfo bindBufferInfos[2] = {
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
    VK_CHECK(vkBindBufferMemory2(p_context->device, 2, bindBufferInfos));
    // Get device address from both memories
    p_context->hostVisibleDeviceBase = 0ull;
    p_context->deviceLocalDeviceBase = 0ull;
    struct VkBufferDeviceAddressInfo deviceAddrInfo = {
        .sType  = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
        .pNext  = nullptr,
        .buffer = s_hostVisibleBuffer
    };
    p_context->hostVisibleDeviceBase = vkGetBufferDeviceAddress(p_context->device, &deviceAddrInfo);
    deviceAddrInfo.buffer = s_deviceLocalBuffer;
    p_context->deviceLocalDeviceBase = vkGetBufferDeviceAddress(p_context->device, &deviceAddrInfo);

    printf("[Calcubrute] Host visible memory starts at device address 0x%llx\n", p_context->hostVisibleDeviceBase);
    printf("[Calcubrute] Device local memory starts at device address 0x%llx\n", p_context->deviceLocalDeviceBase);

    return 0;
}

inline void
ccbFree(struct CcbContext* const p_context)
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
