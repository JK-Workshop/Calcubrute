// Copyright (c) JK Workshop - All rights reserved

#if !defined(JK_CALCUBRUTE_CONTEXT_H)
#define JK_CALCUBRUTE_CONTEXT_H

#include <JK/Calcubrute/Common.h>
#include <JK/Calcubrute/Memory.h>

//static constexpr int NUM_EXTENSIONS_REQUIRED = 1;
//
//static constexpr char
//EXTENSIONS_REQUIRED[NUM_EXTENSIONS_REQUIRED][VK_MAX_EXTENSION_NAME_SIZE] = {
//    VK_KHR_COOPERATIVE_MATRIX_EXTENSION_NAME,
//    VK_KHR_DEVICE_ADDRESS_COMMAND_EXTENSION_NAME
//};

//extern uint32_t CcbLog      = 0u;
//extern uint32_t CcbLogInfo  = 0u;
//extern uint32_t CcbLogError = 0u;

struct CcbContext
{
    VkDevice         device;
    VkPhysicalDevice physicalDevices[VK_MAX_DEVICE_GROUP_SIZE];
    uint32_t         numPhysicalDevices;
    VkDeviceMemory   hostVisibleMemory;
    VkDeviceMemory   deviceLocalMemory;
    uint8_t*         hostVisibleHostBase;
    uint64_t         hostVisibleDeviceBase;
    uint64_t         deviceLocalDeviceBase;
    //PageTable        pageTables[VK_MAX_DEVICE_GROUP_SIZE];
    VkQueue          transferQueue;
    VkQueue          computeQueue;
    VkSemaphore      timelineSemaphore;
    // Device properties
    uint32_t         vendorID;
    char             deviceName[VK_MAX_PHYSICAL_DEVICE_NAME_SIZE];
    char             driverName[VK_MAX_DRIVER_NAME_SIZE];
    char             driverInfo[VK_MAX_DRIVER_INFO_SIZE];
    uint32_t         vulkanVersion;
    uint32_t         maxPushConstSize;
    uint32_t         maxUniformBufferSize;
    uint32_t         maxWorkgroupMemorySize;
    uint64_t         maxMallocSize;
    uint32_t         minNumInvocationsPerSubgroup;
    uint32_t         maxNumInvocationsPerSubgroup;
    uint32_t         transferQueueFamilyIndex;
    uint32_t         computeQueueFamilyIndex;
    // VK_NV_push_constant_bank
    uint32_t         maxNumPushConstBanks;
    uint32_t         maxNumPushDataBanks;
    // VK_NV_shader_sm_builtins
    uint32_t         numStreamingMultiprocessors;
    uint32_t         numSubgroupsPerStreamingMultiprocessor;
}; // struct CcbContext

int
ccbContextInit(struct CcbContext* const p_context JK_NONNULL(),
               const VkInstance         p_instance,
               const uint32_t           p_deviceGroupIndex);

void
ccbContextDestroy(struct CcbContext* const p_context JK_NONNULL());

//void
//ccbContextEnableLog(const uint32_t p_logTypeFlags);

void
ccbContextPrint(const struct CcbContext* const p_context,
                FILE*                          p_fp);

#endif // JK_CALCUBRUTE_CONTEXT_H
