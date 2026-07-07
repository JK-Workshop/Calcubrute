// Copyright (c) JK Workshop - All rights reserved

// Note: all helper functions do not invalidate on error, callers are going to handle it instead

#include <JK/Calcubrute/Context.h>
#include <JK/Calcubrute/Memory.h>

static VkBuffer s_HostVisibleBuffer;
static VkBuffer s_DeviceLocalBuffer;

static inline int
memoryInitLocateTypeIndices(VkPhysicalDevice    p_PhysDev,
                            uint32_t*           p_HostVisibleIdx
                            uint32_t*           p_DeviceLocalIdx)
{
    // Invalidate locally
    *p_HostVisibleIdx = *p_DeviceLocalIdx = UINT32_MAX;

    // Query memory properties
    struct VkPhysicalDeviceMemoryProperties2 MemProps = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_PROPERTIES_2, nullptr};
    vkGetPhysicalDeviceMemoryProperties2(p_PhysDev, &MemProps);

    // Iteratively check memory flags to retrieve indices, ties broken by smaller index first
    constexpr uint32_t f0 = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    constexpr uint32_t f1 = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    for (int i = p.memoryProperties.memoryTypeCount - 1; i >= 0; --i) {
        if ((p.memoryProperties.memoryTypes[i].propertyFlags & f0) == f0) {
            *p_DeviceLocalIdx = i;
        }
        if ((p.memoryProperties.memoryTypes[i].propertyFlags & f1) == f1) {
            *p_HostVisibleIdx = i;
        }
    }

    // Check and return
    if (*p_HostVisibleIdx == UINT32_MAX) {
        sprintf(CcbErrMsg, "failed to locate host visible memory type index");
        return -1;
    }
    if (*p_DeviceLocalIdx == UINT32_MAX) {
        sprintf(CcbErrMsg, "failed to locate device local memory type index");
        return -1;
    }

    return 0;
}

static inline int
memoryInitCreateBuffers(VkDevice     p_Dev,
                        uint64_t*    p_HostVisibleSize,
                        uint64_t*    p_DeviceLocalSize)
{
    int Result;

    /* Get the required size of host visible buffer */
    uint32_t QueueFamIdxs[] = {1, 2};
    struct VkBufferUsageFlags2CreateInfo BufUsageInfo = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_USAGE_FLAGS_2_CREATE_INFO,
        .pNext = nullptr,
        .usage = VK_BUFFER_USAGE_2_TRANSFER_SRC_BIT |
                 VK_BUFFER_USAGE_2_TRANSFER_DST_BIT |
                 VK_BUFFER_USAGE_2_SHADER_DEVICE_ADDRESS_BIT
    };
    struct VkBufferCreateInfo BufCreateInfo = {
        .sType                 = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .pNext                 = &BufUsageInfo,
        .flags                 = 0,
        /* Use the passed-in size to query the required size */
        .size                  = *p_HostVisibleSize,
        .usage                 = 0,
        .sharingMode           = VK_SHARING_MODE_CONCURRENT,
        .queueFamilyIndexCount = 2,
        .pQueueFamilyIndices   = QueueFamIdxs
    };
    struct VkDeviceBufferMemoryRequirements MemReqInfo = {
        .sType                 = VK_STRUCTURE_TYPE_DEVICE_BUFFER_MEMORY_REQUIREMENTS,
        .pNext                 = nullptr,
        .pCreateInfo           = &BufCreateInfo
    };
    struct VkMemoryRequirements2 MemReq = {
        .sType                 = VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2,
        .pNext                 = nullptr,
        .memoryRequirements    = {0}
    };
    vkGetDeviceBufferMemoryRequirements(p_Dev, &MemReqInfo, &MemReq);

    /* To receive a CCB_PAGE_SIZE aligned based without losing the size,
       extent the allocation size by CCB_PAGE_SIZE and then pick from
       [vkGetBufferDeviceAddress, vkGetBufferDeviceAddress + CCB_PAGE_SIZE)
       the value that's CCB_PAGE_SIZE aligned as the base */
    *p_HostVisibleSize = BufCreateInfo.size
                       = MemReq.memoryRequirements.size + CCB_PAGE_SIZE;

    Result = vkCreateBuffer(p_Dev, &BufCreateInfo, nullptr, &s_HostVisibleBuffer);
    if (Result != VK_SUCCESS) {
        sprintf(CcbErrMsg, "host visible buffer creation failed with VkResult %i", Result);
        return -1;
    }

    /* Does the same thing for device local buffer, which reuses the BufCreateInfo
       and BufUsageInfo except */

    /* Different passed-in size */
    BufCreateInfo.size = *p_DeviceLocalSize;

    /* Add the storage buffer usage */
    BufUsageInfo.usage |= VK_BUFFER_USAGE_2_STORAGE_BUFFER_BIT;

    vkGetDeviceBufferMemoryRequirements(p_Dev, &MemReqInfo, &MemReq);

    *p_DeviceLocalSize = BufCreateInfo.size
                       = MemReq.memoryRequirements.size + CCB_PAGE_SIZE;

    Result = vkCreateBuffer(p_Dev, &BufCreateInfo, nullptr, &s_DeviceLocalBuffer);
    if (Result != VK_SUCCESS) {
        /* Destroy previously created host visible buffer */
        vkDestroyBuffer(p_Dev, s_HostVisibleBuffer, nullptr);

        sprintf(CcbErrMsg, "device local buffer creation failed with VkResult %i", Result);
        return -1;
    }

    return 0;
}

