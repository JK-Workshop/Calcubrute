// Copyright (c) JK Workshop - All rights reserved

#include <JK/Calcubrute/Context.h>
#include <JK/Calcubrute/Memory.h>

static VkBuffer s_hostVisibleBuffer = VK_NULL_HANDLE;
static VkBuffer s_deviceLocalBuffer = VK_NULL_HANDLE;

static inline int
memoryInitLocateTypeIndices(VkPhysicalDevice p_physicalDevice,
                            uint32_t* const  p_hi, // host visible index
                            uint32_t* const  p_di) // device local index
{
    // Invalidate
    *p_hi = *p_di = UINT32_MAX;

    // Query memory properties
    struct VkPhysicalDeviceMemoryProperties2 p
        = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_PROPERTIES_2, nullptr};
    vkGetPhysicalDeviceMemoryProperties2(p_physicalDevice, &p);

    // Iteratively check memory flags to retrieve indices, ties broken by smaller index first
    constexpr uint32_t f0 = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    constexpr uint32_t f1 = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    for (int i = p.memoryProperties.memoryTypeCount - 1; i >= 0; --i) {
        if ((p.memoryProperties.memoryTypes[i].propertyFlags & f0) == f0) {
            *p_di = i;
        }
        if ((p.memoryProperties.memoryTypes[i].propertyFlags & f1) == f1) {
            *p_hi = i;
        }
    }

    // Check and return
    if (*p_hi == UINT32_MAX) {
        sprintf(CcbErrorMessage, "failed to locate host visible memory type index");
        return -1;
    }
    if (*p_di == UINT32_MAX) {
        sprintf(CcbErrorMessage, "failed to locate device local memory type index");
        return -1;
    }

    return 0;
}

static inline int
memoryInitCreateBuffers(VkDevice        p_device,
                        uint64_t* const p_hs, // host visible size
                        uint64_t* const p_ds) // device local size
{
    int result;

    // Set up host visible buffer creation infos
    struct VkBufferUsageFlags2CreateInfo bufUsageInfo = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_USAGE_FLAGS_2_CREATE_INFO,
        .pNext = nullptr,
        .usage = VK_BUFFER_USAGE_2_TRANSFER_SRC_BIT
               | VK_BUFFER_USAGE_2_TRANSFER_DST_BIT
               | VK_BUFFER_USAGE_2_SHADER_DEVICE_ADDRESS_BIT
    };
    struct VkBufferCreateInfo bufInfo = {
        .sType                 = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .pNext                 = &bufUsageInfo,
        .flags                 = 0u,
        .size                  = *p_hs,
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
    vkGetDeviceBufferMemoryRequirements(p_device, &reqInfo, &req);
    *p_hs = bufInfo.size = req.memoryRequirements.size + CCB_PAGE_SIZE;
    result = vkCreateBuffer(p_device, &bufInfo, nullptr, &s_hostVisibleBuffer);
    if (result != VK_SUCCESS) {
        s_hostVisibleBuffer = VK_NULL_HANDLE;
        sprintf(CcbErrorMessage, "host visible buffer creation failed with VkResult %i", result);
        return -1;
    }

    // Set up device local buffer creation infos
    bufInfo.size = *p_ds;
    bufUsageInfo.usage |= VK_BUFFER_USAGE_2_STORAGE_BUFFER_BIT;

    // Create device local buffer
    vkGetDeviceBufferMemoryRequirements(p_device, &reqInfo, &req);
    *p_ds = bufInfo.size = req.memoryRequirements.size + CCB_PAGE_SIZE;
    result = vkCreateBuffer(p_device, &bufInfo, nullptr, &s_deviceLocalBuffer);
    if (result != VK_SUCCESS) {
        vkDestroyBuffer(p_device, s_hostVisibleBuffer, nullptr);
        s_hostVisibleBuffer = VK_NULL_HANDLE;
        s_deviceLocalBuffer = VK_NULL_HANDLE;
        sprintf(CcbErrorMessage, "device local buffer creation failed with VkResult %i", result);
        return -1;
    }

    return 0;
}

