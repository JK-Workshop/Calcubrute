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

struct DeviceAttributes
{
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
}; // struct DeviceAttributes

extern struct DeviceAttributes deviceAttributes;

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
}; // struct Device

constexpr uint32_t DEVICE_VENDOR_AMD   = 0x1002u;
constexpr uint32_t DEVICE_VENDOR_INTEL = 0x8086u;
constexpr uint32_t DEVICE_VENDOR_NV    = 0x10DEu;
//constexpr uint32_t DEVICE_VENDER_ARM   =

int
ccbInitContext(struct CcbContext* const      p_context,
               const VkInstance              p_instance,
               const uint32_t                p_deviceIndex);

void
ccbDestroyContext(struct CcbContext* const p_context);

int
ccbMalloc(struct CcbContext* const  p_context,
          const size_t              p_hostVisibleSize,
          const size_t              p_deviceLocalSize);

void
ccbFree(struct CcbContext* const p_context);

// void
// ccbPrintContext(const FILE* p_fp,
//                 const CCBContext& p_context) noexcept;
//     std::print(p_fp,
//                "Device: ({})x{}\n"
//                "Driver: {} {}\n"
//                "Vulkan Version: {}.{}.{}\n"
//                "Max Push Constant Size: {:#08x}\n"
//                "Max Uniform Buffer Size: {:#08x}\n"
//                "Max Workgroup Memory Size: {:#08x}\n"
//                "Max Memory Allocation Size: {:#16x}\n"
//                "Min Number of Invocations per Subgroup: {}\n"
//                "Max Number of Invocations per Subgroup: {}\n"
//                "Transfer Queue Family Index: {}\n"
//                "Compute Queue Family Index: {}\n",
//                deviceAttributes.deviceName, p_device.numPhysicalDevices,
//                deviceAttributes.driverName, deviceAttributes.driverInfo,
//                VK_API_VERSION_MAJOR(deviceAttributes.vulkanVersion),
//                VK_API_VERSION_MINOR(deviceAttributes.vulkanVersion),
//                VK_API_VERSION_PATCH(deviceAttributes.vulkanVersion),
//                deviceAttributes.maxPushConstSize,
//                deviceAttributes.maxUniformBufferSize,
//                deviceAttributes.maxWorkgroupMemorySize,
//                deviceAttributes.maxMallocSize,
//                deviceAttributes.minNumInvocationsPerSubgroup,
//                deviceAttributes.maxNumInvocationsPerSubgroup,
//                deviceAttributes.transferQueueFamilyIndex,
//                deviceAttributes.computeQueueFamilyIndex);
//         switch (deviceAttributes.vendorID) {
//             case DEVICE_VENDOR_AMD:
//                 break;
//             case DEVICE_VENDOR_INTEL:
//                 break;
//             case DEVICE_VENDOR_NV:
//                                      "Max Number of Push Constant Banks: {}\n"
//                                      "Max Number of Push Data Banks: {}\n"
//                                      "Number of Streaming Multiprocessors: {}\n"
//                                      "Number of Subgroups per Streaming Multiprocessor: {}\n",
//                                      deviceAttributes.maxNumPushConstBanks,
//                                      deviceAttributes.maxNumPushDataBanks,
//                                      deviceAttributes.numStreamingMultiprocessors,
//                                      deviceAttributes.numSubgroupsPerStreamingMultiprocessor);
//                 break;
//         }

#endif // JK_CALCUBRUTE_CONTEXT_H
