// Copyright (c) JK Workshop - All rights reserved

#if !defined(JK_CALCUBRUTE_MEMORY_H)
#define JK_CALCUBRUTE_MEMORY_H

#include <JK/Calcubrute/Common.h>
#include <JK/Calcubrute/Tensor1D.h>
#include <JK/Calcubrute/Tensor2D.h>

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
    uint64_t*                        entryMap;
    uint64_t*                        freePagePool;
    uint32_t                         freePagePoolTop; // decrease to consume, increase to return
    VkCommandPool                    transferCmdPool;
    VkCommandBuffer                  transferCmdBuffer;
    VkQueue                          transferQueue;
}; // struct CCBMemory

int
ccbMemoryInit(struct CCBContext* const p_context JK_NONNULL(),
              struct CCBMemory* const  p_memory  JK_NONNULL(),
              const uint64_t           p_size);

void
ccbMemoryDestroy(struct CCBContext* const p_context JK_NONNULL(),
                 struct CCBMemory* const  p_memory  JK_NONNULL());

int
ccbMemoryTransferBegin(struct CCBMemory* const  p_memory JK_NONNULL(),
                       struct CCBContext* const p_context JK_NONNULL());

int
ccbMemoryTransferEnd(struct CCBMemory* const p_memory JK_NONNULL());

void
ccbMemoryTransferFlush(struct CCBMemory* const             p_memory JK_NONNULL(),
                       const struct VkSemaphoreSubmitInfo* p_waitInfo,
                       const struct VkSemaphoreSubmitInfo* p_signalInfo);

void
ccbMemoryUploadTensor2D(struct CCBMemory* const   p_memory   JK_NONNULL(),
                        struct CCBTensor2D* const p_tensor2D JK_NONNULL(),
                        uint64_t                  p_deviceLocalBase);

void
ccbMemoryDownloadTensor2D(struct CCBMemory* const   p_memory   JK_NONNULL(),
                          struct CCBTensor2D* const p_tensor2D JK_NONNULL(),
                          uint64_t                  p_deviceLocalBase);

void
ccbMemoryPrint(struct CCBMemory* const p_memory JK_NONNULL(),
               FILE*                   p_fp);

#endif // JK_CALCUBRUTE_MEMORY_H
