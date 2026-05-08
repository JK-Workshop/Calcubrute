#include <JK/Calcubrute.h>

int
main(int p_numArgs, char** p_args)
{
    VK_CHECK(volkInitialize());
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
    VK_CHECK(vkCreateInstance(&instanceInfo, nullptr, &instance));
    volkLoadInstance(instance);
    struct CcbContext context;
    int result = ccbInitContext(&context, instance, 0u);
    if (result != CCB_SUCCESS) {
        printf("%d\n", result);
        ccbDestroyContext(&context);
        return -1;
    }
    ccbMalloc(&context, 1ull << 20, 1ull << 20);
    //FILE* fp = fopen("info.txt", "w");
    //std::print(stdout, "{}\n", device);
    //fclose(fp);
    ccbDestroyContext(&context);
    vkDestroyInstance(instance, nullptr);

    return 0;
}
