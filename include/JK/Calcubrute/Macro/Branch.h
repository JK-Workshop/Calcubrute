// Copyright (c) JK Workshop - All rights reserved

#if !defined(JK_CALCUBRUTE_BRANCH_H)
#define JK_CALCUBRUTE_BRANCH_H

#if defined(__GNUC__) || defined(__clang__)
#   define JK_BRANCH_UNUSED __attribute__((unused))
#   define JK_BRANCH_COLD   __attribute__((cold))
#   define JK_BRANCH_HOT    __attribute__((hot))
#else
#   define JK_BRANCH_UNUSED
#   define JK_BRANCH_COLD
#   define JK_BRANCH_HOT
#endif

#endif // JK_CALCUBRUTE_BRANCH_H
