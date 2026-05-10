// Copyright (c) JK Workshop - All rights reserved

#if !defined(JK_CALCUBRUTE_CONTEXT_H)
#define JK_CALCUBRUTE_CONTEXT_H

#include <JK/Calcubrute/Common.h>

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
    union {
        struct { // AMD
            uint32_t placeHolder;
        };
        struct { // INTEL
            uint32_t placeHolder1;
        };
        struct { // NV
            uint32_t maxNumPushConstBanks;
            uint32_t maxNumPushDataBanks;
            uint32_t numStreamingMultiprocessors;
            uint32_t numSubgroupsPerStreamingMultiprocessor;
        };
    };
}; // struct CcbContext

int
ccbContextInit(struct CcbContext* const p_context JK_NONNULL(),
               const VkInstance         p_instance,
               const uint32_t           p_deviceIndex);

void
ccbContextDestroy(struct CcbContext* const p_context JK_NONNULL());

//void
//ccbContextEnableLog(const uint32_t p_logTypeFlags);

void
ccbContextPrint(const struct CcbContext* const p_context,
                FILE*                          p_fp);

int
ccbMemoryAllocate(struct CcbContext* const p_context JK_NONNULL(),
                  const uint64_t           p_hostVisibleSize,
                  const uint64_t           p_deviceLocalSize);

void
ccbMemoryFree(struct CcbContext* const p_context JK_NONNULL());

#endif // JK_CALCUBRUTE_CONTEXT_H
