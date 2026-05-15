#include <JK/Calcubrute.h>

int
main(int p_numArgs, char** p_args)
{
    int result;

    result = volkInitialize();
    if (result != VK_SUCCESS) {
        fprintf(stderr, "[Calcubrute Error] Failed to initialize volk\n");
        return -1;
    }

    VkInstance instance;
    struct VkApplicationInfo appInfo = {
        .sType            = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pNext            = nullptr,
        .apiVersion       = VK_API_VERSION_1_4
    };
    struct VkInstanceCreateInfo instanceInfo = {
        .sType            = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pNext            = nullptr,
        .pApplicationInfo = &appInfo,
    };
    vkCreateInstance(&instanceInfo, nullptr, &instance);
    volkLoadInstance(instance);
    struct CCBContext* pContext = malloc(sizeof(struct CCBContext));
    struct CCBMemory*  pMemory  = malloc(sizeof(struct CCBMemory));
    ccbContextInit(pContext, instance, 0u);
    ccbMemoryInit(pContext, pMemory, 1ull << 24);

    struct CCBTensor2D X, Y, Z;
    ccbTensor2DAllocate(&Y, pMemory, 64u, 128u);
    ccbTensor2DAllocate(&X, pMemory, 4096u, 1024u);
    
    // puts("");
    // ccbContextPrint(pContext, stdout);
    puts("");
    ccbMemoryPrint(pMemory, stdout);
    puts("Y:");
    ccbTensor2DPrint(&Y, stdout);
    puts("X:");
    ccbTensor2DPrint(&X, stdout);

    ccbTensor2DFree(&Y, pMemory);

    puts("");
    ccbMemoryPrint(pMemory, stdout);

    ccbTensor2DAllocate(&Z, pMemory, 256u, 256u);

    puts("");
    ccbMemoryPrint(pMemory, stdout);
    puts("Z:");
    ccbTensor2DPrint(&Z, stdout);

    ccbTensor2DFree(&X, pMemory);
    ccbTensor2DFree(&Z, pMemory);

    // VkCommandPool   cmdPool;
    // VkCommandBuffer cmdBuf;

    // const struct VkCommandPoolCreateInfo cmdPoolInfo = {
    //     .sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
    //     .pNext            = nullptr,
    //     .flags            = 0u,
    //     .queueFamilyIndex = pContext->transferQueueFamilyIndex
    // };
    // result = vkCreateCommandPool(pContext->device, &cmdPoolInfo, nullptr, &cmdPool);

    // const struct VkCommandBufferAllocateInfo cmdBufInfo = {
    //     .sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
    //     .pNext              = nullptr,
    //     .commandPool        = cmdPool,
    //     .level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
    //     .commandBufferCount = 1u
    // };
    // result = vkAllocateCommandBuffers(pContext->device, &cmdBufInfo, &cmdBuf);

    // const struct VkCommandBufferBeginInfo beginInfo = {
    //     .sType            = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
    //     .pNext            = nullptr,
    //     .flags            = 0u,
    //     .pInheritanceInfo = nullptr
    // };
    // result = vkBeginCommandBuffer(cmdBuf, &beginInfo);
    // // uint32_t* base = (uint32_t*)pMemory->hostVisibleHostBase;
    // // for (uint32_t i = 0u; i < 10u; ++i) {
    // //     base[i] = i;
    // // }
    // const struct VkDeviceAddressRangeKHR range = {pMemory->deviceLocalDeviceBase, 9 * sizeof(uint32_t)};
    // vkCmdFillMemoryKHR(cmdBuf, &range, VK_ADDRESS_COMMAND_FULLY_BOUND_BIT_KHR, 114514u);
    // const struct VkMemoryRangeBarrierKHR barrier = {
    //     .sType               = VK_STRUCTURE_TYPE_MEMORY_RANGE_BARRIER_KHR,
    //     .pNext               = nullptr,
    //     .srcStageMask        = VK_PIPELINE_STAGE_2_CLEAR_BIT,
    //     .srcAccessMask       = VK_ACCESS_2_TRANSFER_WRITE_BIT,
    //     .dstStageMask        = VK_PIPELINE_STAGE_2_COPY_BIT,
    //     .dstAccessMask       = VK_ACCESS_2_TRANSFER_READ_BIT,
    //     .srcQueueFamilyIndex = pContext->transferQueueFamilyIndex,
    //     .dstQueueFamilyIndex = pContext->transferQueueFamilyIndex,
    //     .addressRange        = {pMemory->hostVisibleDeviceBase, CCB_PAGE_SIZE},
    //     .addressFlags        = VK_ADDRESS_COMMAND_FULLY_BOUND_BIT_KHR
    // };
    // const struct VkDependencyInfo dep = {VK_STRUCTURE_TYPE_DEPENDENCY_INFO, &barrier};
    // vkCmdPipelineBarrier2(cmdBuf, &dep);
    // download(pMemory, pMemory->deviceLocalDeviceBase);
    // ccbMemorySync(pMemory, cmdBuf);
    // result = vkEndCommandBuffer(cmdBuf);

    // const struct VkCommandBufferSubmitInfo cmdBufSubmitInfo = {
    //     .sType         = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
    //     .pNext         = nullptr,
    //     .commandBuffer = cmdBuf,
    //     .deviceMask    = 1u
    // };
    // const struct VkSubmitInfo2 submitInfo = {
    //     .sType                  = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
    //     .pNext                  = nullptr,
    //     .flags                  = 0u,
    //     .commandBufferInfoCount = 1u,
    //     .pCommandBufferInfos    = &cmdBufSubmitInfo
    // };
    // result = vkQueueSubmit2(pContext->transferQueue, 1u, &submitInfo, VK_NULL_HANDLE);
    // result = vkQueueWaitIdle(pContext->transferQueue);

    // auto base = (uint32_t*)pMemory->hostVisibleHostBase;
    // for (uint32_t i = 0u; i < 10u; ++i) {
    //     printf("%u: %u\n", i + 1u, base[i]);
    // }
    
    // vkDestroyCommandPool(pContext->device, cmdPool, nullptr);

CleanUp:
    ccbMemoryDestroy(pContext, pMemory);
    ccbContextDestroy(pContext);
    free(pContext);
    vkDestroyInstance(instance, nullptr);
    return 0;
}
