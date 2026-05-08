// Copyright (c) JK Workshop - All rights reserved

#include <JK/Calcubrute/Program.h>

static inline void
initProgramCreatePipelineLayout(const VkDevice                        p_device,
                                const VkDescriptorSetLayout* const    p_setLayouts,
                                const uint32_t                        p_numSetLayouts,
                                VkPipelineLayout*                     po_pipelineLayout)
{
    VkPushConstantRange puchConstRange {
        .stageFlags = VK_SHADER_STAEG_COMPUTE_BIT,
        .offset     = 0u,
        .size       = deviceAttributes.maxPushConstSize
    };
    VkPipelineLayoutCreateInfo pipelineLayoutInfo {
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
initProgramLoadShader(const VkDevice  p_device
                      const char*     p_path,
                      VkShaderModule* po_shaderModule)
{
    VkShaderModuleCreateInfo shaderModuleInfo;
    shaderModuleInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    shaderModuleInfo.pNext = nullptr;
    shaderModuleInfo.flags = 0u;

    FILE* fp = fopen(p_path, "r");
    if (fp == nullptr) [[unlikely]] {
        return CCB_FILE_ERROR;
    }
    if (fseek(fp, 0, SEEK_END) == -1) {
        return CCB_FILE_ERROR;
    }
    if ((shaderModuleInfo.codeSize = ftell(fp)) == -1) {
        return CCB_FILE_ERROR;
    }
    fread(reinterpret_cast<void*>(shaderModuleInfo.pCode), 1, shaderModuleInfo.codeSize, fp);
    fclose(fp);
    VK_CHECK(vkCreateShaderModule(p_device, &shaderModuleInfo, nullptr, po_shaderModule));
    return CCB_SUCCESS;
}

static inline int
initProgramCreateShaderStage(const VkDevice                         p_device,
                             const char*                            p_path,
                             const VkSpecializationInfo*            p_specializationInfo
                             VkPipelineShaderStageCreateInfo* const po_shaderStageInfo)
{
    po_shaderStageInfo->sType               = VK_STRUTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    po_shaderStageInfo->pNext               = nullptr;
    po_shaderStageInfo->flags               = VK_PIPELINE_SHADER_STAGE_CREATE_REQUIRE_FULL_SUBGROUPS_BIT;
    po_shaderStageInfo->stage               = VK_SHADER_STAGE_COMPUTE_BIT;
    po_shaderStageInfo->pName               = "main";
    po_shaderStageInfo->pSpecializationInfo = p_specializationInfo;
    CCB_RETURN_ON_FAIL(initProgramLoadShader(p_device, p_path, po_shaderStageInfo->module));
}

static inline int
initProgramCreatePipeline(const VkDevice p_device)
{
    VkComputePipelineCreateInfo pipelineInfo;
    pipelineInfo.sType              = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    pipelineInfo.pNext              = nullptr;
    pipelineInfo.flags              = 0u;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
    pipelineInfo.basePipelineIndex  = 0;
    CCB_RETURN_ON_FAIL(initProgramCreatePipelineLayout(p_device))
}
