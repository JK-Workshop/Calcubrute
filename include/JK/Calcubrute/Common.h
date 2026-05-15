// Copyright (c) JK Workshop - All rights reserved

#if !defined(JK_CALCUBRUTE_COMMON_H)
#define JK_CALCUBRUTE_COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <immintrin.h>

#define VOLK_NO_PROTOTYPE
#include <volk/volk.h>

#include "Macro/Nonnull.h"

typedef _Float16 float16_t;
typedef __bf16   bfloat16_t;

constexpr uint32_t CCB_MAX_ERROR_MESSAGE_LENGTH = 1024u;

extern char CcbErrorMessage[CCB_MAX_ERROR_MESSAGE_LENGTH];

#endif // JK_CALCUBRUTE_COMMON_H
