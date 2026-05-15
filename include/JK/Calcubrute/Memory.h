// Copyright (c) JK Workshop - All rights reserved

#if !defined(JK_CALCUBRUTE_MEMORY_H)
#define JK_CALCUBRUTE_MEMORY_H

#include <JK/Calcubrute/Common.h>

constexpr uint32_t CCB_PAGE_SIZE = 0x2000u;

struct CCBContext;

struct CCBMemory
{
    struct VkCopyDeviceMemoryInfoKHR memcpyInfo;
    VkDeviceMemory                   hostVisibleMemory;
    VkDeviceMemory                   deviceLocalMemory;
    uint8_t*                         hostVisibleHostBase;
    uint64_t                         hostVisibleDeviceBase;
    uint64_t                         deviceLocalDeviceBase;
    uint32_t                         numPages;
    int64_t                          pageToFrameAddOn;
    uint64_t*                        entryMap;
    uint64_t*                        freePagePool;
    uint32_t                         freePagePoolTop; // decrease to consume, increase to return
}; // struct CCBMemory

int
ccbMemoryInit(struct CCBContext* const p_context JK_NONNULL(),
              struct CCBMemory* const  p_memory  JK_NONNULL(),
              const uint64_t           p_size);

void
ccbMemoryDestroy(struct CCBContext* const p_context JK_NONNULL(),
                 struct CCBMemory* const  p_memory  JK_NONNULL());

void
ccbMemorySync(struct CCBMemory* const p_memory JK_NONNULL(),
              VkCommandBuffer         p_commandBuffer);

void
ccbMemoryPrint(struct CCBMemory* const p_memory JK_NONNULL(),
               FILE*                   p_fp);

void
upload(struct CCBMemory* const p_memory, const uint64_t p_pageBase);

void
download(struct CCBMemory* const p_memory, const uint64_t p_frameBase);

#endif // JK_CALCUBRUTE_MEMORY_H
