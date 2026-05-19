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

    struct CCBTensor2D X;
    ccbTensor2DAllocate(&X, pMemory, 128u, 64u);
    
    puts("Context");
    ccbContextPrint(pContext, stdout);
    puts("Memory");
    ccbMemoryPrint(pMemory, stdout);

    float16_t* p = ccbTensor2DAccessPage(&X, pMemory, 0);
    p[0] = 1.0;
    p[1] = 2.0;
    p[2] = 3.0;

    uint64_t deviceLocalBase = pMemory->deviceLocalDeviceBase;
    ccbMemoryTransferBegin(pMemory, pContext);
    ccbMemoryUploadTensor2D(pMemory, &X, deviceLocalBase);
    ccbMemoryTransferEnd(pMemory);

    struct VkSemaphoreSubmitInfo waitInfo = {
        .sType       = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
        .pNext       = nullptr,
        .semaphore   = pContext->timelineSemaphore,
        .value       = 0ull,
        .stageMask   = VK_SHADER_STAGE_COMPUTE_BIT,
        .deviceIndex = 1u
    };
    struct VkSemaphoreSubmitInfo signalInfo = {
        .sType       = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
        .pNext       = nullptr,
        .semaphore   = pContext->timelineSemaphore,
        .value       = 114514ull,
        .stageMask   = VK_SHADER_STAGE_COMPUTE_BIT,
        .deviceIndex = 1u
    };
    ccbMemoryTransferFlush(pMemory, &waitInfo, &signalInfo);

    uint64_t done = 114514ull;
    const struct VkSemaphoreWaitInfo wait = {
        .sType          = VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO,
        .pNext          = nullptr,
        .flags          = 0u,
        .semaphoreCount = 1u,
        .pSemaphores    = &pContext->timelineSemaphore,
        .pValues        = &done
    };
    result = vkWaitSemaphores(pContext->device, &wait, UINT32_MAX);
    if (result == VK_TIMEOUT) {
        fprintf(stderr, "Time out\n");
        goto CleanUp;
    }

    uint64_t value;
    vkGetSemaphoreCounterValue(pContext->device, pContext->timelineSemaphore, &value);
    printf("%llu\n", value);

    printf("%f, %f, %f\n", (float)p[0], (float)p[1], (float)p[2]);
    p[0] = p[1] = p[2] = 0.0;
    printf("%f, %f, %f\n", (float)p[0], (float)p[1], (float)p[2]);

    ccbMemoryTransferBegin(pMemory, pContext);
    ccbMemoryDownloadTensor2D(pMemory, &X, deviceLocalBase);
    ccbMemoryTransferEnd(pMemory);

    waitInfo.value = 114514ull;
    signalInfo.value = 1919810ull;
    ccbMemoryTransferFlush(pMemory, &waitInfo, &signalInfo);

    done = 1919810ull;
    result = vkWaitSemaphores(pContext->device, &wait, UINT32_MAX);
    if (result == VK_TIMEOUT) {
        fprintf(stderr, "Time out\n");
        goto CleanUp;
    }

    vkGetSemaphoreCounterValue(pContext->device, pContext->timelineSemaphore, &value);
    printf("%llu\n", value);

    printf("%f, %f, %f\n", (float)p[0], (float)p[1], (float)p[2]);

CleanUp:
    ccbTensor2DFree(&X, pMemory);
    ccbMemoryDestroy(pContext, pMemory);
    ccbContextDestroy(pContext);
    free(pMemory);
    free(pContext);
    vkDestroyInstance(instance, nullptr);
    return 0;
}
