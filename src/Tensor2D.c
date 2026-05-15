// Copyright (c) JK Workshop - All rights reserved

#include <JK/Calcubrute/Tensor2D.h>

inline int
ccbTensor2DAllocate(struct CCBTensor2D* const p_tensor2D,
                    struct CCBMemory* const   p_memory,
                    const uint32_t            p_dimX,
                    const uint32_t            p_dimY)
{
    const uint32_t numPagesRequired = p_dimX * p_dimY >> 12u;
    p_tensor2D->dimX = p_dimX;
    p_tensor2D->dimY = p_dimY;
    p_tensor2D->hostBases = malloc(numPagesRequired * sizeof(uint64_t));
    if (p_tensor2D->hostBases == nullptr) {
        sprintf(CcbErrorMessage, "failed to allocate page entries\n");
        return -1;
    }

    // Check for free page budget
    if (p_memory->freePagePoolTop + 1u < numPagesRequired) {
        sprintf(CcbErrorMessage, "out of device memory");
        free(p_tensor2D->hostBases);
        p_tensor2D->hostBases = nullptr;
        return -1;
    }

    // Consume free pages from p_memory->freePagePool
    p_memory->freePagePoolTop -= numPagesRequired;
    memcpy(p_tensor2D->hostBases,
           p_memory->freePagePool + p_memory->freePagePoolTop,
           numPagesRequired * sizeof(uint64_t));

    return 0;
}

inline void
ccbTensor2DFree(struct CCBTensor2D* const p_tensor2D,
                struct CCBMemory* const   p_memory)
{
    // Return pages to p_memory->freePagePool
    const uint32_t numPagesRequired = p_tensor2D->dimX * p_tensor2D->dimY >> 12u;
    memcpy(p_memory->freePagePool + p_memory->freePagePoolTop,
           p_tensor2D->hostBases,
           numPagesRequired * sizeof(uint64_t));
    p_memory->freePagePoolTop += numPagesRequired;

    // Free tensor local page list
    if (p_tensor2D->hostBases != nullptr) {
        free(p_tensor2D->hostBases);
        p_tensor2D->hostBases = nullptr;
    }
}

inline void
ccbTensor2DPrint(struct CCBTensor2D* const p_tensor2D,
                 FILE*                     p_fp)
{
    const uint32_t numPagesRequired = p_tensor2D->dimX * p_tensor2D->dimY >> 12u;
    fprintf(p_fp, "Dimension: %ux%u\n", p_tensor2D->dimX, p_tensor2D->dimY);
    fprintf(p_fp, "Host Bases (Pages Consumed):\n");
    for (uint32_t i = 0u; i < numPagesRequired; ++i) {
        fprintf(p_fp, "\t0x%llx\n", p_tensor2D->hostBases[i]);
    }
}