static inline int
memoryInitMalloc(struct CCBMemory* const p_memory,
                 VkDevice                p_device,
                 const uint64_t          p_hs, // host visible size
                 const uint64_t          p_ds, // device local size
                 const uint32_t          p_hi, // host visible index
                 const uint32_t          p_di) // device local index
{
    int result;

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
        .allocationSize  = p_hs,
        .memoryTypeIndex = p_hi
    };

    // Allocate host visible memory
    result = vkAllocateMemory(p_device, &mallocInfo, nullptr, &p_memory->hostVisibleMemory);
    if (result != VK_SUCCESS) {
        sprintf(CcbErrorMessage, "host visible memory allocation failed with VkResult %i", result);
        goto OnError;
    }

    // Set up device local memory allocation infos
    mallocInfo.allocationSize  = p_ds;
    mallocInfo.memoryTypeIndex = p_di;

    // Allocate device local memory
    result = vkAllocateMemory(p_device, &mallocInfo, nullptr, &p_memory->deviceLocalMemory);
    if (result != VK_SUCCESS) {
        vkFreeMemory(p_device, p_memory->hostVisibleMemory, nullptr);
        p_memory->deviceLocalMemory = VK_NULL_HANDLE;
        sprintf(CcbErrorMessage, "device local memory allocation failed with VkResult %i", result);
        goto OnError;
    }

    return 0;

OnError:
    p_memory->hostVisibleMemory = VK_NULL_HANDLE;
    return -1;
}

static inline int
memoryInitMmap(struct CCBMemory* const p_memory,
               VkDevice                p_device)
{
    int result;

    const struct VkMemoryMapInfo info = {
        .sType  = VK_STRUCTURE_TYPE_MEMORY_MAP_INFO,
        .pNext  = nullptr,
        .flags  = 0u,
        .memory = p_memory->hostVisibleMemory,
        .offset = 0ull,
        .size   = VK_WHOLE_SIZE
    };

    result = vkMapMemory2(p_device, &info, (void**)&p_memory->hostVisibleHostBase);
    p_memory->hostVisibleHostBase += 0x2000;
    if (result != VK_SUCCESS) {
        sprintf(CcbErrorMessage, "host visible memory map failed with VkResult %i", result);
        p_memory->hostVisibleHostBase = nullptr;
        return -1;
    }

    return 0;
}

static inline int
memoryInitBindBuffers(struct CCBMemory* const p_memory,
                      VkDevice                p_device)
{
    int result;

    struct VkBindBufferMemoryInfo infos[2] = {
        {
            .sType        = VK_STRUCTURE_TYPE_BIND_BUFFER_MEMORY_INFO,
            .pNext        = nullptr,
            .buffer       = s_hostVisibleBuffer,
            .memory       = p_memory->hostVisibleMemory,
            .memoryOffset = 0ull
        },
        {
            .sType        = VK_STRUCTURE_TYPE_BIND_BUFFER_MEMORY_INFO,
            .pNext        = nullptr,
            .buffer       = s_deviceLocalBuffer,
            .memory       = p_memory->deviceLocalMemory,
            .memoryOffset = 0ull
        }
    };

    result = vkBindBufferMemory2(p_device, 2u, infos);
    if (result != VK_SUCCESS) {
        sprintf(CcbErrorMessage, "buffer binding failed with VkResult %i", result);
        return -1;
    }

    return 0;
}

static inline void
memoryInitGetDeviceAddresses(struct CCBMemory* const p_memory,
                             VkDevice                p_device)
{
    struct VkBufferDeviceAddressInfo info = {
        .sType  = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
        .pNext  = nullptr,
        .buffer = s_hostVisibleBuffer
    };
    p_memory->hostVisibleDeviceBase = vkGetBufferDeviceAddress(p_device, &info) + 0x2000ull & -0x2000ull;
    info.buffer = s_deviceLocalBuffer;
    p_memory->deviceLocalDeviceBase = vkGetBufferDeviceAddress(p_device, &info) + 0x2000ull & -0x2000ull;
}

