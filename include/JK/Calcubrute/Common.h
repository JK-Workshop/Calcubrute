// Copyright (c) JK Workshop - All rights reserved

#if !defined(JK_CALCUBRUTE_COMMON_H)
#define JK_CALCUBRUTE_COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define VOLK_NO_PROTOTYPE
#include <volk/volk.h>

#include "Macro/Nonnull.h"

typedef _Float16 float16_t;
typedef __bf16   bfloat16_t;

constexpr int CCB_SUCCESS              =  0;
constexpr int CCB_MALLOC_ERROR         = -1;
constexpr int CCB_FILE_ERROR           = -2;
constexpr int CCB_EXTENSION_MISS       = -3;
constexpr int CCB_PHYSICAL_DEVICE_MISS = -4;
constexpr int CCB_QUEUE_FAMILY_MISS    = -5;
constexpr int CCB_ARGUMENT_ERROR       = -6;

#define CCB_RETURN_ON_FAIL(RESULT)\
    {\
        int r = (RESULT);\
        if (r != CCB_SUCCESS) {\
            return r;\
        }\
    }

#define CCB_FREE(VARIABLE)\
    if (VARIABLE != nullptr) {\
        free(VARIABLE);\
        VARIABLE = nullptr;\
    }

#define CCB_MALLOC(TYPE, VARIABLE, NUM)\
    TYPE* VARIABLE = malloc((NUM) * sizeof(TYPE));\
    if (VARIABLE == nullptr) {\
        return CCB_MALLOC_ERROR;\
    }

#define CCB_REALLOC(TYPE, VARIABLE, NUM)\
    CCB_FREE(VARIABLE)\
    VARIABLE = malloc((NUM) * sizeof(TYPE));\
    if (VARIABLE == nullptr) {\
        return CCB_MALLOC_ERROR;\
    }

#define CCB_INIT_VK_STRUCT_ARRAY(VARIABLE, NUM, S_TYPE, P_NEXT)\
    for (uint32_t i = 0u; i < (NUM); ++i) {\
        VARIABLE[i].sType = S_TYPE;\
        VARIABLE[i].pNext = P_NEXT;\
    }

#if defined(JK_DEBUG)
    #define VK_CHECK(RESULT)\
        if ((RESULT) != VK_SUCCESS) {\
            fprintf(stderr, "Vulkan error at %s, %s: %d\n", __FILE__, __func__, __LINE__);\
            exit(EXIT_FAILURE);\
        }
#else
    #define VK_CHECK(RESULT) RESULT
#endif // JK_DEBUG

#endif // JK_CALCUBRUTE_COMMON_H
