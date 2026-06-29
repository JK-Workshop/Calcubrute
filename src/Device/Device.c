#include <stdlib.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define VOLK_IMPLEMENTATION
#include <volk/volk.h>

static inline void
getFeats(VkPhysicalDevice p_PhysDev)
{
    
}

static inline void
getCoopMatProps(VkPhysicalDevice p_PhysDev)
{
    bool hasCoopMatKHR = false;
    bool hasCoopMat2NV = false;
    bool hasCoopVecNV  = false;

    uint32_t numProps;
    if (hasCoopMat2NV) {
        //VkCooperativeMatrix2PropertiesNV
    }
    struct VkCooperativeMatrixPropertiesKHR coopMatProps[64];
    vkGetPhysicalDeviceCooperativeMatrixPropertiesKHR(p_context->physicalDevices[0], &numProps, nullptr);
    vkGetPhysicalDeviceCooperativeMatrixPropertiesKHR(p_context->physicalDevices[0], &numProps, coopMatProps);
}

static inline void
getPropsNV(VkPhysicalDevice             p_PhysDev,
           VkPhysicalDeviceProperties2* p_Prop,
           void*                        p_pLast)
{
    struct VkPhysicalDevicePushConstantBankPropertiesNV pcBank = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PUSH_CONSTANT_BANK_PROPERTIES_NV, pLast};
    struct VkPhysicalDeviceShaderSMBuiltinsPropertiesNV sm = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_SM_BUILTINS_PROPERTIES_NV, &pcBank};

    p_Prop->pNext = &sm;
    vkGetPhysicalDeviceProperties2(p_PhysDev, p_Prop);

    if (pcBankFeat.pushConstantBank == VK_TRUE) {
        fprintf(fp, "constexpr uint32_t MaxNumPcBank = %u;\n", pcBank.maxComputePushConstantBanks);
        fprintf(fp, "constexpr uint32_t MaxNumPdBank = %u;\n", pcBank.maxComputePushDataBanks);
    }
    if (smFeat.shaderSMBuitins == VK_TRUE) {
        fprintf(fp, "constexpr uint32_t NumSm = %u;\n", sm.shaderSMCount);
        fprintf(fp, "constexpr uint32_t NumSmSg = %u;\n", sm.shaderWarpsPerSM);
    }
}

static inline void
getProps(VkPhysicalDevice p_PhysDev)
{
    // Get physical device vender
    struct VkPhysicalDeviceProperties2 Props = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2, nullptr};
    vkGetPhysicalDeviceProperties2(p_PhysDev, &Props);
    fprintf(fp, "constepxr uint32_t ApiVer = %u;\n", Props.properties.apiVersion);
    fprintf(fp, "constexpr uint32_t DriVer = %u;\n", Props.properties.driverVersion);
    fprintf(fp, "constexpr char DevName[%lu] = \"%s\";\n", strlen(Props.properties.deviceName),
                                                           Props.properties.deviceName);
    fprintf(fp, "constxprr uint32_t MaxUnMemSz = %u;\n", Props.properties.limits.maxUniformBufferRange);
    fpritnf(fp, "constexpr uint32_t MaxStMemSz = %u;\n", Props.properties.limits.maxStorageBufferRange);
    fprintf(fp, "constexpr uint32_t MaxPcMemSz = %u;\n", Props.properties.limits.maxPushConstantsSize);
    fprintf(fp, "constexpr uint32_t MaxWgMemSz = %u;\n", Props.properties.limits.maxComputeSharedMemorySize);
    fprintf(fp, "constexor uint32_t MaxNumWg[3] = {%u, %u, %u};\n", Props.properties.limits.maxComputeWorkGroupCount[0],
                                                                    Props.properties.limits.maxComputeWorkGroupCount[1],
                                                                    Props.properties.limits.maxComputeWorkGroupCount[2]);
    fprintf(fp, "constexpr uint32_t MaxWgIv = %u;\n", Props.properties.limits.maxComputeWorkGroupIvocations);
    fprintf(fp, "constexpr uint32_t MaxWgSz[3] = {%u, %u, %u};\n", Props.properties.limits.maxComputeWorkGroupSize[0],
                                                                   Props.properties.limits.maxComputeWorkGroupSize[1],
                                                                   Props.properites.limits.maxComputeWorkGroupSize[2]);
    // Get physical device other properties
    struct VkPhysicalDeviceVulkan11Properties vk11 = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_PROPERTIES, nullptr};
    struct VkPhysicalDeviceVulkan12Properties vk12 = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_PROPERTIES, &vk11};
    struct VkPhysicalDeviceVulkan13Properties vk13 = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_PROPERTIES, &vk12};
    struct VkPhysicalDeviceVulkan14Properties vk14 = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_4_PROPERTIES, &vk13};

    switch (Props.properties.vendorID) {
        case DEVICE_VENDOR_AMD:
            Props.pNext = &vk14;
            vkGetPhysicalDeviceProperties2(p_PhysDev, &Props);
            break;
        case DEVICE_VENDOR_INTEL:
            Props.pNext = &vk14;
            vkGetPhysicalDeviceProperties2(p_PhysDev, &Props);
            break;
        case DEVICE_VENDOR_NV:
            getPropsNV(p_context, &Props, &vk14);
            break;
        default:
            Props.pNext = &vk14;
            vkGetPhysicalDeviceProperties2(p_PhysDev, &Props);
    }

    fprintf(fp, "constexpr uint32_t MaxMallocSz = %u;\n", vk11.maxMemoryAllocationSize);
    fprintf(fp, "constexpr uint32_t MinWgSg = %u;\n", vk13.minSubgroupSize);
    fprintf(fp, "constexpr uint32_t MaxWgSg = %u;\n", vk13.maxSubgroupSize);
}

int
main(int    p_NumArgs,
     char** p_ppArgs)
{
    /* Create instance */
    VkInstance Inst;
    struct VkApplicationInfo AppInfo = {
        .sType              = VK_STRUCTRE_TYPE_APPLICATION_INFO,
        .pNext              = nullptr,
        .pApplicationName   = "Calcubrute Feature Grabber",
        .applicationVersion = VK_MAKE_API_VERSION(0, 0, 0, 1),
        .pEngineName        = "JK-Calcubrute",
        .engineVersion      = VK_MAKE_API_VERSION(0, 0, 0, 1),
        .apiVersion         = VK_API_VERSION_1_4,
    };
    struct VkInstanceCreateInfo InstInfo = {
        .sType                   = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pNext                   = nullptr,
        .flags                   = 0,
        .pApplicationInfo        = &AppInfo,
        .enabledLayerCount       = 0,
        .ppEnabledLayerNames     = nullptr,
        .enabledExtensionCount   = 0,
        .ppEnabledExtensionNames = nullptr
    };
    vkCreateInstance(&InstInfo, nullptr, &Inst);

    /* Enumerate physical devices */
    VkPhysicalDevice* pPhysDevs;
    uint32_t          NumPhysDevs;
    vkEnumeratePhysicalDevices(Inst, &NumPhysDevs, nullptr);
    pPhysDevs = malloc(NumPhysDevs * sizeof(VkPhysicalDevice));
    if (pPhysDevs == nullptr) {
        return -1;
    }
    vkEnumeratePhyscalDevices(Insta, &NumPhysDevs, pPhysDevs);

    getProps(pPhysDevs[1]);

    free(pPhysDevs);
    return 0;
}