static inline int
memoryInitInitPageTable(struct CCBMemory* const p_memory,
                        const uint64_t          p_size) // must be multiple of CCB_PAGE_SIZE
{
    if ((p_size & 0x1fffull) != 0ull) {
        sprintf(CcbErrorMessage, "memory size is not a multiple of page size");
        return -1;
    }

    p_memory->numPages = p_size >> 13u;

    p_memory->freePagePool = malloc(p_memory->numPages * sizeof(uint64_t));
    p_memory->freePagePoolTop = p_memory->numPages - 1u;
    if (p_memory->freePagePool == nullptr) {
        sprintf(CcbErrorMessage, "failed to allocate available page pool");
        return -1;
    }
    for (uint32_t i = 0u; i < p_memory->numPages; ++i) {
        p_memory->freePagePool[i] = p_memory->hostVisibleDeviceBase + (i << 13);
    }

    p_memory->memcpyInfo = (struct VkCopyDeviceMemoryInfoKHR) {
        .sType       = VK_STRUCTURE_TYPE_COPY_DEVICE_MEMORY_INFO_KHR,
        .pNext       = nullptr,
        .regionCount = 0u,
        .pRegions    = malloc(p_memory->numPages * sizeof(struct VkDeviceMemoryCopyKHR))
    };
    if (p_memory->memcpyInfo.pRegions == nullptr) {
        sprintf(CcbErrorMessage, "failed to allocate memory for memcpy regions");
        return -1;
    }

    // Initialize each region
    auto r = (struct VkDeviceMemoryCopyKHR*)p_memory->memcpyInfo.pRegions;
    for (uint32_t i = 0u; i < p_memory->numPages; ++i) {
        r[i].sType = VK_STRUCTURE_TYPE_DEVICE_MEMORY_COPY_KHR;
        r[i].pNext = nullptr;
        r[i].srcRange.size = CCB_PAGE_SIZE;
        r[i].dstRange.size = CCB_PAGE_SIZE;
    }

    return 0;
}

inline int
ccbMemoryInit(struct CCBContext* const p_context,
              struct CCBMemory* const  p_memory,
              const uint64_t           p_size)
{
    int result;

    const uint64_t hs = p_size;

    uint32_t hi, di;
    result = memoryInitLocateTypeIndices(p_context->physicalDevices[0], &hi, &di);
    if (result != 0) {
        goto OnPreMallocError;
    }

    printf("[Calcubrute Info] Host visible index = %u\n", hi);
    printf("[Calcubrute Info] Device local index = %u\n", di);

    result = memoryInitCreateBuffers(p_context->device, (uint64_t*)&hs, (uint64_t*)&p_size);
    if (result != 0) {
        goto OnPreMallocError;
    }

    printf("[Calcubrute Info] Host visible size = 0x%llx\n", hs - CCB_PAGE_SIZE);
    printf("[Calcubrute Info] Device local size = 0x%llx\n", p_size - CCB_PAGE_SIZE);

    result = memoryInitMalloc(p_memory, p_context->device, hs, p_size, hi, di);
    if (result != 0) {
        goto OnMallocError;
    }

    result = memoryInitMmap(p_memory, p_context->device);
    if (result != 0) {
        goto OnMmapError;
    }

    result = memoryInitBindBuffers(p_memory, p_context->device);
    if (result != 0) {
        goto OnPostMmapError;
    }

    memoryInitGetDeviceAddresses(p_memory, p_context->device);
    p_memory->pageToFrameAddOn = (int64_t)p_memory->deviceLocalDeviceBase - p_memory->hostVisibleDeviceBase;

    result = memoryInitInitPageTable(p_memory, p_size - CCB_PAGE_SIZE);
    if (result != 0) {
        goto OnPostMmapError;
    }

    return 0;

OnPostMmapError:
    const struct VkMemoryUnmapInfo munmapInfo = {
        .sType  = VK_STRUCTURE_TYPE_MEMORY_UNMAP_INFO,
        .pNext  = nullptr,
        .flags  = 0u,
        .memory = p_memory->hostVisibleMemory
    };
    vkUnmapMemory2(p_context->device, &munmapInfo);

OnMmapError:
    vkFreeMemory(p_context->device, p_memory->hostVisibleMemory, nullptr);
    vkFreeMemory(p_context->device, p_memory->deviceLocalMemory, nullptr);

OnMallocError:
    vkDestroyBuffer(p_context->device, s_hostVisibleBuffer, nullptr);
    vkDestroyBuffer(p_context->device, s_deviceLocalBuffer, nullptr);
    s_hostVisibleBuffer = VK_NULL_HANDLE;
    s_deviceLocalBuffer = VK_NULL_HANDLE;

OnPreMallocError:
    return -1;
}

inline void
ccbMemoryDestroy(struct CCBContext* const p_context,
                 struct CCBMemory* const  p_memory)
{
    int result;

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

    // Destroy page table
    if (p_memory->memcpyInfo.pRegions != nullptr) {
        free((void*)p_memory->memcpyInfo.pRegions);
        free(p_memory->freePagePool);
        p_memory->memcpyInfo.pRegions = nullptr;
        p_memory->freePagePool        = nullptr;
    }

