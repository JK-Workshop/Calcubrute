// Copyright (c) JK Workshop - All rights reserved

#include <JK/Calcubrute/Context.h>

static uint32_t                      s_numExtensions;
static struct VkExtensionProperties* s_extensions = nullptr;

struct DeviceAttributes deviceAttributes;

/**
 * @brief Retrieve the whole extension list from the specified p_physicalDevice
 * 
 * @param p_physicalDevice physical device to be queried on
 * @return int CCB_SUCCESS on success, CCB_MALLOC_ERROR on failure
 */
static inline int
initContextGetExtensions(const VkPhysicalDevice p_physicalDevice)
{
    VK_CHECK(vkEnumerateDeviceExtensionProperties(p_physicalDevice, nullptr, &s_numExtensions, nullptr));
    CCB_REALLOC(struct VkExtensionProperties, s_extensions, s_numExtensions); // Freed inside ccbInitContext
    VK_CHECK(vkEnumerateDeviceExtensionProperties(p_physicalDevice, nullptr, &s_numExtensions, s_extensions));
    return CCB_SUCCESS;
}

/**
 * @brief Loop through physical device groups and pick the one specified by p_deviceIndex.
 * 
 * @param po_numPhysicalDevices set to the number of physical devices in this group
 * @param po_physicalDeviceList set to the list of VkPhysicalDevice's in this group
 * @param p_instance VkInstance to be queried
 * @param p_deviceIndex index into detected device group list, specifying the group to be picked
 * @return int CCB_SUCCESS on success, CCB_ARGUMENT_ERROR, CCB_MALLOC_ERROR on failure
 */
static inline int
initContextSetPhysicalDevice(uint32_t* const   po_numPhysicalDevices,
                             VkPhysicalDevice* po_physicalDeviceList,
                             const VkInstance  p_instance,
                             const uint32_t    p_deviceIndex)
{
    // Invalidate
    *po_numPhysicalDevices = UINT32_MAX;

    // List and check device group properties
    uint32_t numDeviceGroups;
    VK_CHECK(vkEnumeratePhysicalDeviceGroups(p_instance, &numDeviceGroups, nullptr));
    if (p_deviceIndex >= numDeviceGroups) {
        return CCB_ARGUMENT_ERROR;
    }
    CCB_MALLOC(struct VkPhysicalDeviceGroupProperties, deviceGroupProps, numDeviceGroups);
    CCB_INIT_VK_STRUCT_ARRAY(deviceGroupProps, numDeviceGroups, VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_GROUP_PROPERTIES, nullptr);
    VK_CHECK(vkEnumeratePhysicalDeviceGroups(p_instance, &numDeviceGroups, deviceGroupProps));
    *po_numPhysicalDevices = deviceGroupProps[p_deviceIndex].physicalDeviceCount;
    memcpy(po_physicalDeviceList, deviceGroupProps[p_deviceIndex].physicalDevices, *po_numPhysicalDevices * sizeof(VkPhysicalDevice));
    CCB_FREE(deviceGroupProps);
    return CCB_SUCCESS;
}

/**
 * @brief 
 * 
 * @param p_physicalDevice 
 * @return int 
 */
static inline int
initContextSetQueueFamilyIndices(const VkPhysicalDevice p_physicalDevice)
{
    uint32_t numQueueFamilies;
    vkGetPhysicalDeviceQueueFamilyProperties2(p_physicalDevice, &numQueueFamilies, nullptr);
    struct VkQueueFamilyProperties2 queueFamilyProps[8];
    CCB_INIT_VK_STRUCT_ARRAY(queueFamilyProps, 8, VK_STRUCTURE_TYPE_QUEUE_FAMILY_PROPERTIES_2, nullptr);
    vkGetPhysicalDeviceQueueFamilyProperties2(p_physicalDevice, &numQueueFamilies, queueFamilyProps);

    // Invalidate
    deviceAttributes.transferQueueFamilyIndex = -1;
    deviceAttributes.computeQueueFamilyIndex  = -1;

    // Ties broken by ascending order of index
    for (int i = numQueueFamilies - 1; i >= 0; --i) {
        const uint32_t flags = queueFamilyProps[i].queueFamilyProperties.queueFlags;

        // Try locating dedicated transfer queue family
        if ((flags &
                (VK_QUEUE_TRANSFER_BIT | VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT)
            ) == VK_QUEUE_TRANSFER_BIT)
        {
            deviceAttributes.transferQueueFamilyIndex = i;
        }
        // If failed, then locate any queue family with transfer capability
        else if (deviceAttributes.transferQueueFamilyIndex == -1 && (flags & VK_QUEUE_TRANSFER_BIT)) {
            deviceAttributes.transferQueueFamilyIndex = i;
        }

        // Locate compute queue family
        if (flags & VK_QUEUE_COMPUTE_BIT) {
            deviceAttributes.computeQueueFamilyIndex = i;
        }
    }

    if (deviceAttributes.transferQueueFamilyIndex == -1 || deviceAttributes.computeQueueFamilyIndex  == -1) {
        return CCB_QUEUE_FAMILY_MISS;
    }
    return CCB_SUCCESS;
}

