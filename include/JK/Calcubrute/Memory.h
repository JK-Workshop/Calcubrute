// Copyright (c) JK Workshop - All rights reserved

#if !defined(JK_CALCUBRUTE_MEMORY_H)
#define JK_CALCUBRUTE_MEMORY_H

#include <JK/Calcubrute/Common.h>

struct CcbContext;

int
ccbMemoryAllocate(struct CcbContext* const p_context JK_NONNULL(),
                  const uint64_t           p_hostVisibleSize,
                  const uint64_t           p_deviceLocalSize);

void
ccbMemoryFree(struct CcbContext* const p_context JK_NONNULL());

#endif // JK_CALCUBRUTE_MEMORY_H
