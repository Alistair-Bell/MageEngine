#include <mageAPI.h>

#if defined (MAGE_VULKAN)



static const char *mageRequiredExtensions[] = 
{
    VK_KHR_SWAPCHAIN_EXTENSION_NAME,
};
static const char *const mageRequiredLayers[] = 
{
    "VK_LAYER_KHRONOS_validation",
};

static VKAPI_ATTR VkBool32 VKAPI_CALL mageVulkanDebugCallback( VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT *callbackData, void *pUserData) 
{
    switch (messageType)
    {   
        case VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT:
            MAGE_LOG_CORE_WARNING("Validation Layers %s\n", callbackData->pMessage);
            break;
        case VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT:
            MAGE_LOG_CORE_FATAL_ERROR("Validation Layers : violation issue %s\n", callbackData->pMessage);
            break;
        case VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT:
            MAGE_LOG_CORE_WARNING("Validation Layers : performance issue %s\n", callbackData->pMessage);
            break; 
        default:
            MAGE_LOG_CORE_ERROR("Validation Layers : Unknown validation error\n", NULL);
            break;  
    }
    return VK_FALSE;
}
static VkResult mageCreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkDebugUtilsMessengerEXT *pDebugMessenger) 
{
    PFN_vkCreateDebugUtilsMessengerEXT function = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    if (function != NULL) 
    {
        return function(instance, pCreateInfo, pAllocator, pDebugMessenger);
    } 
    else 
    {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}
static void mageDestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator) 
{
    PFN_vkDestroyDebugUtilsMessengerEXT function = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    if (function != NULL) 
    {
        function(instance, debugMessenger, pAllocator);
    }
}
static uint8_t mageCheckValidationLayers(struct mageRenderer *renderer, struct mageWindow *window)
{
    uint32_t i, j;
    uint32_t layerCount;
    uint32_t layerFound = 0;

    vkEnumerateInstanceLayerProperties(&layerCount, NULL);
    VkLayerProperties *properties = calloc(layerCount, sizeof(VkLayerProperties));
    vkEnumerateInstanceLayerProperties(&layerCount, properties);

    for (i = 0; i < layerCount; i++)
    {
        for (j = 0; j < sizeof(mageRequiredLayers) / sizeof(const char *); j++)
        {
            if (strcmp(mageRequiredLayers[j], properties[i].layerName) == 0)
            {
                layerFound = 1;
                break;
            }
        }
    }
    if (!layerFound)
    {
        free(properties);
        MAGE_LOG_CORE_FATAL_ERROR("Required validation layers not found\n", NULL);
        return 0;
    }
    MAGE_LOG_CORE_INFORM("Required validation layers found\n", NULL);
    
    free(properties);
    return 1;
}
static char **mageGetRequiredExtensions(uint32_t *count)
{
    uint32_t debugCount = 1;
    uint32_t totalCount;
    uint32_t glfwCount;
    uint32_t i;
    char *debugExtensions[] = { VK_EXT_DEBUG_UTILS_EXTENSION_NAME };
    const char **glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwCount);
#if defined (MAGE_DEBUG)
    totalCount = glfwCount + debugCount;
#else
    totalCount = glfwCount;
