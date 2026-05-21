// Copyright (c) JK Workshop - All rights reserved

#if !defined(JK_CALCUBRUTE_TENSOR_H)
#define JK_CALCUBRUTE_TENSOR_H

#include <JK/Calcubrute/Common.h>

struct CCBContext;
struct CCBMemory;

struct CCBTensorPool
{
    VkDeviceMemory memory;    // invalidator
    VkBuffer       buffer;
    uint8_t*       hostBase;
    uint64_t*      pageBases;
    uint32_t       numPages;
}; // CCBTensorPool

struct CCBTensor
{
    uint64_t* hostBases;
    uint32_t  size;
}; // struct CCBTensor

int
ccbTensorPoolInit(struct CCBTensorPool* const p_tensorPool JK_NONNULL(),
                  struct CCBContext* const    p_context    JK_NONNULL(),
                  const uint64_t              p_size);

void
ccbTensorPoolDestroy(struct CCBTensorPool* const p_tensorPool JK_NONNULL(),
                     struct CCBContext* const    p_context    JK_NONNULL());

int
ccbTensorAllocate(struct CCBTensor* const p_tensor JK_NONNULL(),
                  struct CCBMemory* const p_memory JK_NONNULL(),
                  const uint32_t          p_size);

void
ccbTensorFree(struct CCBTensor* const p_tensor JK_NONNULL(),
              struct CCBMemory* const p_memory JK_NONNULL());

float16_t*
ccbTensorAccessPage(const struct CCBTensor* const p_tensor JK_NONNULL(),
                    const struct CCBMemory* const p_memory JK_NONNULL(),
                    const uint32_t                p_pageIndex);

void
ccbTensorPrint(struct CCBTensor* const p_tensor JK_NONNULL(),
               FILE*                   p_fp);

#endif // JK_CALCUBRUTE_TENSOR_H