static inline int
memoryInitMalloc(struct CCBMemory* p_pMemory,
                 VkDevice          p_Dev,
                 uint64_t          p_HostVisibleSize,
                 uint64_t          p_DeviceLocalSize,
                 uint32_t          p_HostVisibleIdx,
                 uint32_t          p_DeviceLocalIdx)
{
    int Result;

    /* Allocate host visible memory */
    struct VkMemoryAllocateFlagsInfo MallocFlagsInfo = {
        .sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO,
        .pNext           = nullptr,
        .flags           = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT,
        .deviceMask      = 0
    };
    struct VkMemoryAllocateInfo MallocInfo = {
        .sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .pNext           = &MallocFlagsInfo,
        .allocationSize  = p_HostVisibleSize,
        .memoryTypeIndex = p_HostVisibleIdx
    };

    Result = vkAllocateMemory(p_Dev, &MallocInfo, nullptr, &p_pMemory->HostVisibleMemory);
    if (Result != VK_SUCCESS) {
        sprintf(CcbErrMsg, "host visible memory allocation failed with VkResult %i", Result);
        return -1;
    }

    /* Does the same thing for device local memory, which reuses the MallocInfo with */
    mallocInfo.allocationSize  = p_DeviceLocalSize;
    mallocInfo.memoryTypeIndex = p_DeviceLocalIdx;

    Result = vkAllocateMemory(p_Dev, &MallocInfo, nullptr, &p_pMemory->DeviceLocalMemory);
    if (Result != VK_SUCCESS) {
        vkFreeMemory(p_Dev, p_pMemory->HostVisibleMemory, nullptr);
        sprintf(CcbErrMsg, "device local memory allocation failed with VkResult %i", Result);
        return -1;
    }

    return 0;
}

static inline int
memoryInitMmap(struct CCBMemory* p_pMemory,
               VkDevice          p_Dev)
{
    int Result;

    /* Map host visible memory to host memory space */
    struct VkMemoryMapInfo MmapInfo = {
        .sType  = VK_STRUCTURE_TYPE_MEMORY_MAP_INFO,
        .pNext  = nullptr,
        .flags  = 0,
        .memory = p_pMemory->HostVisileMemory,
        .offset = 0,
        .size   = VK_WHOLE_SIZE
    };

    Result = vkMapMemory2(p_Dev, &MmapInfo, (void**)&p_pMemory->HostVisibleHostBase);
    if (Result != VK_SUCCESS) {
        sprintf(CcbErrMsg, "host visible memory map failed with VkResult %i", result);
        return -1;
    }

    /* Since the base of host visible memory is modified to align with CCB_PAGE_SIZE,
       also modify the mapped base accordingly */
    p_pMemory->HostVisibleHostBase += CCB_PAGE_SIZE; // Warning!

    return 0;
}

static inline int
memoryInitBindBuffers(struct CCBMemory* p_pMemory,
                      VkDevice          p_Dev)
{
    int Result;

    /* Bind both host visible and device local buffers to their memories */
    struct VkBindBufferMemoryInfo BindBufInfos[2] = {
        {
            .sType        = VK_STRUCTURE_TYPE_BIND_BUFFER_MEMORY_INFO,
            .pNext        = nullptr,
            .buffer       = s_HostVisibleBuffer,
            .memory       = p_pMemory->HostVisibleMemory,
            .memoryOffset = 0
        },
        {
            .sType        = VK_STRUCTURE_TYPE_BIND_BUFFER_MEMORY_INFO,
            .pNext        = nullptr,
            .buffer       = s_DeviceLocalBuffer,
            .memory       = p_pMemory->DeviceLocalMemory,
            .memoryOffset = 0
        }
    };

    Result = vkBindBufferMemory2(p_Dev, 2, BindBufInfos);
    if (Result != VK_SUCCESS) {
        sprintf(CcbErrMsg, "buffer binding failed with VkResult %i", Result);
        return -1;
    }

    return 0;
}