#endif

    char **extensions = calloc(totalCount, sizeof(char *));

    for (i = 0; i < totalCount; i++)
    {
        if (i < glfwCount)
        {
            extensions[i] = (char *)glfwExtensions[i];
        }
#if defined (MAGE_DEBUG)
        else
        {
            extensions[i] = (char *)debugExtensions[i - glfwCount];
        }
#endif
        MAGE_LOG_CORE_INFORM("Required extensions %s (%d of %d)\n", extensions[i], i + 1, totalCount);
    }   
    *count = totalCount;
    return extensions;
}
static void magePopulateValidationLayerCallback(VkDebugUtilsMessengerCreateInfoEXT *info)
{
    memset(info, 0, sizeof(VkDebugUtilsMessengerCreateInfoEXT));
    info->sType                 = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    info->messageSeverity       = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    info->messageType           = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    info->pfnUserCallback       = mageVulkanDebugCallback;
}
static VkResult mageSetupValidationLayerCallback(struct mageRenderer *renderer, struct mageWindow *window, struct mageRendererProps *props)
{
    magePopulateValidationLayerCallback(&renderer->DebugMessengerCreateInfo);
    VkResult result = MAGE_CHECK_VULKAN(mageCreateDebugUtilsMessengerEXT(renderer->Instance, &renderer->DebugMessengerCreateInfo, NULL, &renderer->DebugMessenger)); 
    return result;
}
static uint8_t mageIsDeviceSuitable(struct mageRenderer *renderer, VkPhysicalDevice device)
{
    struct mageIndiciesIndexes indicies;
    mageResult result = mageGetDeviceIndexes(renderer, device, &indicies);
    if (result != MAGE_SUCCESS)
    {
        mageIndiciesIndexesDestroy(&indicies);
        return 0;
    }
    VkBool32 supported;
    vkGetPhysicalDeviceSurfaceSupportKHR(device, indicies.GraphicIndexes[indicies.GraphicIndexesCount - 1], renderer->Surface, &supported);
    
    if (!supported)
    {
        mageIndiciesIndexesDestroy(&indicies);
        return 0;
    }

    mageIndiciesIndexesDestroy(&indicies);
    return 1;
}
static uint32_t mageRateDevice(struct mageRenderer *renderer, VkPhysicalDevice device)
{
    uint32_t score = 0;
    
    if (!mageIsDeviceSuitable(renderer, device))
    {
        return score;
    }
    VkPhysicalDeviceProperties properties;
    VkPhysicalDeviceFeatures features;

    vkGetPhysicalDeviceFeatures(device, &features);
    vkGetPhysicalDeviceProperties(device, &properties);

    switch (properties.deviceType)
    {
        case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU: score += 1000; break;
        case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU: score += 100; break;
        default: score += 0; break;
    }
    if (!features.geometryShader) { score = 0; return score; }
    return score;
}
static uint32_t mageRankScores(uint32_t *scores, uint32_t count)
{
    uint32_t indexLead = 0;
    uint32_t i;
    
    for (i = 0; i < count; i++)
    {   
        if (scores[indexLead] < scores[i])
        {
            indexLead = i;
        }
    }
    return indexLead;
}




