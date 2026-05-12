#include <JK/Calcubrute.h>

int
main(int p_numArgs, char** p_args)
{
    volkInitialize();
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
    VkInstance instance;
    vkCreateInstance(&instanceInfo, nullptr, &instance);
    volkLoadInstance(instance);
    auto pContext = malloc(sizeof(struct CcbContext));
    ccbContextInit(pContext, instance, 0u);
    ccbMemoryAllocate(pContext, 1ull << 20, 1ull << 20);
    ccbContextPrint(pContext, stdout);
    ccbContextDestroy(pContext);
    free(pContext);
    vkDestroyInstance(instance, nullptr);

    return 0;
}
