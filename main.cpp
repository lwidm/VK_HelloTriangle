#include "vulkan/vulkan.hpp"
#include <algorithm>
#include <cstdint>
#include <limits>
#include <map>
#include <utility>
#include <vector>
#include <vulkan/vulkan_core.h>
#include <vulkan/vulkan_raii.hpp>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <cstdlib>
#include <iostream>
#include <stdexcept>

constexpr uint32_t WIDTH  = 800;
constexpr uint32_t HEIGHT = 600;

const std::vector<const char *> validationLayers = {
    "VK_LAYER_KHRONOS_validation"};

#ifdef NDEBUG
constexpr bool enableValidationLayers = false;
#else
constexpr bool enableValidationLayers = true;
#endif

class HelloTriangleApplication
{
  public:
    void run()
    {
        this->initWindow();
        this->initVulkan();
        this->mainLoop();
        this->cleanup();
    }

  private:
    GLFWwindow                      *window = nullptr;
    vk::raii::Context                context;
    vk::raii::Instance               instance       = nullptr;
    vk::raii::DebugUtilsMessengerEXT debugMessenger = nullptr;
    vk::raii::SurfaceKHR             surface        = nullptr;
    vk::raii::PhysicalDevice         physicalDevice = nullptr;
    vk::raii::Device                 device         = nullptr;
    vk::raii::Queue                  graphicsQueue  = nullptr;
    vk::raii::Queue                  presentQueue   = nullptr;
    vk::raii::SwapchainKHR           swapChain      = nullptr;
    std::vector<vk::Image>           swapChainImages;
    vk::SurfaceFormatKHR             swapChainSurfaceFormat;
    vk::Extent2D                     swapChainExtent;

    vk::Format swapChainImageFormat = vk::Format::eUndefined;

    uint32_t queueFamilyIndices[2];

    std::vector<const char *> deviceExtensions = {vk::KHRSwapchainExtensionName};

