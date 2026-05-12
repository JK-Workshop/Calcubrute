// Copyright (c) JK Workshop - All rights reserved

#if !defined(JK_CALCUBRUTE_PROGRAM_H)
#define JK_CALCUBRUTE_PROGRAM_H

#include <JK/Calcubrute/Common.h>

struct CcbProgram
{
    VkPipeline       pipeline;
    VkPipelineLayout pipelineLayout;
}; // struct Shader

struct CcbProgramInfo

extern struct VkSpecializationInfo                                g_shaderStageSpecializationInfo;
extern struct VkPipelineShaderStageRequiredSubgroupSizeCreateInfo g_shaderStageSgSizeInfo;
extern struct VkPipelineCreateFlags2CreateInfo                    g_pipelineFlagsInfo;
extern struct VkComputePipelineCreateInfo                         g_pipelineInfo;
extern void**                                                     g_pipelinePLast;
extern void**                                                     g_shaderStagePLast;

int
ccbProgramInit(struct CcbProgram* const p_program JK_NONNULL(),
               struct CcbContext* const p_context JK_NONNULL(),
               const char*              p_path    JK_NONNULL());

void
ccbProgramDestroy(struct CcbProgram* const p_program JK_NONNULL(),
                  struct CcbContext* const p_context JK_NONNULL());

void
ccbProgramClear(void);

void
ccbProgramStaticParamerters(const struct VkSpecializationMapEntry* p_mapEntries JK_NONNULL(),
                            const uint32_t                         p_numMapEntries,
                            const void*                            p_data       JK_NONNULL(),
                            const size_t                           p_dataSize);

void
ccbProgramDynamicParameters()

void
ccbProgramFixSubgroupSize(const uint32_t p_subgroupSize);

void
ccbProgramVarySubgroupSize(void);

void
ccbProgramPrintStates(void);


#endif // JK_CALCUBRUTE_PROGRAM_HPP