static VkResult mageCreateInstance(struct mageRenderer *renderer, struct mageWindow *window, struct mageRendererProps *props)
{
    if (!mageCheckValidationLayers(renderer, window))
    {
        return VK_ERROR_INCOMPATIBLE_DRIVER;
    }
    VkInstanceCreateInfo instanceCreateInfo;
    VkApplicationInfo applicationInfo;
    uint32_t count;
    memset(&instanceCreateInfo, 0, sizeof(VkInstanceCreateInfo));
    memset(&applicationInfo, 0, sizeof(VkApplicationInfo));
    
    applicationInfo.sType                           = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    applicationInfo.apiVersion                      = VK_API_VERSION_1_2;
    applicationInfo.pApplicationName                = window->Title;
    applicationInfo.pEngineName                     = "MAGE-ENGINE";
    applicationInfo.engineVersion                   = VK_MAKE_VERSION(1, 0, 0);
    
    const char **extensions = (const char **)mageGetRequiredExtensions(&count);

    instanceCreateInfo.sType                        = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instanceCreateInfo.pApplicationInfo             = &applicationInfo;
    instanceCreateInfo.ppEnabledExtensionNames      = extensions;
    instanceCreateInfo.enabledExtensionCount        = count;

#if defined (MAGE_VULKAN)
    instanceCreateInfo.ppEnabledLayerNames          = mageRequiredLayers;
    instanceCreateInfo.enabledLayerCount            = 1;
    magePopulateValidationLayerCallback(&renderer->DebugMessengerCreateInfo);
    instanceCreateInfo.pNext                        = (VkDebugUtilsMessengerCreateInfoEXT *) &renderer->DebugMessengerCreateInfo;
#else
    instanceCreateInfo.ppEnabledLayerNames          = NULL;
    instanceCreateInfo.enabledLayerCount            = 0;
#endif

    VkResult result = MAGE_CHECK_VULKAN(vkCreateInstance(&instanceCreateInfo, NULL, &renderer->Instance));
    free(extensions);
    return result;
}
static VkResult mageCreateSurface(struct mageRenderer *renderer, struct mageWindow *window, struct mageRendererProps *props)
{
    VkResult result = MAGE_CHECK_VULKAN(glfwCreateWindowSurface(renderer->Instance, window->Context, NULL, &renderer->Surface));
    return result;
}
static VkResult magePickPhysicalDevice(struct mageRenderer *renderer, struct mageWindow *window, struct mageRendererProps *props)
{
    uint32_t deviceCount, i;
    vkEnumeratePhysicalDevices(renderer->Instance, &deviceCount, NULL);
    if (deviceCount <= 0)
    {
        MAGE_LOG_CORE_FATAL_ERROR("Unable to find any vulkan physical devices\n", NULL);
        return VK_ERROR_INCOMPATIBLE_DRIVER;
    }
    VkPhysicalDevice *devices = calloc(deviceCount, sizeof(VkPhysicalDevice));
    uint32_t *scores = calloc(deviceCount, sizeof(uint32_t));    
    vkEnumeratePhysicalDevices(renderer->Instance, &deviceCount, devices);

    for (i = 0; i < deviceCount; i++)
    {
        scores[i] = mageRateDevice(renderer, devices[i]);
    }
    uint32_t index = mageRankScores(scores, deviceCount);
    renderer->PhysicalDevice = devices[index];

    VkPhysicalDeviceProperties properties;
    vkGetPhysicalDeviceProperties(renderer->PhysicalDevice, &properties);
    MAGE_LOG_CORE_INFORM("Device picked %s\n", properties.deviceName);
    mageGetDeviceIndexes(renderer, renderer->PhysicalDevice, &renderer->Indexes);
    free(devices);
    free(scores);
    return VK_SUCCESS;
}
static VkResult mageCreateDevice(struct mageRenderer *renderer, struct mageWindow *window, struct mageRendererProps *props)
{
    const float priorities[] = { 1.0f };
    VkDeviceQueueCreateInfo queueCreateInfo;
    VkDeviceCreateInfo deviceCreateInfo;

    memset(&deviceCreateInfo, 0, sizeof(VkDeviceCreateInfo));
    memset(&queueCreateInfo, 0, sizeof(VkDeviceQueueCreateInfo));
    queueCreateInfo.sType                       = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueCreateInfo.queueFamilyIndex            = renderer->Indexes.GraphicIndexes[0];
    queueCreateInfo.pQueuePriorities            = priorities;
    queueCreateInfo.queueCount                  = 1;

    deviceCreateInfo.sType                      = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceCreateInfo.ppEnabledExtensionNames    = mageRequiredExtensions;
    deviceCreateInfo.enabledExtensionCount      = 1;    
    deviceCreateInfo.pQueueCreateInfos          = &queueCreateInfo;
    deviceCreateInfo.queueCreateInfoCount       = 1;

    VkResult result = MAGE_CHECK_VULKAN(vkCreateDevice(renderer->PhysicalDevice, &deviceCreateInfo, NULL, &renderer->Device));
    return result;
}
static VkResult mageFetchQueues(struct mageRenderer *renderer, struct mageWindow *window, struct mageRendererProps *props)
{
    vkGetDeviceQueue(renderer->Device, renderer->Indexes.GraphicIndexes[renderer->Indexes.GraphicIndexesCount - 1], 0, &renderer->GraphicalQueue);
    vkGetDeviceQueue(renderer->Device, renderer->Indexes.PresentIndexes[renderer->Indexes.PresentIndexesCount - 1], 0, &renderer->PresentQueue);
    return VK_SUCCESS;
}
static VkResult mageCreateSwapChain(struct mageRenderer *renderer, struct mageWindow *window, struct mageRendererProps *props)
{
    if (mageGetSwapChainSupport(&renderer->SwapChainSupportInfo, window, renderer->PhysicalDevice, renderer->Surface) != MAGE_SUCCESS)
    {
        MAGE_LOG_CORE_FATAL_ERROR("Failed to create swap chain, hardware invalid\n", NULL);
    }
    
    VkFormat format = mageSwapChainSupportPickSurfaceFormat(&renderer->SwapChainSupportInfo).format;;
    uint32_t imageCount = renderer->SwapChainSupportInfo.Capabilities.minImageCount + 1;
    uint32_t indicies[] = { renderer->Indexes.GraphicIndexes[0], renderer->Indexes.PresentIndexes[0] };

    VkSwapchainCreateInfoKHR createInfo;
    memset(&createInfo, 0, sizeof(VkSwapchainCreateInfoKHR));
    
    createInfo.sType                        = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface                      = renderer->Surface;
    createInfo.imageFormat                  = format;
    createInfo.imageColorSpace              = mageSwapChainSupportPickSurfaceFormat(&renderer->SwapChainSupportInfo).colorSpace;
    createInfo.imageExtent                  = renderer->SwapChainSupportInfo.Extent;
    createInfo.imageArrayLayers             = 1;
    createInfo.imageUsage                   = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    
    createInfo.minImageCount                = imageCount;
    createInfo.preTransform                 = renderer->SwapChainSupportInfo.Capabilities.currentTransform;
    createInfo.compositeAlpha               = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.clipped                      = VK_TRUE;
    createInfo.oldSwapchain                 = VK_NULL_HANDLE;

    if (renderer->Indexes.GraphicIndexes[0] != renderer->Indexes.PresentIndexes[0])
    {
        createInfo.imageSharingMode         = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount    = 2;
        createInfo.pQueueFamilyIndices      = indicies;
    }
    else
    {
        createInfo.imageSharingMode         = VK_SHARING_MODE_EXCLUSIVE;
    }
    
    VkResult result = MAGE_CHECK_VULKAN(vkCreateSwapchainKHR(renderer->Device, &createInfo, NULL, &renderer->SwapChain)); 
    return result;
}
static VkResult mageCreateSwapChainImages(struct mageRenderer *renderer, struct mageWindow *window, struct mageRendererProps *props)
{
    uint32_t imageCount, i;
    vkGetSwapchainImagesKHR(renderer->Device, renderer->SwapChain, &imageCount, NULL);
    renderer->SwapChainImages = calloc(imageCount, sizeof(VkImage));
    renderer->SwapChainImageViews = calloc(imageCount, sizeof(VkImageView));
    vkGetSwapchainImagesKHR(renderer->Device, renderer->SwapChain, &imageCount, renderer->SwapChainImages);
    renderer->SwapChainImageCount = imageCount;

    for (i = 0; i < imageCount; i++)
    {
        VkImageViewCreateInfo createInfo;
        memset(&createInfo, 0, sizeof(VkImageViewCreateInfo));

        createInfo.sType                            = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        createInfo.image                            = renderer->SwapChainImages[i];
        createInfo.viewType                         = VK_IMAGE_VIEW_TYPE_2D;
        createInfo.format                           = mageSwapChainSupportPickSurfaceFormat(&renderer->SwapChainSupportInfo).format;
        createInfo.components.r                     = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.g                     = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.b                     = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.a                     = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.subresourceRange.aspectMask      = VK_IMAGE_ASPECT_COLOR_BIT;
        createInfo.subresourceRange.baseMipLevel    = 0;
        createInfo.subresourceRange.levelCount      = 1;
        createInfo.subresourceRange.baseArrayLayer  = 0;
        createInfo.subresourceRange.layerCount      = 1;

        VkResult result = MAGE_CHECK_VULKAN(vkCreateImageView(renderer->Device, &createInfo, NULL, &renderer->SwapChainImageViews[i]));
        if (result != VK_SUCCESS) return result;
    }

    return VK_SUCCESS;
}
static VkResult mageCreateRenderPass(struct mageRenderer *renderer, struct mageWindow *window, struct mageRendererProps *props)
{
    VkAttachmentDescription attachmentDescription;
    memset(&attachmentDescription, 0, sizeof(VkAttachmentDescription));
    attachmentDescription.format                = mageSwapChainSupportPickSurfaceFormat(&renderer->SwapChainSupportInfo).format;
    attachmentDescription.samples               = VK_SAMPLE_COUNT_1_BIT;
    attachmentDescription.loadOp                = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachmentDescription.storeOp               = VK_ATTACHMENT_STORE_OP_STORE;
    attachmentDescription.stencilLoadOp         = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachmentDescription.stencilStoreOp        = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachmentDescription.initialLayout         = VK_IMAGE_LAYOUT_UNDEFINED;
    attachmentDescription.finalLayout           = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference attachmentReference;
    memset(&attachmentReference, 0, sizeof(VkAttachmentReference));
    attachmentReference.attachment              = 0;
    attachmentReference.layout                  = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpassDescription;
    memset(&subpassDescription, 0, sizeof(VkSubpassDescription));
    subpassDescription.pipelineBindPoint            = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpassDescription.colorAttachmentCount         = 1;
    subpassDescription.pColorAttachments            = &attachmentReference;


    VkRenderPassCreateInfo createInfo;
    memset(&createInfo, 0, sizeof(VkRenderPassCreateInfo));
    createInfo.sType                = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    createInfo.attachmentCount      = 1;
    createInfo.pAttachments         = &attachmentDescription;
    createInfo.subpassCount         = 1;
    createInfo.pSubpasses           = &subpassDescription;
    

    vkCreateRenderPass(renderer->Device, &createInfo, NULL, &renderer->PrimaryRenderPass);
    return VK_SUCCESS;
}
static VkResult mageCreateGraphicsPipeline(struct mageRenderer *renderer, struct mageWindow *window, struct mageRendererProps *props)
{
    VkPipelineShaderStageCreateInfo *pipelineShaderStages = calloc(props->ShaderCount, sizeof(VkPipelineShaderStageCreateInfo));
    VkShaderModule *pipelineShaderModules                 = calloc(props->ShaderCount, sizeof(VkShaderModule));
    {
        uint32_t i;
        for (i = 0; i < props->ShaderCount; i++)
        {
            VkShaderModule module = mageShaderCreateModule(&props->RuntimeShaders[i], renderer->Device);

            VkPipelineShaderStageCreateInfo stageCreateInfo;
            memset(&stageCreateInfo, 0, sizeof(VkPipelineShaderStageCreateInfo));
            stageCreateInfo.sType     = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
            stageCreateInfo.stage     = mageShaderTypeToBit(props->RuntimeShaders[i].ShaderType);
            stageCreateInfo.module    = module;
            stageCreateInfo.pName     = props->RuntimeShaders[i].RuntimeFunctionName;
            pipelineShaderStages[i]   = stageCreateInfo;
            pipelineShaderModules[i]  = module;
            MAGE_LOG_CORE_INFORM("Creating shader %s with entry point of %s, shader %d of %d\n", props->RuntimeShaders[i].FilePath, props->RuntimeShaders[i].RuntimeFunctionName, i + 1, props->ShaderCount);
        }
    }
    
    VkPipelineVertexInputStateCreateInfo vertexInputInfo;
    memset(&vertexInputInfo, 0, sizeof(VkPipelineVertexInputStateCreateInfo));
    
    vertexInputInfo.sType                               = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount       = 0;
    vertexInputInfo.vertexAttributeDescriptionCount     = 0;

    VkPipelineInputAssemblyStateCreateInfo inputAssembly;
    memset(&inputAssembly, 0, sizeof(VkPipelineInputAssemblyStateCreateInfo));
    
    inputAssembly.sType                     = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology                  = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable    = VK_FALSE;

    VkViewport viewport;
    memset(&viewport, 0, sizeof(VkViewport));

    viewport.x              = 0.0f;
    viewport.y              = 0.0f;
    viewport.width          = (float) renderer->SwapChainSupportInfo.Extent.width;
    viewport.height         = (float) renderer->SwapChainSupportInfo.Extent.height;
    viewport.minDepth       = 0.0f;
    viewport.maxDepth       = 1.0f;

    VkRect2D scissor;
    memset(&scissor, 0, sizeof(VkRect2D));

    scissor.offset = (VkOffset2D){ 0.0f, 0.0f };
    scissor.extent = renderer->SwapChainSupportInfo.Extent;

    VkPipelineViewportStateCreateInfo viewportState;
    memset(&viewportState, 0, sizeof(VkPipelineViewportStateCreateInfo));

    viewportState.sType             = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount     = 1;
    viewportState.pViewports        = &viewport;
    viewportState.scissorCount      = 1;
    viewportState.pScissors         = &scissor;

    VkPipelineRasterizationStateCreateInfo rasterizer;
    memset(&rasterizer, 0, sizeof(VkPipelineRasterizationStateCreateInfo));

    rasterizer.sType                        = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable             = VK_FALSE;
    rasterizer.rasterizerDiscardEnable      = VK_FALSE;
    rasterizer.polygonMode                  = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth                    = 1.0f;
    rasterizer.cullMode                     = VK_CULL_MODE_BACK_BIT;
    rasterizer.frontFace                    = VK_FRONT_FACE_CLOCKWISE;
    rasterizer.depthBiasEnable              = VK_FALSE;

    VkPipelineMultisampleStateCreateInfo multisampling;
    memset(&multisampling, 0, sizeof(VkPipelineMultisampleStateCreateInfo));

    multisampling.sType                 = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable   = VK_FALSE;
    multisampling.rasterizationSamples  = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineColorBlendAttachmentState colorBlendAttachment;
    memset(&colorBlendAttachment, 0, sizeof(VkPipelineColorBlendAttachmentState));

    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable    = VK_FALSE;

    VkPipelineColorBlendStateCreateInfo colorBlending;
    memset(&colorBlending, 0, sizeof(VkPipelineColorBlendStateCreateInfo));

    colorBlending.sType             = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable     = VK_FALSE;
    colorBlending.logicOp           = VK_LOGIC_OP_COPY;
    colorBlending.attachmentCount   = 1;
    colorBlending.pAttachments      = &colorBlendAttachment;
    colorBlending.blendConstants[0] = 0.0f;
    colorBlending.blendConstants[1] = 0.0f;
    colorBlending.blendConstants[2] = 0.0f;
    colorBlending.blendConstants[3] = 0.0f;

    VkPipelineLayoutCreateInfo pipelineLayoutInfo;
    memset(&pipelineLayoutInfo, 0, sizeof(VkPipelineLayoutCreateInfo));

    pipelineLayoutInfo.sType                    = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount           = 0;
    pipelineLayoutInfo.pushConstantRangeCount   = 0;

    VkResult result = MAGE_CHECK_VULKAN(vkCreatePipelineLayout(renderer->Device, &pipelineLayoutInfo, NULL, &renderer->GraphicsPipelineLayout));
    
    uint32_t i;
    for (i = 0; i < props->ShaderCount; i++)
    {
        vkDestroyShaderModule(renderer->Device, pipelineShaderModules[i], NULL);
    }
    free(pipelineShaderModules);
    free(pipelineShaderStages);
        

    return result;  
}
mageResult mageRendererInitialise(struct mageRenderer *renderer, struct mageWindow *window, struct mageRendererProps *props)
{
    uint32_t i;
    typedef VkResult (*function)(struct mageRenderer *, struct mageWindow *, struct mageRendererProps *);
    function functions[] = 
    { 
        mageCreateInstance, 
    #if defined (MAGE_DEBUG)
        mageSetupValidationLayerCallback,
    #endif 
        mageCreateSurface, 
        magePickPhysicalDevice, 
        mageCreateDevice,
        mageFetchQueues,
        mageCreateSwapChain,
        mageCreateSwapChainImages,
        mageCreateRenderPass,
        mageCreateGraphicsPipeline,
    };

    for (i = 0; i < sizeof(functions) / sizeof(function); i++)
    {
        VkResult result = functions[i](renderer, window, props);
        if (result != VK_SUCCESS) return MAGE_UNKNOWN;
    }
    MAGE_LOG_CORE_INFORM("Renderer passed in %d of %d operations\n", i, sizeof(functions) / sizeof(function));
    return MAGE_SUCCESS;
}
void mageRendererDestroy(struct mageRenderer *renderer)
{
    uint32_t i;
    /* vkDestroyPipeline(renderer->Device, renderer->GraphicsPipeline, NULL); */
    vkDestroyPipelineLayout(renderer->Device, renderer->GraphicsPipelineLayout, NULL);
    vkDestroyRenderPass(renderer->Device, renderer->PrimaryRenderPass, NULL);

    for (i = 0; i < renderer->SwapChainImageCount; i++)
    {
        vkDestroyImageView(renderer->Device, renderer->SwapChainImageViews[i], NULL);
    }

    vkDestroySwapchainKHR(renderer->Device, renderer->SwapChain, NULL);
    vkDestroyDevice(renderer->Device, NULL);

#if defined (MAGE_DEBUG)
    mageDestroyDebugUtilsMessengerEXT(renderer->Instance, renderer->DebugMessenger, NULL);
#endif

    vkDestroySurfaceKHR(renderer->Instance, renderer->Surface, NULL);
    vkDestroyInstance(renderer->Instance, NULL);
    mageIndiciesIndexesDestroy(&renderer->Indexes);
    mageSwapChainSupportDestroy(&renderer->SwapChainSupportInfo);
    
    free(renderer->SwapChainImages);
    free(renderer->SwapChainImageViews);
}


#endif