static inline int
memoryInitInitPageTable(struct CCBMemory* p_pMemory,
                        uint64_t          p_Size)
{
    /* Check p_Size is a multiple of CCB_PAGE_SIZE */
    if ((p_Size & 0x1FFF) != 0) {
        sprintf(CcbErrMsg, "memory size must be a multiple of CCB_PAGE_SIZE");
        return -1;
    }

    /* Achieve the number of pages of by dividing memory size by CCB_PAGE_SIZE */
    p_pMemory->NumPages = CCB_DEV_PAGE_SIZE(p_Size);

    /* Allocate the free page pool and set up its top pointer */
    p_pMemory->FreePagePool    = malloc(p_pMemory->NumPages * sizeof(uint64_t));
    p_pMemory->FreePagePoolTop = p_pMemory->NumPages;
    if (p_pMemory->FreePagePool == nullptr) {
        sprintf(CcbErrMsg, "failed to allocate available page pool");
        return -1;
    }

    for (uint32_t i = 0; i < p_pMemory->NumPages; ++i) {
        p_pMemory->FreePagePool[i] = p_pMemory->HostVisibleDeviceBase + CCB_MUL_PAGE_SIZE(i);
    }

#if CCB_HAS_DEVICE_ADDRESS_COMMAND_KHR
    p_pMemory->MemcpyInfo = (struct VkCopyDeviceMemoryInfoKHR) {
        .sType       = VK_STRUCTURE_TYPE_COPY_DEVICE_MEMORY_INFO_KHR,
        .pNext       = nullptr,
        .regionCount = 0,
        .pRegions    = malloc(p_pMemory->NumPages * sizeof(struct VkDeviceMemoryCopyKHR))
    };
    if (p_pMemory->MemcpyInfo.pRegions == nullptr) {
        sprintf(CcbErrMsg, "failed to allocate memory for memcpy regions");
        return -1;
    }

    /* Initialize each region */
    struct VkDeviceMemoryCopyKHR* Regions = p_pMemory->MemcpyInfo.pRegions;
    for (uint32_t i = 0; i < p_pMemory->NumPages; ++i) {
        Regions[i].sType = VK_STRUCTURE_TYPE_DEVICE_MEMORY_COPY_KHR;
        Regions[i].pNext = nullptr;
    }
#else
    p_pMemory->MemcpyInfo = (struct VkCopyBufferInfo2) {
        .sType = VK_STRUCTURE_TYPE_COPY_BUFFER_INFO_2,
        .pNext = nullptr,
        .srcBuffer = s_HostVisibleBuffer,
        .dstBuffer = s_DeviceLocalBuffer,
        .regionCount = 0,
        .pRegions = malloc(p_pMemory->NumPages * sizeof(struct VkBufferCopyInfo2))
    };

    /* Initialize each region */
    struct VkDeviceMemoryCopyKHR* 
#endif

    return 0;
}

