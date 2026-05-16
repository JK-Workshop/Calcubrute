// Copyright (c) JK Workshop - All rights reserved

#if !defined(JK_CALCUBRUTE_TENSOR_2D_H)
#define JK_CALCUBRUTE_TENSOR_2D_H

#include <JK/Calcubrute/Common.h>

struct CCBMemory;

struct CCBTensor2D
{
    uint32_t  dimX;
    uint32_t  dimY;
    uint64_t* hostBases;
}; // struct CCBTensor2D

int
ccbTensor2DAllocate(struct CCBTensor2D* const p_tensor2D JK_NONNULL(),
                    struct CCBMemory* const   p_memory   JK_NONNULL(),
                    const uint32_t            p_dimX,
                    const uint32_t            p_dimY);

void
ccbTensor2DFree(struct CCBTensor2D* const p_tensor2D JK_NONNULL(),
                struct CCBMemory* const   p_memory   JK_NONNULL());

// ccbTensor2DAccess(struct CCBTensor2D* const p_tensor2D JK_NONNULL(),
//                   const uint32_t            p_index);

void
ccbTensor2DPrint(struct CCBTensor2D* const p_tensor2D JK_NONNULL(),
                 FILE*                     p_fp);

#endif // JK_CALCUBRUTE_TENSOR_2D_H
