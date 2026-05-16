// Copyright (c) JK Workshop - All rights reserved

#if !defined(JK_CALCUBRUTE_CONTEXT_H)
#define JK_CALCUBRUTE_CONTEXT_H

#include <JK/Calcubrute/Common.h>
#include <JK/Calcubrute/Memory.h>

struct CCBContext
{
    VkPhysicalDevice  physicalDevices[VK_MAX_DEVICE_GROUP_SIZE];
    VkDevice          device;
    uint32_t          numPhysicalDevices;
    VkQueue           computeQueue;
    VkSemaphore       timelineSemaphore;
    // Device properties
    uint32_t          vendorID;
    char              deviceName[VK_MAX_PHYSICAL_DEVICE_NAME_SIZE];
    char              driverName[VK_MAX_DRIVER_NAME_SIZE];
    char              driverInfo[VK_MAX_DRIVER_INFO_SIZE];
    uint32_t          vulkanVersion;
    uint32_t          maxPushConstSize;
    uint32_t          maxUniformBufferSize;
    uint32_t          maxWorkgroupMemorySize;
    uint64_t          maxMallocSize;
    uint32_t          minNumInvocationsPerSubgroup;
    uint32_t          maxNumInvocationsPerSubgroup;
    uint32_t          transferQueueFamilyIndex;
    uint32_t          computeQueueFamilyIndex;
    // VK_NV_push_constant_bank
    uint32_t          maxNumPushConstBanks;
    uint32_t          maxNumPushDataBanks;
    // VK_NV_shader_sm_builtins
    uint32_t          numStreamingMultiprocessors;
    uint32_t          numSubgroupsPerStreamingMultiprocessor;
}; // struct CCBContext

int
ccbContextInit(struct CCBContext* const p_context JK_NONNULL(),
               const VkInstance         p_instance,
               const uint32_t           p_deviceGroupIndex);

void
ccbContextDestroy(struct CCBContext* const p_context JK_NONNULL());

//void
//ccbContextEnableLog(const uint32_t p_logTypeFlags);

void
ccbContextPrint(const struct CCBContext* const p_context,
                FILE*                          p_fp);

#endif // JK_CALCUBRUTE_CONTEXT_H
