// Copyright (c) JK Workshop - All rights reserved

#if !defined(JK_NONNULL_H)
#define JK_NONNULL_H

/*
    Function attribute forms:

        JK_NONNULL()
            All pointer parameters must be non-null.

        JK_NONNULL(1)
            Parameter 1 must be non-null.

        JK_NONNULL(1, 3)
            Parameters 1 and 3 must be non-null.
*/

#if defined(__has_attribute)
    #if __has_attribute(nonnull)
        #define JK_HAS_NONNULL_ATTRIBUTE 1
    #endif
#endif

#if !defined(JK_HAS_NONNULL_ATTRIBUTE)
    #if defined(__GNUC__) || defined(__clang__)
        #define JK_HAS_NONNULL_ATTRIBUTE 1
    #else
        #define JK_HAS_NONNULL_ATTRIBUTE 0
    #endif
#endif

#if JK_HAS_NONNULL_ATTRIBUTE
    #define JK_NONNULL(...) __attribute__((nonnull __VA_OPT__((__VA_ARGS__))))
#else
    #define JK_NONNULL(...)
#endif

#endif // JK_NONNULL_H