    // Free host visible memory
    if (p_memory->hostVisibleMemory != VK_NULL_HANDLE) {
        // Unmap before freeing
        if (p_memory->hostVisibleHostBase != nullptr) {
            const struct VkMemoryUnmapInfo munmapInfo = {
                .sType  = VK_STRUCTURE_TYPE_MEMORY_UNMAP_INFO,
                .pNext  = nullptr,
                .flags  = 0u,
                .memory = p_memory->hostVisibleMemory
            };
            result = vkUnmapMemory2(p_context->device, &munmapInfo);
            if (result != VK_SUCCESS) {
                sprintf(CcbErrorMessage, "Host visible memory unmap failed with VkResult %i", result);
            }
            p_memory->hostVisibleHostBase = nullptr;
        }
        vkFreeMemory(p_context->device, p_memory->hostVisibleMemory, nullptr);
        p_memory->hostVisibleMemory = VK_NULL_HANDLE;
    }

    // Free device local memory
    if (p_memory->deviceLocalMemory != VK_NULL_HANDLE) {
        vkFreeMemory(p_context->device, p_memory->deviceLocalMemory, nullptr);
        p_memory->deviceLocalMemory = VK_NULL_HANDLE;
    }
}

static inline void
memoryUploadPage(struct VkCopyDeviceMemoryInfoKHR* const p_memcpyInfo,
                 const int64_t                           p_addOn,
                 const uint64_t                          p_pageBase)
{
    auto r = (struct VkDeviceMemoryCopyKHR*)p_memcpyInfo->pRegions + p_memcpyInfo->regionCount++;
    r->srcRange.address = p_pageBase;
    r->srcFlags = VK_ADDRESS_COMMAND_FULLY_BOUND_BIT_KHR;
    r->dstRange.address = p_pageBase + p_addOn;
    r->dstFlags = VK_ADDRESS_COMMAND_FULLY_BOUND_BIT_KHR | VK_ADDRESS_COMMAND_STORAGE_BUFFER_USAGE_BIT_KHR;
}

static inline void
memoryDownloadFrame(struct VkCopyDeviceMemoryInfoKHR* const p_memcpyInfo,
                    const int64_t                           p_addOn,
                    const uint64_t                          p_frameBase)
{
    auto r = (struct VkDeviceMemoryCopyKHR*)p_memcpyInfo->pRegions + p_memcpyInfo->regionCount++;
    r->srcRange.address = p_frameBase;
    r->srcFlags = VK_ADDRESS_COMMAND_FULLY_BOUND_BIT_KHR | VK_ADDRESS_COMMAND_STORAGE_BUFFER_USAGE_BIT_KHR;
    r->dstRange.address = p_frameBase - p_addOn;
    r->dstFlags = VK_ADDRESS_COMMAND_FULLY_BOUND_BIT_KHR;
}

inline void
ccbMemorySync(struct CCBMemory* const p_memory JK_NONNULL(),
              VkCommandBuffer         p_commandBuffer)
{
    vkCmdCopyMemoryKHR(p_commandBuffer, &p_memory->memcpyInfo);
    p_memory->memcpyInfo.regionCount = 0u; // clear copy queue
}

inline void
ccbMemoryPrint(struct CCBMemory* const p_memory,
               FILE*                   p_fp)
{
    fprintf(p_fp, "Host Visible Memory Host Base: 0x%p\n"
                  "Host Visible Memory Device Base: 0x%llx\n"
                  "Device Local Memory Device Base: 0x%llx\n"
                  "Number of Pages: %u\n"
                  "Page to Frame Add On: 0x%llx\n"
                  "Number of Pages Consumed: %u\n",
                  p_memory->hostVisibleHostBase,
                  p_memory->hostVisibleDeviceBase,
                  p_memory->deviceLocalDeviceBase,
                  p_memory->numPages,
                  p_memory->pageToFrameAddOn,
                  p_memory->numPages - p_memory->freePagePoolTop - 1u);
}

inline void
upload(struct CCBMemory* const p_memory, const uint64_t p_pageBase)
{
    memoryUploadPage(&p_memory->memcpyInfo, p_memory->pageToFrameAddOn, p_pageBase);
}

inline void
download(struct CCBMemory* const p_memory, const uint64_t p_frameBase)
{
    memoryDownloadFrame(&p_memory->memcpyInfo, p_memory->pageToFrameAddOn, p_frameBase);
}

// inline void
// PageTableUploadPage256(struct PageTable* )
