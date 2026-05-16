// Copyright (c) JK Workshop - All rights reserved

#if !defined(JK_CALCUBRUTE_PROGRAM_H)
#define JK_CALCUBRUTE_PROGRAM_H

#include <JK/Calcubrute/Common.h>

constexpr uint32_t CCB_PORGRAM_MAX_COMPILE_FLAG_SIZE = 128u;

struct 

struct CCBProgram
{
    VkPipeline       pipeline;
    VkPipelineLayout pipelineLayout;
    uint32_t         subgroupSize;
}; // struct Shader

extern struct VkSpecializationInfo                                g_shaderStageSpecializationInfo;
extern struct VkPipelineShaderStageRequiredSubgroupSizeCreateInfo g_shaderStageSgSizeInfo;
extern struct VkPipelineCreateFlags2CreateInfo                    g_pipelineFlagsInfo;
extern struct VkComputePipelineCreateInfo                         g_pipelineInfo;
extern void**                                                     g_pipelinePLast;
extern void**                                                     g_shaderStagePLast;

int
ccbProgramInit(struct CCBProgram* const p_program JK_NONNULL(),
               struct CCBContext* const p_context JK_NONNULL(),
               const char*              p_path    JK_NONNULL());

void
ccbProgramDestroy(struct CCBProgram* const p_program JK_NONNULL(),
                  struct CCBContext* const p_context JK_NONNULL());

void
ccbProgramLoadTensor2D(struct CCBProgram* const p_program JK_NONULL());

#endif // JK_CALCUBRUTE_PROGRAM_HPP
