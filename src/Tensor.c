// Copyright (c) JK Workshop - All rights reserved

#include <JK/Calcubrute/Context.h>
#include <JK/Calcubrute/Memory.h>
#include <JK/Calcubrute/Tensor.h>

inline int
ccbTensorPoolInit(struct CCBTensorPool* const p_tensorPool,
                  struct CCBContext* const    p_context,
                  const uint64_t              p_size)
{
    int result;

    // Allocate ::pageBases
    p_tensorPool->numPages = p_size >> 13;
    p_tensorPool->pageBases = malloc(p_tensorPool->numPages * sizeof(uint64_t));
    if (p_tensorPool->pageBases == nullptr) {
        sprintf(CcbErrMsg, "failed to allocate pageBases");
        goto OnAllocatePageBasesError;
    }

    // Allocate p_size bytes of host visible memory
    const struct VkMemoryAllocateFlagsInfo mallocFlagsInfo = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO,
        .pNext = nullptr,
        .flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT,
        .deviceMask = 0u // TODO
    };
    const struct VkMemoryAllocateInfo mallocInfo = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .pNext = &mallocFlagsInfo,
        .allocationSize  = p_size,
        .memoryTypeIndex = p_context->hostVisibleIndex
    };
    result = vkAllocateMemory(p_context->device, &mallocInfo, nullptr, &p_tensorPool->memory);
    if (result != VK_SUCCESS) {
        sprintf(CcbErrMsg, "failed to allocate memory with VkResult %i", result);
        goto OnMallocError;
    }

    // Map memory to RAM
    const struct VkMemoryMapInfo mmapInfo = {
        .sType  = VK_STRUCTURE_TYPE_MEMORY_MAP_INFO,
        .pNext  = nullptr,
        .flags  = 0u,
        .memory = p_tensorPool->memory,
        .offset = 0ull,
        .size   = p_size
    };
    result = vkMapMemory2KHR(p_context->device, &mmapInfo, (void*)&p_tensorPool->hostBase);
    if (result != VK_SUCCESS) {
        sprintf(CcbErrMsg, "failed to map memory with VkResult %i", result);
        goto OnMmapError;
    }

    // Create buffer for memory
    const struct VkBufferCreateInfo bufInfo = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0u,
        .usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT
               | VK_BUFFER_USAGE_TRANSFER_DST_BIT
               | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = 0u,
        .pQueueFamilyIndices   = nullptr
    };
    result = vkCreateBuffer(p_context->device, &bufInfo, nullptr, &p_tensorPool->buffer);
    if (result != VK_SUCCESS) {
        sprintf(CcbErrMsg, "failed to create buffer with VkResult %i", result);
        goto OnCreateBufferError;
    }

    // Bind buffer to memory
    const struct VkBindBufferMemoryInfo bindBufInfo = {
        .sType        = VK_STRUCTURE_TYPE_BIND_BUFFER_MEMORY_INFO,
        .pNext        = nullptr,
        .buffer       = p_tensorPool->buffer,
        .memory       = p_tensorPool->memory,
        .memoryOffset = 0ull
    };
    result = vkBindBufferMemory2(p_context->device, 1u, &bindBufInfo);
    if (result != VK_SUCCESS) {
        sprintf(CcbErrMsg, "failed to bind buffer with VkResult %i", result);
        goto OnBindBufferError;
    }

    // Get memory device address base from buffer
    const struct VkBufferDeviceAddressInfo bufAddrInfo = {
        .sType  = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
        .pNext  = nullptr,
        .buffer = p_tensorPool->buffer
    };
    p_tensorPool->pageBases[0] = vkGetBufferDeviceAddress(p_context->device, &bufAddrInfo);

    // Fill ::pageBases
    for (uint32_t i = 1u; i < p_tensorPool->numPages; ++i) {
        p_tensorPool->pageBases[i] = p_tensorPool->pageBases[i - 1] + CCB_PAGE_SIZE;
    }

    return 0;

OnBindBufferError:
    vkDestroyBuffer(p_context->device, p_tensorPool->buffer, nullptr);
