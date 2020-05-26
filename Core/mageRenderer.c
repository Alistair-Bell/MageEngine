#include "mageAPI.h"

void *mageRendererAllocate()
{
    return malloc(sizeof(struct MAGE_RENDERER_STRUCT));
}
void mageRendererInitialise(mageRenderer *renderer, mageWindow *window, uint8_t *success)
{
    mageTryDumpSuccess(0, success);
    #if defined(MAGE_VULKAN)
        uint8_t flag;
        mageVulkanHandlerInitialise(&renderer->Handler, window, &flag);
    #endif
}
void *mageVulkanHandlerAllocate()
{
    return malloc(sizeof(struct MAGE_VULKAN_HANDLER_STRUCT));
}

#if defined(MAGE_VULKAN)
    
    static const char * const RequiredDeviceExtensions[] = { VK_KHR_SWAPCHAIN_EXTENSION_NAME, };
    static const uint32_t RequiredExtensionCount = 1;
    
    typedef struct MAGE_QUEUE_FAMILIES_STRUCT
    {
        uint32_t *GraphicsFamily;
        
        uint32_t GraphicsCount;


    } mageQueueFamilies;

    static void mageQueueInitialise(mageQueueFamilies *queue)
    {
        queue->GraphicsCount = 0;
        queue->GraphicsFamily = calloc(0, sizeof(uint32_t));
    }    
    static void mageQueuePush(mageQueueFamilies *queue, uint32_t graphicsValue)
    {
        queue->GraphicsCount++;
        queue->GraphicsFamily = calloc(graphicsValue, sizeof(uint32_t));
        queue->GraphicsFamily[queue->GraphicsCount -1] = graphicsValue;
    }
    static void mageDeviceFindQueue(VkPhysicalDevice device, mageQueueFamilies *queue)
    {
        uint32_t queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, NULL);

        VkQueueFamilyProperties *queueFamilies = calloc(queueFamilyCount, sizeof(VkQueueFamilyProperties));
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies);

        uint32_t i = 0;

        for (i = 0; i < queueFamilyCount; i++)
        {
            if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
            {
                mageQueuePush(queue, i);
                MAGE_LOG_CORE_INFORM("Graphics card graphics queue found\n", NULL);
            }
        }
    }
    static void mageQueueFree(mageQueueFamilies *queue)
    {
        mageFreeMethod(queue->GraphicsFamily); 
    }
    static void mageCreateInstance(mageVulkanHandler *handler, mageWindow *window, uint8_t *success)
    {
        mageTryDumpSuccess(0, success);    
        /*!************************
            Creating instance
        **************************/
        VkApplicationInfo applicationInformation;
        applicationInformation.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        applicationInformation.pNext = NULL;
        applicationInformation.pApplicationName = window->Title;
        applicationInformation.applicationVersion = VK_MAKE_VERSION(0, 1, 0);
        applicationInformation.pEngineName = "MAGE";
        applicationInformation.apiVersion = VK_MAKE_VERSION(1, 0, 26);

        VkInstanceCreateInfo createInformation;
        createInformation.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        createInformation.pNext = NULL;
        createInformation.flags = 0;
        createInformation.pApplicationInfo = &applicationInformation;
        
        createInformation.ppEnabledLayerNames = NULL;
        createInformation.enabledLayerCount = 0;
        
        #if defined (MAGE_GLFW)

            uint32_t glfwExtensionCount;
        
            const char **glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

            createInformation.enabledExtensionCount = glfwExtensionCount;
            createInformation.ppEnabledExtensionNames = glfwExtensions;
        
        #else
            createInformation.enabledExtensionCount = 0;
            createInformation.ppEnabledExtensionNames = NULL;
        #endif


        VkResult result = vkCreateInstance(&createInformation, NULL, &handler->Instance);

        if (result != VK_SUCCESS)
        {
            MAGE_LOG_CORE_FATAL_ERROR("Failed to create vulkan instance\n", NULL);
            return;
        }
        MAGE_LOG_CORE_INFORM("Vulkan instance has been created\n", NULL);
    

        mageTryDumpSuccess(1, success);
    }
    static void mageIsDeviceSuitable(VkPhysicalDevice device, uint8_t *suitable)
    {
        uint32_t flag;
        VkPhysicalDeviceProperties deviceProperties;
        VkPhysicalDeviceFeatures deviceFeatures;
        vkGetPhysicalDeviceProperties(device, &deviceProperties);
        vkGetPhysicalDeviceFeatures(device, &deviceFeatures);   
        
        flag = deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU && deviceFeatures.geometryShader;

        if (!flag) return;

        mageQueueFamilies families;
        mageQueueInitialise(&families);

        mageDeviceFindQueue(device, &families);

        if (families.GraphicsCount <= 0) return;


        mageQueueFree(&families);
        *suitable = 1;
    }
    static void magePickPhysicalDevice(mageVulkanHandler *handler)
    {
        uint32_t deviceCount, i;
        vkEnumeratePhysicalDevices(handler->Instance, &deviceCount, NULL);

        if (deviceCount <= 0)
        {
            MAGE_LOG_CORE_FATAL_ERROR("Unable to find any physical devices\n", NULL);
            return;
        }
        VkPhysicalDevice *devices = calloc(deviceCount, sizeof(VkPhysicalDevice));
        vkEnumeratePhysicalDevices(handler->Instance, &deviceCount, devices);

    
        uint8_t flag = 0;
        for (i = 0; i < deviceCount; i++)
        {
            mageIsDeviceSuitable(devices[i], &flag);

            if (flag)
            {
                handler->PhysicalDevice = devices[i];
                vkGetPhysicalDeviceProperties(handler->PhysicalDevice, &handler->PhysicalProperties);
                MAGE_LOG_CORE_INFORM("Physical device chosen %s\n", handler->PhysicalProperties.deviceName);
                break;
            }

        }
        if (!flag)
        {
            MAGE_LOG_CORE_FATAL_ERROR("Unable to find suitable device for use\n", NULL);
        }
        
        mageFreeMethod(devices);
    }
    static void mageCreateDevice(mageVulkanHandler *handler, uint8_t *success)
    {
        VkResult result = vkCreateDevice(handler->PhysicalDevice,
            &(VkDeviceCreateInfo) {
                .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
                .queueCreateInfoCount = 1,
                .pQueueCreateInfos = &(VkDeviceQueueCreateInfo) {
                .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
                .queueFamilyIndex = 0,
                .queueCount = 1,
                .pQueuePriorities = (float []) { 1.0f },
                },
                .enabledExtensionCount = RequiredExtensionCount,
                .ppEnabledExtensionNames = RequiredDeviceExtensions,
            },
            NULL,
            &handler->Device);

        if (result != VK_SUCCESS)
        {
            MAGE_LOG_CORE_FATAL_ERROR("Vulkan logical device has failed to be created\n", NULL);
            return;
        }
        MAGE_LOG_CORE_INFORM("Vulkan logical device has been created\n", NULL); 
    
        
    
    }
#endif

void mageVulkanHandlerInitialise(mageVulkanHandler *handler, mageWindow *window, uint8_t *success)
{
    #if defined(MAGE_VULKAN)
        mageCreateInstance(handler, window, NULL);
        magePickPhysicalDevice(handler);
        mageCreateDevice(handler, NULL);
    #endif
}
void mageVulkanHandlerCleanup(mageVulkanHandler *handler)
{
    #if defined(MAGE_VULKAN)
        vkDestroyDevice(handler->Device, NULL);
    #endif
}