    void initWindow()
    {
        glfwInit();

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

        this->window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);
    }

    void initVulkan()
    {
        this->createInstance();
        this->setupDebugMessenger();
        this->createSurface();
        this->pickPhysicalDevice();
        this->createLogicalDevice();
        this->createSwapChain(); // TODO : Might be out of date
    }

    void mainLoop()
    {
        while (!glfwWindowShouldClose(this->window))
        {
            glfwPollEvents();
        }
    }

    void cleanup()
    {
        glfwDestroyWindow(window);

        glfwTerminate();
    }

    void createInstance()
    {
        constexpr vk::ApplicationInfo appInfo{
            .pApplicationName   = "Hello Triangle",
            .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
            .pEngineName        = "No Engine",
            .engineVersion      = VK_MAKE_VERSION(1, 0, 0),
            .apiVersion         = vk::ApiVersion14};

        // Get the required layers
        std::vector<const char *> requiredLayers;
        if (enableValidationLayers)
        {
            requiredLayers.assign(validationLayers.begin(), validationLayers.end());
        }

        // Check if the required layers are supported by the Vulkan implementation.
        auto layerProperties = this->context.enumerateInstanceLayerProperties();
        if (std::ranges::any_of(requiredLayers, [&layerProperties](const auto &requiredLayer) {
                return std::ranges::none_of(layerProperties, [requiredLayer](const auto &layerProperty) {
                    return strcmp(layerProperty.layerName, requiredLayer) == 0;
                });
            }))
        {
            throw std::runtime_error("One or more required layers are not supported!");
        }

        auto requiredExtensions = this->getRequiredExtensions();

        // Check if the required extensions are supported by the Vulkan implementation.
        auto extensionProperties = this->context.enumerateInstanceExtensionProperties();
        for (uint32_t i = 0; i < requiredExtensions.size(); ++i)
        {
            if (std::ranges::none_of(extensionProperties,
                                     [requiredExtension = requiredExtensions[i]](auto const &extensionProperty) { return strcmp(extensionProperty.extensionName, requiredExtension) == 0; }))
            {
                throw std::runtime_error("Required GLFW extension not supported: " + std::string(requiredExtensions[i]));
            }
        }

        vk::InstanceCreateInfo createInfo{
            .pApplicationInfo        = &appInfo,
            .enabledLayerCount       = static_cast<uint32_t>(requiredLayers.size()),
            .ppEnabledLayerNames     = requiredLayers.data(),
            .enabledExtensionCount   = static_cast<uint32_t>(requiredExtensions.size()),
            .ppEnabledExtensionNames = requiredExtensions.data()};

        instance = vk::raii::Instance(context, createInfo);
    }

    void setupDebugMessenger()
    {
        if (!enableValidationLayers)
            return;
        vk::DebugUtilsMessageSeverityFlagsEXT severityFlags(vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose | vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning | vk::DebugUtilsMessageSeverityFlagBitsEXT::eError);
        vk::DebugUtilsMessageTypeFlagsEXT     messageTypeFlags(vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance | vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation);
        vk::DebugUtilsMessengerCreateInfoEXT  debugUtilsMessengerCreateInfoEXT{
             .messageSeverity = severityFlags,
             .messageType     = messageTypeFlags,
             .pfnUserCallback = &debugCallback};
        this->debugMessenger = this->instance.createDebugUtilsMessengerEXT(debugUtilsMessengerCreateInfoEXT);
    }

    void createSurface()
    {
        VkSurfaceKHR _surface;
        if (glfwCreateWindowSurface(*this->instance, this->window, nullptr, &_surface) != 0)
        {
            throw std::runtime_error("failed to create window surface!");
        }
        this->surface = vk::raii::SurfaceKHR(this->instance, _surface);
    }

    void pickPhysicalDevice()
    {
        std::vector<vk::raii::PhysicalDevice> devices = this->instance.enumeratePhysicalDevices();
        const auto                            devIter = std::ranges::find_if(devices, [&](const vk::raii::PhysicalDevice &device) {
            auto       queueFamilies = device.getQueueFamilyProperties();
            bool       isSuitable    = device.getProperties().apiVersion >= VK_API_VERSION_1_3;
            const auto qfpIter       = std::ranges::find_if(queueFamilies,
                                                                                       [](const vk::QueueFamilyProperties &qfp) {
                                                          return (qfp.queueFlags & vk::QueueFlagBits::eGraphics) != static_cast<vk::QueueFlags>(0);
                                                      });
            isSuitable               = isSuitable && (qfpIter != queueFamilies.end());
            auto extensions          = device.enumerateDeviceExtensionProperties();
            bool found               = true;
            for (const auto &extension : this->deviceExtensions)
            {
                auto extensionIter = std::ranges::find_if(extensions, [extension](auto const &ext) { return strcmp(ext.extensionName, extension) == 0; });
                found              = found && extensionIter != extensions.end();
            }
            isSuitable = isSuitable && found;
            if (isSuitable)
            {
                this->physicalDevice = device;
            }
            return isSuitable;
        });
        if (devIter == devices.end())
        {
            throw std::runtime_error("failed to find a suitable GPU!");
        }
    }

    void createLogicalDevice()
    {
        // find the index of the first queue family that supports graphics
        std::vector<vk::QueueFamilyProperties> queueFamilyProperties = this->physicalDevice.getQueueFamilyProperties();

        // get the first index into queueFamilyProperties which supports graphics
        auto graphicsQueueFamilyProperty = std::ranges::find_if(queueFamilyProperties, [](const vk::QueueFamilyProperties &qfp) { return (qfp.queueFlags & vk::QueueFlagBits::eGraphics) != static_cast<vk::QueueFlags>(0); });
        assert(graphicsQueueFamilyProperty != queueFamilyProperties.end() && "No graphics queue family found!");
        auto graphicsIndex = static_cast<uint32_t>(std::distance(queueFamilyProperties.begin(), graphicsQueueFamilyProperty));

        // determine a queueFamilyIndex that supports present
        // first check if the graphicsIndex is good enough
        auto presentIndex = this->physicalDevice.getSurfaceSupportKHR(graphicsIndex, *this->surface) ? graphicsIndex : static_cast<uint32_t>(queueFamilyProperties.size());
        if (presentIndex == queueFamilyProperties.size())
        {
            // the graphicsIndex doesn't support present -> look for another family index that supports both graphics and present
            for (size_t i = 0; i < queueFamilyProperties.size(); i++)
            {
                if ((queueFamilyProperties[i].queueFlags & vk::QueueFlagBits::eGraphics) && physicalDevice.getSurfaceSupportKHR(static_cast<uint32_t>(i), *this->surface))
                {
                    graphicsIndex = static_cast<uint32_t>(i);
                    presentIndex  = graphicsIndex;
                    break;
                }
            }
            if (presentIndex == queueFamilyProperties.size())
            {
                // there's nothing like a single family index that supports both graphics and present -> look for another family index that supports present
                for (size_t i = 0; i < queueFamilyProperties.size(); i++)
                {
                    if (physicalDevice.getSurfaceSupportKHR(static_cast<uint32_t>(i), *this->surface))
                    {
                        presentIndex = static_cast<uint32_t>(i);
                        break;
                    }
                }
            }
        }
        if ((graphicsIndex == queueFamilyProperties.size()) || (presentIndex == queueFamilyProperties.size()))
        {
            throw std::runtime_error("Could not find a queue for graphics or present -> terminating");
        }
        this->queueFamilyIndices[0] = graphicsIndex;
        this->queueFamilyIndices[1] = presentIndex;

        // query for Vulkan 1.3 features
        vk::StructureChain<vk::PhysicalDeviceFeatures2, vk::PhysicalDeviceVulkan13Features, vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT> featureChain = {
            {},
            {.dynamicRendering = true},           // Enable dynamic rendering from Vulkan 1.3
            {.extendedDynamicState = true}        // Enabled extended dynamic state from the extension
        };

        // create a Device
        float                     queuePriority = 0.5f;
        vk::DeviceQueueCreateInfo deviceQueueCreateInfo{.queueFamilyIndex = graphicsIndex, .queueCount = 1, .pQueuePriorities = &queuePriority};
        vk::DeviceCreateInfo      deviceCreateInfo{
                 .pNext                   = &featureChain.get<vk::PhysicalDeviceFeatures2>(),
                 .queueCreateInfoCount    = 1,
                 .pQueueCreateInfos       = &deviceQueueCreateInfo,
                 .enabledExtensionCount   = static_cast<uint32_t>(this->deviceExtensions.size()),
                 .ppEnabledExtensionNames = this->deviceExtensions.data()};

        this->device        = vk::raii::Device(this->physicalDevice, deviceCreateInfo);
        this->graphicsQueue = vk::raii::Queue(this->device, graphicsIndex, 0);
    }

    void createSwapChain()
    {
        auto surfaceCapabilities     = physicalDevice.getSurfaceCapabilitiesKHR(*this->surface);
        this->swapChainSurfaceFormat = this->chooseSwapSurfaceFormat(this->physicalDevice.getSurfaceFormatsKHR(*this->surface));
        this->swapChainExtent        = this->chooseSwapExtent(surfaceCapabilities);
        auto minImageCount           = std::max(3u, surfaceCapabilities.minImageCount);
        minImageCount                = (surfaceCapabilities.maxImageCount > 0 && minImageCount > surfaceCapabilities.maxImageCount) ? surfaceCapabilities.maxImageCount : minImageCount;
        vk::SwapchainCreateInfoKHR swapChainCreateInfo{
            .flags            = vk::SwapchainCreateFlagsKHR(),
            .surface          = *this->surface,
            .minImageCount    = minImageCount,
            .imageFormat      = this->swapChainSurfaceFormat.format,
            .imageColorSpace  = this->swapChainSurfaceFormat.colorSpace,
            .imageExtent      = this->swapChainExtent,
            .imageArrayLayers = 1,
            .imageUsage       = vk::ImageUsageFlagBits::eColorAttachment,
            .preTransform     = surfaceCapabilities.currentTransform,
            .compositeAlpha   = vk::CompositeAlphaFlagBitsKHR::eOpaque,
            .presentMode      = this->chooseSwapPresentMode(this->physicalDevice.getSurfacePresentModesKHR(*this->surface)),
            .clipped          = true,
            .oldSwapchain     = nullptr};
        if (this->queueFamilyIndices[0] != this->queueFamilyIndices[1])
        {
            swapChainCreateInfo.imageSharingMode      = vk::SharingMode::eConcurrent;
            swapChainCreateInfo.queueFamilyIndexCount = 2;
            swapChainCreateInfo.pQueueFamilyIndices   = this->queueFamilyIndices;
        }
        else
        {
            swapChainCreateInfo.imageSharingMode      = vk::SharingMode::eExclusive;
            swapChainCreateInfo.queueFamilyIndexCount = 0;              // optional
            swapChainCreateInfo.pQueueFamilyIndices   = nullptr;        // optional
        }
        this->swapChain       = vk::raii::SwapchainKHR(this->device, swapChainCreateInfo);
        this->swapChainImages = swapChain.getImages();
        this->swapChainImageFormat = this->swapChainSurfaceFormat.format;
    }

    static vk::SurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<vk::SurfaceFormatKHR> &availableFormats)
    {
        assert(!availableFormats.empty());
        for (const auto &availableFormat : availableFormats)
        {
            if (availableFormat.format == vk::Format::eB8G8R8A8Srgb && availableFormat.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear)
            {
                return availableFormat;
            }
        }
        return availableFormats[0];
    }

    static vk::PresentModeKHR chooseSwapPresentMode(const std::vector<vk::PresentModeKHR> &availablePresentModes)
    {
        for (const auto &availablePresentMode : availablePresentModes)
        {
            if (availablePresentMode == vk::PresentModeKHR::eMailbox)
            {
                return availablePresentMode;
            }
        }
        return vk::PresentModeKHR::eFifo;
    }

    static std::vector<const char *> getRequiredExtensions()
    {
        uint32_t glfwExtensionCount = 0;
        auto     glfwExtensions     = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

        std::vector<const char *> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);
        if (enableValidationLayers)
            extensions.push_back(vk::EXTDebugUtilsExtensionName);

        return extensions;
    }

    vk::Extent2D chooseSwapExtent(const vk::SurfaceCapabilitiesKHR &capabilities)
    {
        if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
        {
            return capabilities.currentExtent;
        }
        int width, height;
        glfwGetFramebufferSize(this->window, &width, &height);

        return {
            std::clamp<uint32_t>(width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width),
            std::clamp<uint32_t>(height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height)};
    }

    static VKAPI_ATTR vk::Bool32 VKAPI_CALL debugCallback(vk::DebugUtilsMessageSeverityFlagBitsEXT severity, vk::DebugUtilsMessageTypeFlagsEXT type, const vk::DebugUtilsMessengerCallbackDataEXT *pCallbackData, void *)
    {
        if (severity == vk::DebugUtilsMessageSeverityFlagBitsEXT::eError || severity == vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning)
        {
            std::cerr << "validation layer: type " << to_string(type) << " msg: " << pCallbackData->pMessage << std::endl;
        }
        return vk::False;
    }
};

int main()
{
    HelloTriangleApplication app;

    try
    {
        app.run();
    }
    catch (const std::exception &e)
    {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
