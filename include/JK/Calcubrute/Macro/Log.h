// Copyright (c) JK Workshop - All rights reserved

#if !defined(JK_CALCUBRUTE_LOG_H)
#define JK_CALCUBRUTE_LOG_H

#if defined(CCB_LOG)
    #define CCB_LOG_INFO
    #define CCB_LOG_ERROR
#endif // CCB_LOG

#if defined(CCB_LOG_INFO)
    #define _CCB_LOG_INFO(FMT, ...) fprintf(stderr, FMT __VA_OPT__(,) __VA_ARGS__)
#else
    #define _CCB_LOG_INFO(FMT, ...)
#endif // CCB_LOG_INFO

#if defined(CCB_LOG_ERROR)
    #define _CCB_LOG_ERROR(FMT, ...) printf(FMT __VA_OPT__(,) __VA_ARGS__)
#else
    #define _CCB_LOG_ERROR(FMT, ...)
#endif // CCB_LOG_ERROR

#endif // JK_CALCUBRUTE_LOG_H