inline int
ccbMemoryInit(struct CCBContext* p_context,
              struct CCBMemory*  p_memory,
              uint64_t           p_size)
{
    int result;

    // The only invalidation for a CCBMemory object
    p_memory->transferQueue = VK_NULL_HANDLE;

    // Retrieve transfer queue from VkDevice
    struct VkDeviceQueueInfo2 queueInfo = {
        .sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_INFO_2,
        .pNext            = nullptr,
        .flags            = 0u,
        .queueFamilyIndex = p_context->transferQueueFamilyIndex,
        .queueIndex       = 0u,
    };
    vkGetDeviceQueue2(p_context->device, &queueInfo, &p_memory->transferQueue);

    uint64_t hs = p_size;
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

    // Get device address
    struct VkBufferDeviceAddressInfo info = {
        .sType  = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
        .pNext  = nullptr,
        .buffer = s_hostVisibleBuffer
    };
    p_memory->hostVisibleDeviceBase = vkGetBufferDeviceAddress(p_context->device, &info)
                                    + 0x2000ull & -0x2000ull;
    info.buffer = s_deviceLocalBuffer;
    p_memory->deviceLocalDeviceBase = vkGetBufferDeviceAddress(p_context->device, &info)
                                    + 0x2000ull & -0x2000ull;

    result = memoryInitInitPageTable(p_memory, p_size - CCB_PAGE_SIZE);
    if (result != 0) {
        goto OnPostMmapError;
    }
    
    // Create transfer command pool
    struct VkCommandPoolCreateInfo cmdPoolInfo = {
        .sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .pNext            = nullptr,
        .flags            = 0u,
        .queueFamilyIndex = p_context->transferQueueFamilyIndex,
    };
    result = vkCreateCommandPool(p_context->device, &cmdPoolInfo, nullptr, &p_memory->transferCmdPool);
    if (result != VK_SUCCESS) {
        sprintf(CcbErrMsg, "failed to create transfer command pool with VkResult %i", result);
        goto OnPostMmapError;
    }
    
    // Allocate transfer command buffer
    struct VkCommandBufferAllocateInfo cmdBufInfo = {
        .sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .pNext              = nullptr,
        .commandPool        = p_memory->transferCmdPool,
        .level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1u // reserved
    };
    result = vkAllocateCommandBuffers(p_context->device, &cmdBufInfo, &p_memory->transferCmdBuffer);
    if (result != VK_SUCCESS) {
        sprintf(CcbErrMsg, "failed to allocate transfer command buffer with VkResult %i", result);
        goto OnAllocateCmdBufferError;
    }

    return 0;

OnAllocateCmdBufferError:
    vkDestroyCommandPool(p_context->device, p_memory->transferCmdPool, nullptr);

OnPostMmapError:
    struct VkMemoryUnmapInfo munmapInfo = {
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

OnPreMallocError:
    // Invalidate again, prevent future free on nullptr
    p_memory->transferQueue = VK_NULL_HANDLE;
    return -1;
}

inline void
ccbMemoryDestroy(struct CCBContext* p_context,
                 struct CCBMemory*  p_memory)
{
    if (p_memory->transferQueue == VK_NULL_HANDLE) {
        return;
    }

    int result;

    vkDestroyBuffer(p_context->device, s_hostVisibleBuffer, nullptr);
    vkDestroyBuffer(p_context->device, s_deviceLocalBuffer, nullptr);
    vkFreeCommandBuffers(p_context->device, p_memory->transferCmdPool, 1u, &p_memory->transferCmdBuffer);
    vkDestroyCommandPool(p_context->device, p_memory->transferCmdPool, nullptr);
    free((void*)p_memory->memcpyInfo.pRegions);
    free(p_memory->freePagePool);
    struct VkMemoryUnmapInfo munmapInfo = {
        .sType  = VK_STRUCTURE_TYPE_MEMORY_UNMAP_INFO,
        .pNext  = nullptr,
        .flags  = 0u,
        .memory = p_memory->hostVisibleMemory
    };
    result = vkUnmapMemory2(p_context->device, &munmapInfo);
    if (result != VK_SUCCESS) {
        sprintf(CcbErrMsg, "Host visible memory unmap failed with VkResult %i", result);
    }
    vkFreeMemory(p_context->device, p_memory->hostVisibleMemory, nullptr);
    vkFreeMemory(p_context->device, p_memory->deviceLocalMemory, nullptr);
}

inline int
ccbMemoryTransferBegin(struct CCBMemory*  p_memory,
                       struct CCBContext* p_context)
{
    int result;

    // Rewind memcpy region stack and free frame stack
    p_memory->memcpyInfo.regionCount = 0u;

    // Reset transfer command pool along with all allocated command buffers
    result = vkResetCommandPool(p_context->device, p_memory->transferCmdPool, 0u);
    if (result != VK_SUCCESS) {
        sprintf(CcbErrMsg, "failed to reset transfer command pool with VkResult %i", result);
        return -1;
    }

    // Begin recording transfer commands
    struct VkCommandBufferBeginInfo cmdBufBeginInfo = {
        .sType            = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .pNext            = nullptr,
        .flags            = 0u,
        .pInheritanceInfo = nullptr
    };
    result = vkBeginCommandBuffer(p_memory->transferCmdBuffer, &cmdBufBeginInfo);
    if (result != VK_SUCCESS) {
        sprintf(CcbErrMsg, "failed to begin transfer command buffer with VkResult %i", result);
        return -1;
    }

    return 0;
}

inline int
ccbMemoryTransferEnd(struct CCBMemory* p_memory)
{
    int result;

    // Record all memcpy regions
    vkCmdCopyMemoryKHR(p_memory->transferCmdBuffer, &p_memory->memcpyInfo);

    // End recording transfer commands
    result = vkEndCommandBuffer(p_memory->transferCmdBuffer);
    if (result != VK_SUCCESS) {
        sprintf(CcbErrMsg, "failed to end transfer command buffer with VkResult %i", result);
        return -1;
    }

    return 0;
}

inline void
ccbMemoryTransferFlush(struct CCBMemory*             p_memory,
                       struct VkSemaphoreSubmitInfo* p_waitInfo,
                       struct VkSemaphoreSubmitInfo* p_signalInfo)
{
    struct VkCommandBufferSubmitInfo cmdBufSubmitInfo = {
        .sType         = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
        .pNext         = nullptr,
        .commandBuffer = p_memory->transferCmdBuffer,
        .deviceMask    = 1u // reserved
    };
    struct VkSubmitInfo2 submitInfo = {
        .sType                    = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
        .pNext                    = nullptr,
        .flags                    = 0u,
        .waitSemaphoreInfoCount   = 1u,
        .pWaitSemaphoreInfos      = p_waitInfo,
        .commandBufferInfoCount   = 1u,
        .pCommandBufferInfos      = &cmdBufSubmitInfo,
        .signalSemaphoreInfoCount = 1u,
        .pSignalSemaphoreInfos    = p_signalInfo
    };
    vkQueueSubmit2(p_memory->transferQueue, 1u, &submitInfo, VK_NULL_HANDLE);
}

inline void
ccbMemoryUploadTensor(struct CCBMemory* p_memory,
                      struct CCBTensor* p_tensor,
                      uint64_t                p_deviceLocalBase)
{
    uint32_t numPagesRequired = p_tensor->size >> 12;
    for (uint32_t i = 0u; i < numPagesRequired; ++i) {
        auto r = (struct VkDeviceMemoryCopyKHR*)p_memory->memcpyInfo.pRegions + p_memory->memcpyInfo.regionCount;
        ++p_memory->memcpyInfo.regionCount;
        r->srcRange.address = p_tensor->hostBases[i];
        r->srcFlags         = VK_ADDRESS_COMMAND_FULLY_BOUND_BIT_KHR;
        r->dstRange.address = p_deviceLocalBase;
        r->dstFlags         = VK_ADDRESS_COMMAND_FULLY_BOUND_BIT_KHR | VK_ADDRESS_COMMAND_STORAGE_BUFFER_USAGE_BIT_KHR;
        p_deviceLocalBase += CCB_PAGE_SIZE;
    }
}

inline void
ccbMemoryDownloadTensor(struct CCBMemory* p_memory,
                        struct CCBTensor* p_tensor,
                        uint64_t                p_deviceLocalBase)
{
    uint32_t numPagesRequired = p_tensor->size >> 12;
    for (uint32_t i = 0u; i < numPagesRequired; ++i) {
        auto r = (struct VkDeviceMemoryCopyKHR*)p_memory->memcpyInfo.pRegions + p_memory->memcpyInfo.regionCount;
        ++p_memory->memcpyInfo.regionCount;
        r->srcRange.address = p_deviceLocalBase;
        r->srcFlags         = VK_ADDRESS_COMMAND_FULLY_BOUND_BIT_KHR | VK_ADDRESS_COMMAND_STORAGE_BUFFER_USAGE_BIT_KHR;
        r->dstRange.address = p_tensor->hostBases[i];
        r->dstFlags         = VK_ADDRESS_COMMAND_FULLY_BOUND_BIT_KHR;
        p_deviceLocalBase += CCB_PAGE_SIZE;
    }
}

inline void
ccbMemoryReleaseQueue(struct CCBMemory*  p_memory,
                      struct CCBContext* p_context)
{
    // struct VkMemoryRangeBarrierKHR barrier = {
    //     .sType               = VK_STRUCTURE_TYPE_MEMORY_RANGE_BARRIER_KHR,
    //     .pNext               = nullptr,
    //     .srcStageMask        = VK_PIPELINE_STAGE_2_COPY_BIT,
    //     .srcAccessMask       = VK_ACCESS_2_TRANSFER_WRITE_BIT,
    //     .dstStageMask        = VK_PIPELINE_STAGE_2_NONE,
    //     .dstAccessMask       = VK_ACCESS_2_NONE,
    //     .srcQueueFamilyIndex = p_context->transferQueueFamilyIndex,
    //     .dstQueueFamilyIndex = p_context->computeQueueFamilyIndex,
    //     .addressRange        = {p_memory->deviceLocalDeviceBase, },
    //     .addressFlags        = VK_ADDRESS_COMMAND_FULLY_BOUND_BIT_KHR | VK_ADDRESS_COMMAND_STORAGE_BUFFER_USAGE_BIT_KHR
    // };
    // struct VkMemoryRangeBarriersInfoKHR info = {
    //     .sType                   = VK_STRUCTURE_TYPE_MEMORY_RANGE_BARRIERS_INFO_KHR,
    //     .pNext                   = nullptr,
    //     .memoryRangeBarrierCount = 1u,
    //     .pMemoryRangeBarriers    = &barrier
    // };
    // struct VkDependencyInfo dep = {VK_STRUCTURE_TYPE_DEPENDENCY_INFO, &info};
    // vkCmdPipelineBarrier2(p_memory->transferCmdBuffer, &dep);
}

inline void
ccbMemoryPrint(struct CCBMemory* p_memory,
               FILE*                   p_fp)
{
    fprintf(p_fp, "[Calcubrute Info] Memory:\n"
                  "|__ Host Visible Memory Host Base: 0x%p\n"
                  "|__ Host Visible Memory Device Base: 0x%llx\n"
                  "|__ Device Local Memory Device Base: 0x%llx\n"
                  "|__ Number of Pages: %u\n"
                  "|__ Number of Pages Consumed: %u\n",
                  p_memory->hostVisibleHostBase,
                  p_memory->hostVisibleDeviceBase,
                  p_memory->deviceLocalDeviceBase,
                  p_memory->numPages,
                  p_memory->numPages - p_memory->freePagePoolTop);
}
    Result = vkCreateBuffer(p_Dev, &BufCreateInfo, nullptr, &s_HostVisibleBuffer);
    if (Result != VK_SUCCESS) {
        sprintf(CcbErrMsg, "host visible buffer creation failed with VkResult %i", Result);
        return -1;
    }

    // Set up device local buffer creation infos
    BufCreateInfo.size = *p_DeviceLocalSize;
    BufUsageInfo.usage |= VK_BUFFER_USAGE_2_STORAGE_BUFFER_BIT;

    // Create device local buffer
    vkGetDeviceBufferMemoryRequirements(p_device, &reqInfo, &req);
    *p_ds = bufInfo.size = req.memoryRequirements.size + CCB_PAGE_SIZE;
    result = vkCreateBuffer(p_device, &bufInfo, nullptr, &s_deviceLocalBuffer);
    if (result != VK_SUCCESS) {
        vkDestroyBuffer(p_device, s_hostVisibleBuffer, nullptr);
        sprintf(CcbErrMsg, "device local buffer creation failed with VkResult %i", result);
        return -1;
    }

    return 0;
}

