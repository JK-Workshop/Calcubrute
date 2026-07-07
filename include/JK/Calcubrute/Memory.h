// Copyright (c) JK Workshop - All rights reserved

#if !defined(JK_CALCUBRUTE_MEMORY_H)
#define JK_CALCUBRUTE_MEMORY_H

#include <JK/Calcubrute/Common.h>
#include <JK/Calcubrute/Tensor.h>

#define CCB_HAS_DEVICE_ADDRESS_COMMAND_KHR 1

#define CCB_PAGE_SIZE 8192
#define CCB_MUL_PAGE_SIZE(N) (N << 13)
#define CCB_DIV_PAGE_SIZE(N) (N >> 13)

struct CCBContext;

struct CCBMemory
{
#if CCB_HAS_DEVICE_ADDRESS_COMMAND_KHR
    struct VkCopyDeviceMemoryInfoKHR MemcpyInfo;
#else
    struct VkCopyBufferInfo2         MemcpyInfo;
#endif
    VkDeviceMemory                   HostVisibleMemory;
    VkDeviceMemory                   DeviceLocalMemory;
    uint8_t*                         HostVisibleHostBase;
    uint64_t                         HostVisibleDeviceBase;
    uint64_t                         DeviceLocalDeviceBase;
    uint32_t                         NumPages;
    uint64_t*                        EntryMap;
    uint64_t*                        FreePagePool;
    uint32_t                         FreePagePoolTop; // decrease to consume, increase to return
    VkCommandPool                    TransferCmdPool;
    VkCommandBuffer                  TransferCmdBuffer;
    VkQueue                          TransferQueue;
}; // struct CCBMemory

int
ccbMemoryInit(struct CCBContext*    p_Context    JK_NONNULL(),
              struct CCBMemory*     p_Memory     JK_NONNULL(),
              uint64_t              p_Size);

void
ccbMemoryDestroy(struct CCBContext*    p_Context    JK_NONNULL(),
                 struct CCBMemory*     p_Memory     JK_NONNULL());

int
ccbMemoryTransferBegin(struct CCBMemory*     p_Memory     JK_NONNULL(),
                       struct CCBContext*    p_Context    JK_NONNULL());

int
ccbMemoryTransferEnd(struct CCBMemory*    p_Memory    JK_NONNULL());

void
ccbMemoryTransferFlush(struct CCBMemory*                p_Memory    JK_NONNULL(),
                       struct VkSemaphoreSubmitInfo*    p_WaitInfo,
                       struct VkSemaphoreSubmitInfo*    p_SignalInfo);

void
ccbMemoryUploadTensor(struct CCBMemory*    p_Memory    JK_NONNULL(),
                      struct CCBTensor*    p_Tensor    JK_NONNULL(),
                      uint64_t             p_DeviceLocalBase);

void
ccbMemoryDownloadTensor(struct CCBMemory*    p_Memory    JK_NONNULL(),
                        struct CCBTensor*    p_Tensor    JK_NONNULL(),
                        uint64_t             p_DeviceLocalBase);

void
ccbMemoryReleaseQueue(struct CCBMemory*     p_Memory     JK_NONNULL(),
                      struct CCBContext*    p_Context    JK_NONNULL());

void
ccbMemoryPrint(struct CCBMemory*    p_Memory    JK_NONNULL(),
               int                  p_Fd)

#endif // JK_CALCUBRUTE_MEMORY_H