OnCreateBufferError:
    const struct VkMemoryUnmapInfo munmapInfo = {
        .sType  = VK_STRUCTURE_TYPE_MEMORY_UNMAP_INFO,
        .pNext  = nullptr,
        .flags  = 0u,
        .memory = p_tensorPool->memory
    };
    vkUnmapMemory2KHR(p_context->device, &munmapInfo);
OnMmapError:
    vkFreeMemory(p_context->device, p_tensorPool->memory, nullptr);
OnMallocError:
    free(p_tensorPool->pageBases);
OnAllocatePageBasesError:
    p_tensorPool->memory = VK_NULL_HANDLE;
    return -1;
}

inline void
ccbTensorPoolDestroy(struct CCBTensorPool* const p_tensorPool,
                     struct CCBContext* const    p_context)
{
    if (p_tensorPool->memory == VK_NULL_HANDLE)
        return;
    vkDestroyBuffer(p_context->device, p_tensorPool->buffer, nullptr);
    const struct VkMemoryUnmapInfo munmapInfo = {
        .sType  = VK_STRUCTURE_TYPE_MEMORY_UNMAP_INFO,
        .pNext  = nullptr,
        .flags  = 0u,
        .memory = p_tensorPool->memory
    };
    vkUnmapMemory2KHR(p_context->device, &munmapInfo);
    vkFreeMemory(p_context->device, p_tensorPool->memory, nullptr);
    free(p_tensorPool->pageBases);
    p_tensorPool->memory = VK_NULL_HANDLE;
}

inline int
ccbTensorAllocate(struct CCBTensor* const p_tensor,
                  struct CCBMemory* const p_memory,
                  const uint32_t          p_size)
{
    // 4096 align
    p_tensor->size = p_size + ((p_size & 4095) ? 4096u : 0u) & -4096;
    const uint32_t numPagesRequired = p_tensor->size >> 12;

    p_tensor->hostBases = malloc(numPagesRequired * sizeof(uint64_t));
    if (p_tensor->hostBases == nullptr) {
        sprintf(CcbErrMsg, "failed to allocate page entries\n");
        return -1;
    }

    // Check for free page budget
    if (p_memory->freePagePoolTop < numPagesRequired) {
        sprintf(CcbErrMsg, "out of host visible memory");
        free(p_tensor->hostBases);
        p_tensor->hostBases = nullptr;
        return -1;
    }

    // Consume free pages from p_memory->freePagePool, store in p_tensor->hostBases
    p_memory->freePagePoolTop -= numPagesRequired;
    memcpy(p_tensor->hostBases,
           p_memory->freePagePool + p_memory->freePagePoolTop,
           numPagesRequired * sizeof(uint64_t));

    return 0;
}

inline void
ccbTensorFree(struct CCBTensor* const p_tensor,
              struct CCBMemory* const p_memory)
{
    const uint32_t numPagesRequired = p_tensor->size >> 12;

    // Return pages to p_memory->freePagePool, from p_tensor->hostBases
    memcpy(p_memory->freePagePool + p_memory->freePagePoolTop,
           p_tensor->hostBases,
           numPagesRequired * sizeof(uint64_t));
    p_memory->freePagePoolTop += numPagesRequired;

    // Free tensor local page list
    if (p_tensor->hostBases != nullptr) {
        free(p_tensor->hostBases);
        p_tensor->hostBases = nullptr;
    }
}

inline float16_t*
ccbTensorAccessPage(const struct CCBTensor* const p_tensor,
                    const struct CCBMemory* const p_memory,
                    const uint32_t                p_pageIndex)
{
    const int64_t addOn = (int64_t)p_memory->hostVisibleHostBase
                        - p_memory->hostVisibleDeviceBase;
    return (float16_t*)(p_tensor->hostBases[p_pageIndex] + addOn);
}

inline void
ccbTensorPrint(struct CCBTensor* const p_tensor,
               FILE*                   p_fp)
{
    const uint32_t numPagesRequired = p_tensor->size >> 12;
    fprintf(p_fp, "Size: %u\n"
                  "Number of pages required: %u\n"
                  "Host Bases (Pages Consumed):\n",
                  p_tensor->size, numPagesRequired);
    for (uint32_t i = 0u; i < numPagesRequired; ++i) {
        fprintf(p_fp, "\t0x%llx\n", p_tensor->hostBases[i]);
    }
}