static inline int
memoryInitMalloc(struct CCBMemory* p_memory,
                 VkDevice                p_device,
                 uint64_t          p_hs, // host visible size
                 uint64_t          p_ds, // device local size
                 uint32_t          p_hi, // host visible index
                 uint32_t          p_di) // device local index
{
    int result;

    // Set up host visible memory allocation infos
    struct VkMemoryAllocateFlagsInfo mallocFlagsInfo = {
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
        sprintf(CcbErrMsg, "host visible memory allocation failed with VkResult %i", result);
        return -1;
    }

    // Set up device local memory allocation infos
    mallocInfo.allocationSize  = p_ds;
    mallocInfo.memoryTypeIndex = p_di;

    // Allocate device local memory
    result = vkAllocateMemory(p_device, &mallocInfo, nullptr, &p_memory->deviceLocalMemory);
    if (result != VK_SUCCESS) {
        vkFreeMemory(p_device, p_memory->hostVisibleMemory, nullptr);
        sprintf(CcbErrMsg, "device local memory allocation failed with VkResult %i", result);
        return -1;
    }

    return 0;
}

static inline int
memoryInitMmap(struct CCBMemory* p_memory,
               VkDevice                p_device)
{
    int result;

    struct VkMemoryMapInfo info = {
        .sType  = VK_STRUCTURE_TYPE_MEMORY_MAP_INFO,
        .pNext  = nullptr,
        .flags  = 0u,
        .memory = p_memory->hostVisibleMemory,
        .offset = 0ull,
        .size   = VK_WHOLE_SIZE
    };

    result = vkMapMemory2(p_device, &info, (void**)&p_memory->hostVisibleHostBase);
    p_memory->hostVisibleHostBase += CCB_PAGE_SIZE; // Warning!
    if (result != VK_SUCCESS) {
        sprintf(CcbErrMsg, "host visible memory map failed with VkResult %i", result);
        return -1;
    }

    return 0;
}

