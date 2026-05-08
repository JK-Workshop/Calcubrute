// Copyright (c) JK Workshop - All rights reserved

#if !defined(JK_CALCUBRUTE_PROGRAM_H)
#define JK_CALCUBRUTE_PROGRAM_H

#include <JK/Calcubrute/Common.h>

extern spv_target_env SPV_ENV;

struct CcbProgram
{
    VkPipeline       pipeline;
    VkPipelineLayout pipelineLayout;
}; // struct Shader

int
ccbInitProgram(struct CcbContext* const p_context,
               struct CcbProgram* const p_program,
               const char*              p_path);

void
ccbDestroyProgram(struct CcbProgram* const p_program);

int
ccbFixSubgroupSize(struct CcbProgram* const p_program,
                   const uint32_t           p_subgroupSize);

#endif // JK_CALCUBRUTE_PROGRAM_HPP
