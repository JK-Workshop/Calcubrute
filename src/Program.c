// Copyright (c) JK Workshop - All rights reserved

#include <JK/Calcubrute/Program.h>

struct VkSpecializationInfo g_shaderStageSpecializationInfo = {
    .mapEntryCount = 0u,      // vary, optional
    .pMapEntries   = nullptr, // vary, optional
    .dataSize      = 0,       // vary, optional
    .pData         = nullptr  // vary, optional
};
struct VkPipelineShaderStageRequiredSubgroupSizeCreateInfo g_shaderStageSgSizeInfo = {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_REQUIRED_SUBGROUP_SIZE_CREATE_INFO, // fixed
    .pNext = nullptr                                                                     // vary, optional
};
struct VkPipelineCreateFlags2CreateInfo g_pipelineFlagsInfo = {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_CREATE_FLAGS_2_CREATE_INFO,     // fixed
    .pNext = nullptr,                                                   // vary, optional
    .flags = 0ull                                                       // vary, optional
};
struct VkComputePipelineCreateInfo g_pipelineInfo = {
    .sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,            // fixed
    .pNext = &g_pipelineFlagsInfo,                                      // fixed
    .flags = 0u,                                                        // fixed, deprecated
    .stage = {
        .sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADAER_STAGE_CREATE_INFO, // fixed
        .pNext  = nullptr,                                              // vary, optional
        .flags  = 0u,                                                   // vary, optional
        .stage  = VK_SHADER_STAGE_COMPUTE_BIT,                          // fixed
        .module = VK_NULL_HANDLE,                                       // vary, invalidate
        .pName  = "main",                                               // fixed
        .pSpecializationInfo = &g_shaderStageSpecializationInfo         // fixed
    },
    .layout = VK_NULL_HANDLE;                                           // vary, invalidate
    .basePipelineHandle = VK_NULL_HANDLE,                               // vary, optional
    .basePipelineIndex  = -1                                            // vary, optional
};
void** g_pipelinePLast    = g_pipelineFlagsInfo.pNext;
void** g_shaderStagePLast = g_pipelineInfo.stage.pNext;

static inline void
programInitCreatePipelineLayout(const VkDevice                     p_device,
                                const VkDescriptorSetLayout* const p_setLayouts,
                                const uint32_t                     p_numSetLayouts,
                                VkPipelineLayout*                  po_pipelineLayout)
{
    struct VkPushConstantRange puchConstRange = {
        .stageFlags = VK_SHADER_STAEG_COMPUTE_BIT,
        .offset     = 0u,
        .size       = deviceAttributes.maxPushConstSize
    };
    struct VkPipelineLayoutCreateInfo pipelineLayoutInfo = {
        .sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .pNext                  = nullptr,
        .flags                  = 0u,
        .setLayoutCount         = p_numSetLayouts,
        .pSetLayouts            = p_setLayouts,
        .pushConstantRangeCount = 1u,
        .pPushConstantRanges    = &pushConstRange
    };
    VK_CHECK(vkCreatePipelineLayout(p_device, &pipelineLayoutInfo, nullptr, po_pipelineLayout));
}

static inline int
programInitCreateShaderModule(const VkDevice  p_device,
                              const char*     p_path)
{
    struct VkShaderModuleCreateInfo shaderModuleInfo;
    shaderModuleInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    shaderModuleInfo.pNext = nullptr;
    shaderModuleInfo.flags = 0u;

    FILE* fp = fopen(p_path, "r");
    if (fp == nullptr) {
        return CCB_FILE_ERROR;
    }
    if (fseek(fp, 0, SEEK_END) == -1) {
        return CCB_FILE_ERROR;
    }
    if ((shaderModuleInfo.codeSize = ftell(fp)) == -1) {
        fclose(fp);
        return CCB_FILE_ERROR;
    }
    if (fread(shaderModuleInfo.pCode, 1, shaderModuleInfo.codeSize, fp) < shaderModuleInfo.codeSize) {
        fclose(fp);
        return CCB_FILE_ERROR;
    }
    fclose(fp);
    VK_CHECK(vkCreateShaderModule(p_device, &shaderModuleInfo, nullptr, &g_pipelineInfo.stage.module));
    return CCB_SUCCESS;
}

inline int
ccbProgramInit(struct CcbProgram* const           p_program,
               struct CCBContext* const           p_context,
               const VkDescriptorSetLayout* const p_setLayouts,
               const uint32_t                     p_numSetLayouts)
{
    CCB_RETURN_ON_FAIL(programInitCreateShaderModule(p_context->device, p_path));
    CCB_RETURN_ON_FAIL(programInitCreatePipelineLayout(p_context->device, p_setLayouts, p_numSetLayouts, &g_pipelineInfo.pipelineLayout));
    VK_CHECK(vkCreateComputePipelines(p_context->device, VK_NULL_HANDLE, 1, &g_pipelineInfo, nullptr, p_program->pipeline));
    po_program->pipelineLayout = g_pipelineInfo.layout;
    return CCB_SUCCESS;
}

inline void
ccbProgramDestroy(struct CcbProgram* const p_program,
                  struct CCBContext* const p_context)
{
    if (p_program->pipeline != VK_NULL_HANDLE) {
        vkDestroyPipeline(p_context->device, p_program->pipeline, nullptr);
        p_program->pipeline = VK_NULL_HANDLE;
    }
    if (p_program->pipelineLayout != VK_NULL_HANDLE) {
        vkDestroyPipelineLayout(p_context->device, p_program->pipelineLayout, nullptr);
        p_program->pipelineLayout = VK_NULL_HANDLE;
    }
}

inline void
ccbProgramClear(void)
{
    g_pipelineFlagsInfo.pNext  = nullptr;
    g_pipelineInfo.stage.pNext = nullptr;
    *g_pipelinePLast    = g_pipelineFlagsInfo.pNext;
    *g_shaderStagePLast = g_pipelineInfo.stage.pNext;
    
    g_shaderStageSpecializationInfo = {0u, nullptr, 0, nullptr};
    g_pipelineFlagsInfo.flags = 0ull;
}

inline void
ccbProgramTemplate(const struct VkSpecializationMapEntry* p_mapEntries,
                   const uint32_t                         p_numMapEntries,
                   const void*                            p_data,
                   const size_t                           p_dataSize)
{
    g_shaderStageSpecializationInfo.mapEntryCount = p_numMapEntries;
    g_shaderStageSpecializationInfo.pMapEntries   = p_mapEntries;
    g_shaderStageSpecializationInfo.dataSize      = p_dataSize;
    g_shaderStageSpecializationInfo.pData         = p_data;
}

inline void
ccbProgramFixSubgroupSize(const uint32_t p_subgroupSize)
{
    g_shaderStageSgSizeInfo.requiredSubgroupSize = p_subgroupSize;
    g_shaderStagePLast = &g_shaderStageSgSizeInfo;
    
}