static inline int
memoryInitBindBuffers(struct CCBMemory* p_memory,
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
        sprintf(CcbErrMsg, "buffer binding failed with VkResult %i", result);
        return -1;
    }

    return 0;
}

static inline int
memoryInitInitPageTable(struct CCBMemory* p_memory,
                        uint64_t          p_size) // must be multiple of CCB_PAGE_SIZE
{
    if ((p_size & 0x1fffull) != 0ull) {
        sprintf(CcbErrMsg, "memory size is not a multiple of page size");
        return -1;
    }

    p_memory->numPages = p_size >> 13u;

    p_memory->freePagePool    = malloc(p_memory->numPages * sizeof(uint64_t));
    p_memory->freePagePoolTop = p_memory->numPages;
    if (p_memory->freePagePool == nullptr) {
        sprintf(CcbErrMsg, "failed to allocate available page pool");
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
        sprintf(CcbErrMsg, "failed to allocate memory for memcpy regions");
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
ccbMemoryInit(struct CCBContext* p_context,
              struct CCBMemory*  p_memory,
              uint64_t           p_size)
{
    int result;

    // The only invalidation for a CCBMemory object
    p_memory->transferQueue = VK_NULL_HANDLE;

    // Retrieve transfer queue from VkDevice
    struct VkDeviceQueueInfo2 queueInfo = {
        .sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_INFO_2,
        .pNext            = nullptr,
        .flags            = 0u,
        .queueFamilyIndex = p_context->transferQueueFamilyIndex,
        .queueIndex       = 0u,
    };
    vkGetDeviceQueue2(p_context->device, &queueInfo, &p_memory->transferQueue);

    uint64_t hs = p_size;
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

    // Get device address
    struct VkBufferDeviceAddressInfo info = {
        .sType  = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
        .pNext  = nullptr,
        .buffer = s_hostVisibleBuffer
    };
    p_memory->hostVisibleDeviceBase = vkGetBufferDeviceAddress(p_context->device, &info)
                                    + 0x2000ull & -0x2000ull;
    info.buffer = s_deviceLocalBuffer;
    p_memory->deviceLocalDeviceBase = vkGetBufferDeviceAddress(p_context->device, &info)
                                    + 0x2000ull & -0x2000ull;

    result = memoryInitInitPageTable(p_memory, p_size - CCB_PAGE_SIZE);
    if (result != 0) {
        goto OnPostMmapError;
    }
    
    // Create transfer command pool
    struct VkCommandPoolCreateInfo cmdPoolInfo = {
        .sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .pNext            = nullptr,
        .flags            = 0u,
        .queueFamilyIndex = p_context->transferQueueFamilyIndex,
    };
    result = vkCreateCommandPool(p_context->device, &cmdPoolInfo, nullptr, &p_memory->transferCmdPool);
    if (result != VK_SUCCESS) {
        sprintf(CcbErrMsg, "failed to create transfer command pool with VkResult %i", result);
        goto OnPostMmapError;
    }
    
    // Allocate transfer command buffer
    struct VkCommandBufferAllocateInfo cmdBufInfo = {
        .sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .pNext              = nullptr,
        .commandPool        = p_memory->transferCmdPool,
        .level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1u // reserved
    };
    result = vkAllocateCommandBuffers(p_context->device, &cmdBufInfo, &p_memory->transferCmdBuffer);
    if (result != VK_SUCCESS) {
        sprintf(CcbErrMsg, "failed to allocate transfer command buffer with VkResult %i", result);
        goto OnAllocateCmdBufferError;
    }

    return 0;

OnAllocateCmdBufferError:
    vkDestroyCommandPool(p_context->device, p_memory->transferCmdPool, nullptr);

OnPostMmapError:
    struct VkMemoryUnmapInfo munmapInfo = {
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

OnPreMallocError:
    // Invalidate again, prevent future free on nullptr
    p_memory->transferQueue = VK_NULL_HANDLE;
    return -1;
}

inline void
ccbMemoryDestroy(struct CCBContext* p_context,
                 struct CCBMemory*  p_memory)
{
    if (p_memory->transferQueue == VK_NULL_HANDLE) {
        return;
    }

    int result;

    vkDestroyBuffer(p_context->device, s_hostVisibleBuffer, nullptr);
    vkDestroyBuffer(p_context->device, s_deviceLocalBuffer, nullptr);
    vkFreeCommandBuffers(p_context->device, p_memory->transferCmdPool, 1u, &p_memory->transferCmdBuffer);
    vkDestroyCommandPool(p_context->device, p_memory->transferCmdPool, nullptr);
    free((void*)p_memory->memcpyInfo.pRegions);
    free(p_memory->freePagePool);
    struct VkMemoryUnmapInfo munmapInfo = {
        .sType  = VK_STRUCTURE_TYPE_MEMORY_UNMAP_INFO,
        .pNext  = nullptr,
        .flags  = 0u,
        .memory = p_memory->hostVisibleMemory
    };
    result = vkUnmapMemory2(p_context->device, &munmapInfo);
    if (result != VK_SUCCESS) {
        sprintf(CcbErrMsg, "Host visible memory unmap failed with VkResult %i", result);
    }
    vkFreeMemory(p_context->device, p_memory->hostVisibleMemory, nullptr);
    vkFreeMemory(p_context->device, p_memory->deviceLocalMemory, nullptr);
}

inline int
ccbMemoryTransferBegin(struct CCBMemory*  p_memory,
                       struct CCBContext* p_context)
{
    int result;

    // Rewind memcpy region stack and free frame stack
    p_memory->memcpyInfo.regionCount = 0u;

    // Reset transfer command pool along with all allocated command buffers
    result = vkResetCommandPool(p_context->device, p_memory->transferCmdPool, 0u);
    if (result != VK_SUCCESS) {
        sprintf(CcbErrMsg, "failed to reset transfer command pool with VkResult %i", result);
        return -1;
    }

    // Begin recording transfer commands
    struct VkCommandBufferBeginInfo cmdBufBeginInfo = {
        .sType            = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .pNext            = nullptr,
        .flags            = 0u,
        .pInheritanceInfo = nullptr
    };
    result = vkBeginCommandBuffer(p_memory->transferCmdBuffer, &cmdBufBeginInfo);
    if (result != VK_SUCCESS) {
        sprintf(CcbErrMsg, "failed to begin transfer command buffer with VkResult %i", result);
        return -1;
    }

    return 0;
}

inline int
ccbMemoryTransferEnd(struct CCBMemory* p_memory)
{
    int result;

    // Record all memcpy regions
    vkCmdCopyMemoryKHR(p_memory->transferCmdBuffer, &p_memory->memcpyInfo);

    // End recording transfer commands
    result = vkEndCommandBuffer(p_memory->transferCmdBuffer);
    if (result != VK_SUCCESS) {
        sprintf(CcbErrMsg, "failed to end transfer command buffer with VkResult %i", result);
        return -1;
    }

    return 0;
}

inline void
ccbMemoryTransferFlush(struct CCBMemory*             p_memory,
                       struct VkSemaphoreSubmitInfo* p_waitInfo,
                       struct VkSemaphoreSubmitInfo* p_signalInfo)
{
    struct VkCommandBufferSubmitInfo cmdBufSubmitInfo = {
        .sType         = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
        .pNext         = nullptr,
        .commandBuffer = p_memory->transferCmdBuffer,
        .deviceMask    = 1u // reserved
    };
    struct VkSubmitInfo2 submitInfo = {
        .sType                    = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
        .pNext                    = nullptr,
        .flags                    = 0u,
        .waitSemaphoreInfoCount   = 1u,
        .pWaitSemaphoreInfos      = p_waitInfo,
        .commandBufferInfoCount   = 1u,
        .pCommandBufferInfos      = &cmdBufSubmitInfo,
        .signalSemaphoreInfoCount = 1u,
        .pSignalSemaphoreInfos    = p_signalInfo
    };
    vkQueueSubmit2(p_memory->transferQueue, 1u, &submitInfo, VK_NULL_HANDLE);
}

inline void
ccbMemoryUploadTensor(struct CCBMemory* p_memory,
                      struct CCBTensor* p_tensor,
                      uint64_t                p_deviceLocalBase)
{
    uint32_t numPagesRequired = p_tensor->size >> 12;
    for (uint32_t i = 0u; i < numPagesRequired; ++i) {
        auto r = (struct VkDeviceMemoryCopyKHR*)p_memory->memcpyInfo.pRegions + p_memory->memcpyInfo.regionCount;
        ++p_memory->memcpyInfo.regionCount;
        r->srcRange.address = p_tensor->hostBases[i];
        r->srcFlags         = VK_ADDRESS_COMMAND_FULLY_BOUND_BIT_KHR;
        r->dstRange.address = p_deviceLocalBase;
        r->dstFlags         = VK_ADDRESS_COMMAND_FULLY_BOUND_BIT_KHR | VK_ADDRESS_COMMAND_STORAGE_BUFFER_USAGE_BIT_KHR;
        p_deviceLocalBase += CCB_PAGE_SIZE;
    }
}

inline void
ccbMemoryDownloadTensor(struct CCBMemory* p_memory,
                        struct CCBTensor* p_tensor,
                        uint64_t                p_deviceLocalBase)
{
    uint32_t numPagesRequired = p_tensor->size >> 12;
    for (uint32_t i = 0u; i < numPagesRequired; ++i) {
        auto r = (struct VkDeviceMemoryCopyKHR*)p_memory->memcpyInfo.pRegions + p_memory->memcpyInfo.regionCount;
        ++p_memory->memcpyInfo.regionCount;
        r->srcRange.address = p_deviceLocalBase;
        r->srcFlags         = VK_ADDRESS_COMMAND_FULLY_BOUND_BIT_KHR | VK_ADDRESS_COMMAND_STORAGE_BUFFER_USAGE_BIT_KHR;
        r->dstRange.address = p_tensor->hostBases[i];
        r->dstFlags         = VK_ADDRESS_COMMAND_FULLY_BOUND_BIT_KHR;
        p_deviceLocalBase += CCB_PAGE_SIZE;
    }
}

inline void
ccbMemoryReleaseQueue(struct CCBMemory*  p_memory,
                      struct CCBContext* p_context)
{
    // struct VkMemoryRangeBarrierKHR barrier = {
    //     .sType               = VK_STRUCTURE_TYPE_MEMORY_RANGE_BARRIER_KHR,
    //     .pNext               = nullptr,
    //     .srcStageMask        = VK_PIPELINE_STAGE_2_COPY_BIT,
    //     .srcAccessMask       = VK_ACCESS_2_TRANSFER_WRITE_BIT,
    //     .dstStageMask        = VK_PIPELINE_STAGE_2_NONE,
    //     .dstAccessMask       = VK_ACCESS_2_NONE,
    //     .srcQueueFamilyIndex = p_context->transferQueueFamilyIndex,
    //     .dstQueueFamilyIndex = p_context->computeQueueFamilyIndex,
    //     .addressRange        = {p_memory->deviceLocalDeviceBase, },
    //     .addressFlags        = VK_ADDRESS_COMMAND_FULLY_BOUND_BIT_KHR | VK_ADDRESS_COMMAND_STORAGE_BUFFER_USAGE_BIT_KHR
    // };
    // struct VkMemoryRangeBarriersInfoKHR info = {
    //     .sType                   = VK_STRUCTURE_TYPE_MEMORY_RANGE_BARRIERS_INFO_KHR,
    //     .pNext                   = nullptr,
    //     .memoryRangeBarrierCount = 1u,
    //     .pMemoryRangeBarriers    = &barrier
    // };
    // struct VkDependencyInfo dep = {VK_STRUCTURE_TYPE_DEPENDENCY_INFO, &info};
    // vkCmdPipelineBarrier2(p_memory->transferCmdBuffer, &dep);
}

inline void
ccbMemoryPrint(struct CCBMemory* p_memory,
               FILE*                   p_fp)
{
    fprintf(p_fp, "[Calcubrute Info] Memory:\n"
                  "|__ Host Visible Memory Host Base: 0x%p\n"
                  "|__ Host Visible Memory Device Base: 0x%llx\n"
                  "|__ Device Local Memory Device Base: 0x%llx\n"
                  "|__ Number of Pages: %u\n"
                  "|__ Number of Pages Consumed: %u\n",
                  p_memory->hostVisibleHostBase,
                  p_memory->hostVisibleDeviceBase,
                  p_memory->deviceLocalDeviceBase,
                  p_memory->numPages,
                  p_memory->numPages - p_memory->freePagePoolTop);
}