/**
 * @brief 
 * 
 * @param p_device 
 * @param po_timelineSemaphore 
 */
static inline void
initContextSetTimelineSemaphore(const VkDevice     p_device,
                                VkSemaphore* const po_timelineSemaphore)
{
    if (*po_timelineSemaphore != VK_NULL_HANDLE) {
        vkDestroySemaphore(p_device, *po_timelineSemaphore, nullptr);
        *po_timelineSemaphore = VK_NULL_HANDLE;
    }
    struct VkSemaphoreTypeCreateInfo semaphoreTypeInfo = {
        .sType         = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO,
        .pNext         = nullptr,
        .semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE,
        .initialValue  = 0ull
    };
    struct VkSemaphoreCreateInfo semaphoreInfo = {
        .sType         = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
        .pNext         = &semaphoreTypeInfo,
        .flags         = 0u
    };
    VK_CHECK(vkCreateSemaphore(p_device, &semaphoreInfo, nullptr, po_timelineSemaphore));
}

int
ccbInitContext(struct CcbContext* const p_context,
               const VkInstance         p_instance,
               const uint32_t           p_deviceIndex)
{
    // Invalidate
    p_context->device            = VK_NULL_HANDLE;
    p_context->timelineSemaphore = VK_NULL_HANDLE;

    CCB_RETURN_ON_FAIL(initContextSetPhysicalDevice(&p_context->numPhysicalDevices, p_context->physicalDevices, p_instance, p_deviceIndex));
    CCB_RETURN_ON_FAIL(initContextSetQueueFamilyIndices(p_context->physicalDevices[0]));

    float priority = 1.0f;
    struct VkDeviceQueueCreateInfo queueInfos[2] = {
        {
            .sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
            .pNext            = nullptr,
            .flags            = 0,
            .queueFamilyIndex = deviceAttributes.transferQueueFamilyIndex,
            .queueCount       = 1,
            .pQueuePriorities = &priority
        },
        {
            .sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
            .pNext            = nullptr,
            .flags            = 0,
            .queueFamilyIndex = deviceAttributes.computeQueueFamilyIndex,
            .queueCount       = 1,
            .pQueuePriorities = &priority
        }
    };
    struct VkDeviceGroupDeviceCreateInfo deviceGroupInfo = {
        .sType                = VK_STRUCTURE_TYPE_DEVICE_GROUP_DEVICE_CREATE_INFO,
        .pNext                = nullptr,
        .physicalDeviceCount  = p_context->numPhysicalDevices,
        .pPhysicalDevices     = p_context->physicalDevices
    };
    struct VkDeviceCreateInfo deviceInfo = {
        .sType                = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .pNext                = &deviceGroupInfo,
        .flags                = 0,
        .queueCreateInfoCount = 2,
        .pQueueCreateInfos    = queueInfos
    };
    VK_CHECK(vkCreateDevice(p_context->physicalDevices[0], &deviceInfo, nullptr, &p_context->device));
    volkLoadDevice(p_context->device);

    initContextSetTimelineSemaphore(p_context->device, &p_context->timelineSemaphore);

    CCB_FREE(s_extensions);
    return CCB_SUCCESS;
}

void
ccbDestroyContext(struct CcbContext* const p_context)
{
    if (p_context->hostVisibleMemory != VK_NULL_HANDLE) {
        ccbFree(p_context);
    }
    if (p_context->timelineSemaphore != VK_NULL_HANDLE) {
        vkDestroySemaphore(p_context->device, p_context->timelineSemaphore, nullptr);
        p_context->timelineSemaphore = VK_NULL_HANDLE;
    }
    if (p_context->device != VK_NULL_HANDLE) {
        vkDestroyDevice(p_context->device, nullptr);
        p_context->device = VK_NULL_HANDLE;
    }
}
