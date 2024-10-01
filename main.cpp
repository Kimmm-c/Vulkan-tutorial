#define GLFW_INCLUDE_VULKAN

#include <glfw3.h>

#include <iostream>
#include <stdexcept>
#include <cstdlib>
#include <vector>
#include <cstring>
#include <map>
#include <optional>
#include <set>
#include <limits>
#include <algorithm>

using namespace std;

struct QueueFamilyIndices {
    // optional data type indicates that the value of the variable could be absent or undefined.
    optional<uint32_t> graphicsFamily;
    optional<uint32_t> presentFamily;

    bool isComplete() {
        return graphicsFamily.has_value()
               && presentFamily.has_value();
    }
};

struct SwapChainSupportDetails {
    VkSurfaceCapabilitiesKHR capabilities;  // Eg: min/max number of images in swap chain, min/max width and height of images, etc..
    vector<VkSurfaceFormatKHR> formats;     // Eg: pixel format, color space
    vector<VkPresentModeKHR> presentModes;
};

/**
 * @brief Set up a debug messenger in a Vulkan application.
 * It handles the creation of a debug callback that allows you to receive diagnostic messages from the Vulkan API, such as validation errors, warnings, or performance issues.
 *
 * @param instance A Vulkan instance, representing the connection between the application and the Vulkan library.
 * @param pCreateInfo A struct containing instructions on how the debug messenger should behave.
 * @param pAllocator An optional param for custom memory allocation. Usually a nullptr.
 * @param pDebugMessenger A pointer to where the created debug messenger handle is stored, after successfully created.
 * @return
 */
VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT *pCreateInfo,
                                      const VkAllocationCallbacks *pAllocator,
                                      VkDebugUtilsMessengerEXT *pDebugMessenger) {
    /*
     * Retrieve the address of the Vulkan function vkCreateDebugUtilsMessengerEXT.
     *
     * Note:
     * 1. We can't call vkCreateDebugUtilsMessengerEXT directly because the function is not part of Vulkan core API and,
     * therefore, is not automatically loaded when the Vulkan library is loaded. Instead, we need to use vkGetInstanceProcAddr
     * to retrieve the function at runtime.
     *
     * 2. vkCreateDebugUtilsMessengerEXT is part of extensions.
     *
     * 3. We need to pass instance along with the function name to vkGetInstanceProcAddr as a specific-context.
     * It reads as we are retrieving the extension of this instance and for this instance. Each instance may have
     * access to different extensions based on its platform.
     */
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    if (func != nullptr) {
        return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
    } else {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}

/**
 * @brief Destroy the debug messenger.
 *
 * @param instance A Vulkan instance, representing the connection between the application and the Vulkan library.
 * @param debugMessenger The created debug messenger.
 * @param pAllocator An optional param for custom memory allocation. Usually a nullptr.
 */
void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger,
                                   const VkAllocationCallbacks *pAllocator) {
    // Similar to vkCreateDebugUtilsMessengerEXT, vkDestroyDebugUtilsMessengerEXT has to be loaded using vkGetInstanceProcAddr.
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance,
                                                                            "vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr) {
        func(instance, debugMessenger, pAllocator);
    }
}

class TriangleApp {
    GLFWwindow *window;
    VkInstance instance;
    VkDebugUtilsMessengerEXT debugMessenger;
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkDevice logicalDevice; // Logical device

    // Queue families
    VkQueue graphicsQueue;
    VkQueue presentQueue;

    VkSurfaceKHR surface;

    // Swap chain and its settings
    VkSwapchainKHR swapChain;
    vector<VkImage> swapChainImages;
    vector<VkImageView> swapChainImageViews;
    VkFormat swapChainImageFormat;
    VkExtent2D swapChainExtent;

    // Conditionally turn on the validation layers if the program is compiled in debug mode. Otherwise, turn off the validation layers to improve performance.
    const vector<const char *> validationLayers{"VK_LAYER_KHRONOS_validation"};
#ifdef NDEBUG
    const bool enableValidationLayers = false;
#else
    const bool enableValidationLayers = true;
#endif // NDEBUG

    const vector<const char *> deviceExtensions = {
            VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };
public:
    void run() {
        initWindow();
        initVulkan();
        mainLoop();
        cleanup();
    }

    /**
     * @brief Handle a debug message.
     *
     * @param messageSeverity specifies the severity of the message, either Verbose, Info, Warning, or Error.
     * @param messageType specifies the type of the message, either General, Validation, or Performance.
     * @param pCallbackData a VkDebugUtilsMessengerCallbackDataEXT struct containing the details of the message.
     * @param pUserData a pointer that is specified during the setup of the callback, allowing you to pass your own data to it.
     * @return a boolean indicating if the Vulkan call that triggered the validation layer message should be aborted.
     */
    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
            VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType,
            const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData, void *pUserData) {
        cerr << "Validation layer: " << pCallbackData->pMessage << endl;

        return VK_FALSE;
    }

private:
    void initWindow() {
        // Create glfw library
        glfwInit();

        // GLFW was originally designed to create an OpenGL context, we need to tell it to not create an OpenGL context
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        // Temporarily disable window resizing
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

        const uint32_t WIDTH = 800;
        const uint32_t HEIGHT = 600;
        window = glfwCreateWindow(WIDTH, HEIGHT, "Triangle", nullptr, nullptr);
    }

    void initVulkan() {
        // Create Vulkan lib
        createInstance();
        createSurface();
        setupDebugMessenger();

        // Pick a physical device (a graphic card) that supports the Vulkan library features.
        pickPhysicalDevice();

        // Create a logical device. This device is the middle layer/translator that enables the communication between your app and the physical device.
        createLogicalDevice();

        createSwapChain();
        createImageView();
        createGraphicsPipeline();
    }

    void createGraphicsPipeline() {

    }

    /**
     * @brief Create an image view, which describes how to access an image and which part to access.
     */
    void createImageView() {
        uint8_t imageCount = swapChainImages.size();
        swapChainImageViews.resize(imageCount);

        for (size_t i = 0; i < imageCount; i++) {
            VkImageViewCreateInfo createInfo{};
            createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            createInfo.image = swapChainImages[i];

            // Specify how image data should be interpreted.
            createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            createInfo.format = swapChainImageFormat;

            createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

            // Specify the image's purpose and which part of the image should be accessed.
            createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT; // Access the color portion of the image
            createInfo.subresourceRange.baseMipLevel = 0;                       // mip = 0 means the full resolution image
            createInfo.subresourceRange.levelCount = 1;                         // the number of mipmap levels accessible from the image
            createInfo.subresourceRange.baseArrayLayer = 0;                     // The starting layer, in this case, starting from the first layer.
            createInfo.subresourceRange.layerCount = 1;                         // the number of layers

            if (vkCreateImageView(logicalDevice, &createInfo, nullptr, &swapChainImageViews[i]) != VK_SUCCESS) {
                throw runtime_error("Failed to create image views");
            }
        }
    }

    void createSwapChain() {
        SwapChainSupportDetails swapChainSupport = querySwapChainSupport(physicalDevice);

        VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
        swapChainImageFormat = surfaceFormat.format;
        VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
        VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities);
        swapChainExtent = extent;

        /*
         * Specify the number of images in the swap chain.
         *
         * Why an extra image is necessary?
         * Because minImageCount = 2 images (double buffering), 1 is being displayed to the screen while the other is
         * being rendered to. This may lead to synchronization issue where the application has to wait for the currently
         * displayed image to finish presenting before rendering the next image, causing delays.
         *
         * Adding an extra image means triple buffering, allowing overlap in rendering process, where:
         * 1 image being displayed on screen.
         * 1 image being rendered by the GPU.
         * 1 image available for the application to start rendering as soon as it finished its current rendering work.
         */
        uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;

        // If maxImageCount exists, make sure the image count doesn't exceed the maximumImageCount.
        if (swapChainSupport.capabilities.maxImageCount > 0 &&
            imageCount > swapChainSupport.capabilities.maxImageCount) {
            imageCount = swapChainSupport.capabilities.maxImageCount;
        }

        // Create the swap chain
        VkSwapchainCreateInfoKHR createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        createInfo.surface = surface;
        createInfo.minImageCount = imageCount;
        createInfo.imageFormat = surfaceFormat.format;
        createInfo.imageColorSpace = surfaceFormat.colorSpace;
        createInfo.imageExtent = extent;
        createInfo.presentMode = presentMode;

        // Specify the number of layers each image consists of.
        createInfo.imageArrayLayers = 1;

        // Specify the kind of operations we're using the swap chain for.
        createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

        // Specify how swap chain images are handled across different queue families. In this case, graphics and presentation queue families.
        QueueFamilyIndices indices = findQueueFamilies(physicalDevice);
        uint32_t queueFamilyIndices[] = {indices.graphicsFamily.value(), indices.presentFamily.value()};

        // If graphics queue family is not the same as present queue family, set sharing mode to Concurrent to allow
        // using images across multiple queue families (no explicit ownership transfer). Otherwise, set image sharing
        // mode to Exclusive to ensure an image is owned by one queue family at a time (ownership must be explicitly
        // transferred before the image can be used by another queue family).
        if (indices.graphicsFamily != indices.presentFamily) {
            createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
            createInfo.queueFamilyIndexCount = 2;
            createInfo.pQueueFamilyIndices = queueFamilyIndices;
        } else {
            createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
            createInfo.queueFamilyIndexCount = 0;       // Optional
            createInfo.pQueueFamilyIndices = nullptr;   // Optional
        }

        // Specify transform (like rotation, horizontal flip, etc...)
        createInfo.preTransform = swapChainSupport.capabilities.currentTransform;

        // Specify if alpha channel should be used for blending with other windows in the window system.
        createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR; // Ignore the alpha channel.

        // Enable clipping to ignore the color of pixels that are obscured when being hidden by another window.
        createInfo.clipped = VK_TRUE;

        // The swap chain may become invalid or unoptimized while the app is running (eg: when the window is resized).
        // This field allows us to re-create another swap chain that reference to the old swap chain.
        // Note: leave it alone for now.
        createInfo.oldSwapchain = VK_NULL_HANDLE;

        if (vkCreateSwapchainKHR(logicalDevice, &createInfo, nullptr, &swapChain) != VK_SUCCESS) {
            throw runtime_error("Failed to create swap chain");
        }

        // Retrieve the images of the swap chain for later interaction
        vkGetSwapchainImagesKHR(logicalDevice, swapChain, &imageCount, nullptr);
        swapChainImages.resize(imageCount);
        vkGetSwapchainImagesKHR(logicalDevice, swapChain, &imageCount, swapChainImages.data());
    }

    void createSurface() {
        if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS) {
            throw runtime_error("Failed to create window surface");
        }
    }

    void createLogicalDevice() {
        // Specify queue info and create queues
        QueueFamilyIndices indices = findQueueFamilies(physicalDevice);

        vector<VkDeviceQueueCreateInfo> queueCreateInfos;
        set<uint32_t> uniqueQueueFamilies = {indices.graphicsFamily.value(), indices.presentFamily.value()};

        float queuePriority = 1.0f; // This priority will influence the scheduling of command buffer execution.
        for (uint32_t queueFamily: uniqueQueueFamilies) {
            VkDeviceQueueCreateInfo queueCreateInfo{};
            queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queueCreateInfo.queueFamilyIndex = queueFamily;
            queueCreateInfo.queueCount = 1;
            queueCreateInfo.pQueuePriorities = &queuePriority;
            queueCreateInfos.push_back(queueCreateInfo);
        }

        // Specify required device features
        VkPhysicalDeviceFeatures deviceFeatures{};


        // Creating the Logical device
        VkDeviceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        createInfo.pQueueCreateInfos = queueCreateInfos.data();
        createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
        createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
        createInfo.ppEnabledExtensionNames = deviceExtensions.data();
        createInfo.pEnabledFeatures = &deviceFeatures;

        if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &logicalDevice)) {
            throw runtime_error("Failed to create logical device");
        }

        // Retrieve queue for later interactions
        vkGetDeviceQueue(logicalDevice, indices.graphicsFamily.value(), 0, &graphicsQueue);
        vkGetDeviceQueue(logicalDevice, indices.presentFamily.value(), 0, &presentQueue);
    }

    void pickPhysicalDevice() {

        // Get all available graphics cards
        uint32_t deviceCount = 0;
        vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

        if (deviceCount == 0) {
            throw runtime_error("Failed to find GPUs with Vulkan support");
        }

        // Allocate an array to hold the devices and retrieve device details
        vector<VkPhysicalDevice> devices(deviceCount);
        vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

        /*
         * Rank the devices from lowest-suitable score to highest-suitable score.
         * Pick the one with the highest score.
         */
        // Use an ordered map to automatically sort candidates by increasing score
        multimap<int, VkPhysicalDevice> candidates;
        for (const auto &device: devices) {
            int score = rateDeviceSuitability(device);
            candidates.insert(make_pair(score, device));
        }

        // Check whether the best candidate is suitable
        if (candidates.rbegin()->first > 0) {
            physicalDevice = candidates.rbegin()->second;
        } else {
            throw runtime_error("Failed to find a suitable GPU");
        }

        // Raise an error if unable to find a suitable GPU for the instance.
        if (physicalDevice == VK_NULL_HANDLE || !isDeviceSuitable(physicalDevice)) {
            throw runtime_error("Failed to find a suitable GPU");
        }
    }

    /**
     * @brief This function sets up the surface format (color depth) of the swap chain.
     *
     * Note: Each VkSurfaceFormatKHR entry contains a format and a colorSpace member.
     * The format member specifies the color channels and types.
     * For example, VK_FORMAT_B8G8R8A8_SRGB means that we store the B, G, R and alpha channels in that order with an 8
     * bit unsigned integer for a total of 32 bits per pixel. The colorSpace member indicates if the sRGB color space is
     * supported or not using the VK_COLOR_SPACE_SRGB_NONLINEAR_KHR flag. Note that this flag used to be called
     * VK_COLORSPACE_SRGB_NONLINEAR_KHR in old versions of the specification.
     *
     * @param availableFormats The swap chain supported formats.
     * @return A VkSurfaceFormatKHR struct, representing the selected format.
     */
    VkSurfaceFormatKHR chooseSwapSurfaceFormat(const vector<VkSurfaceFormatKHR> &availableFormats) {
        // We want to use sRBG color space. Therefore, we want to check if the sRBG color space is supported
        // AND if the format used for such color space is available.
        for (const auto &availableFormat: availableFormats) {
            if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB &&
                availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
                return availableFormat;
            }
        }

        return availableFormats[0];
    }

    /**
     * @brief This function selects the preferred present mode.
     *
     * Note: Presentation mode is important because it represents the conditions for showing images to the screen.
     *
     * There are 4 present modes available in Vulkan:
     * 1. VK_PRESENT_MODE_IMMEDIATE_KHR: Images submitted by your application are transferred to the screen right away,
     * which may result in tearing.
     * 2. VK_PRESENT_MODE_FIFO_KHR: The swap chain is a queue where the display takes an image from the front of the
     * queue when the display is refreshed. The program inserts rendered images at the back of the queue. If the
     * queue is full then the program has to wait. This is most similar to vertical sync as found in modern games. The
     * moment that the display is refreshed is known as "vertical blank".
     * 3. VK_PRESENT_MODE_FIFO_RELAXED_KHR: This mode only differs from the previous one if the application is late and
     * the queue was empty at the last vertical blank. Instead of waiting for the next vertical blank, the image is
     * transferred right away when it finally arrives. This may result in visible tearing.
     * 4. VK_PRESENT_MODE_MAILBOX_KHR: This is another variation of the second mode. Instead of blocking the application
     * when the queue is full, the images that are already queued are simply replaced with the newer ones. This mode can
     * be used to render frames as fast as possible while still avoiding tearing, resulting in fewer latency issues than
     * standard vertical sync. This is commonly known as "triple buffering", although the existence of three buffers
     * alone does not necessarily mean that the framerate is unlocked.
     *
     * @param availablePresentModes A list of available present modes. Usually the presentModes member variable of SwapChainSupportDetails struct.
     * @return A VkPresentModeKHR struct, representing the selected present mode.
     */
    VkPresentModeKHR chooseSwapPresentMode(const vector<VkPresentModeKHR> &availablePresentModes) {
        for (const auto &availablePresentMode: availablePresentModes) {
            // We prefer VK_PRESENT_MODE_MAILBOX_KHR for its low latency.
            if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
                return availablePresentMode;
            }
        }

        return VK_PRESENT_MODE_FIFO_KHR;
    }

    /**
     * @brief Return the current extent (swap chain image resolution) if available. Otherwise, manually determine the
     * resolution.
     *
     * Note: The swap extent is the resolution of the swap chain images and it’s almost always exactly equal to the
     * resolution of the window that we’re drawing to in pixels. The range of the possible resolutions is defined in the
     * VkSurfaceCapabilitiesKHR structure.
     *
     * @param capabilities A VkSurfaceCapabilitiesKHR struct, representing the capabilities of the surface (min/max image size, current extent/resolution of the surface)
     * @return A VkExtent2D struct, representing a selected resolution.
     */
    VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR &capabilities) {
        // numeric_limits<uint32_t>::max(): returns the maximum possible value for the uint32_t type, which is 2^32-1.
        // Why use numeric_limits<uint32_t>::max() to determine if the surface provides a fixed resolution?
        // Because Vulkan sets currentExtent.width and currentExtent.height to the maximum possible value of type uint32_t
        // if the surface doesn't provide a fixed resolution.
        if (capabilities.currentExtent.width != numeric_limits<uint32_t>::max()) {
            return capabilities.currentExtent;
        } else {
            int width, height;
            // Query the actual framebuffer size from the window.
            glfwGetFramebufferSize(window, &width, &height);

            VkExtent2D actualExtent = {
                    static_cast<uint32_t>(width),
                    static_cast<uint32_t>(height)
            };

            // The clamp function is used here to bound the values of width and height between the allowed minimum and maximum extents that are supported.
            actualExtent.width = clamp(actualExtent.width, capabilities.minImageExtent.width,
                                       capabilities.maxImageExtent.width);
            actualExtent.height = clamp(actualExtent.height, capabilities.minImageExtent.height,
                                        capabilities.maxImageExtent.height);

            return actualExtent;
        }
    }

    SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device) {
        SwapChainSupportDetails details;

        // Retrieve surface capabilities
        // This function takes the specified VkPhysicalDevice and VkSurfaceKHR window surface into account when determining the supported capabilities.
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

        // Querying the supported surface formats
        uint32_t formatCount;
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);

        if (formatCount != 0) {
            details.formats.resize(formatCount);
            vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data());
        }

        // Querying the supported present modes
        uint32_t presentModeCount;
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);

        if (presentModeCount != 0) {
            details.presentModes.resize(presentModeCount);
            vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.presentModes.data());
        }

        return details;
    }

    QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device) {
        QueueFamilyIndices indices;

        // Query for available queues
        uint32_t queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

        vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

        // Find a queue family that support VK_QUEUE_GRAPHICS_BIT and assign its index to graphicsFamily
        int i = 0;
        for (const auto &queueFamily: queueFamilies) {
            if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                indices.graphicsFamily = i;
            }

            // Check whether the queue family support presentation
            VkBool32 presentSupport = false;
            vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, surface, &presentSupport);
            if (presentSupport) {
                indices.presentFamily = i;
            }

            // Stop when required queue families are found
            if (indices.isComplete()) {
                break;
            }

            i++;
        }

        return indices;
    }

    int rateDeviceSuitability(VkPhysicalDevice device) {
        // Query for the device details
        VkPhysicalDeviceProperties deviceProperties;
        vkGetPhysicalDeviceProperties(device, &deviceProperties);

        // Query for device optional features
        VkPhysicalDeviceFeatures deviceFeatures;
        vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

        int score = 0;

        // Discrete GPUs have a significant performance advantage
        if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
            score += 1000;
        }

        // Maximum possible size of textures affects graphics quality
        score += deviceProperties.limits.maxImageDimension2D;

        // This application can't function without geometry shaders.
        // NOTE: GPUs doesn't support this feature. Turn this condition off temporarily in order to follow the tutorial.
        // Come back and find alternatives to geometryShare.
//        if (!deviceFeatures.geometryShader) {
//            return 0;
//        }

        return score;
    }

    bool isDeviceSuitable(VkPhysicalDevice device) {
        QueueFamilyIndices indices = findQueueFamilies(device);

        bool extensionsSupported = checkDeviceExtensionSupport(device);

        bool swapChainAdequate = false;
        if (extensionsSupported) {
            SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device);
            swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
        }

        return indices.isComplete()
               && extensionsSupported
               && swapChainAdequate;
    }

    /**
     * @brief Retrieve all the available extensions for the physical device. Return true if they support the required extensions. Otherwise, return False.
     *
     * @param device A physical device.
     * @return True if they support the required extensions. Otherwise, return False.
     *
     */
    bool checkDeviceExtensionSupport(VkPhysicalDevice device) {
        // Retrieve a list of supported extensions.
        // VkExtensionProperties struct contains the name and version of an extension.
        uint32_t extensionCount = 0;
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);
        vector<VkExtensionProperties> availableExtensions(
                extensionCount); // Allocate an array to hold extension properties
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount,
                                             availableExtensions.data()); // Query for the extension details

        // Copy the list of required extensions. Remove an extension from the list if they are supported.
        // If the list is empty after exiting the loop, it means all the required extensions are supported.
        set<string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

        for (const auto &extension: availableExtensions) {
            requiredExtensions.erase(extension.extensionName);
        }

        return requiredExtensions.empty();
    }

    void setupDebugMessenger() {
        if (!enableValidationLayers) return;

        VkDebugUtilsMessengerCreateInfoEXT createInfo;
        populateDebugMessengerCreateInfo(createInfo);

        if (CreateDebugUtilsMessengerEXT(instance, &createInfo, nullptr, &debugMessenger) != VK_SUCCESS) {
            throw runtime_error("Failed to set up debug messenger");
        };
    }

    void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT &createInfo) {
        createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        createInfo.messageSeverity =
                VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        createInfo.messageType =
                VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        createInfo.pfnUserCallback = debugCallback;
        createInfo.pUserData = nullptr; // Optional
    }

    void mainLoop() {
        while (!glfwWindowShouldClose(window)) {
            glfwPollEvents();
        }
    }

    void cleanup() {
        if (enableValidationLayers) {
            DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
        }

        for (auto imageView : swapChainImageViews) {
            vkDestroyImageView(logicalDevice, imageView, nullptr);
        }
        vkDestroySwapchainKHR(logicalDevice, swapChain, nullptr);
        vkDestroyDevice(logicalDevice, nullptr);
        vkDestroySurfaceKHR(instance, surface, nullptr);
        vkDestroyInstance(instance, nullptr);
        glfwDestroyWindow(window);
        glfwTerminate();
    }

    void createInstance() {
        // Throw error if unable to turn on validation layers in debug mode
        if (enableValidationLayers && !checkValidationLayerSupport()) {
            throw runtime_error("Validation layers requested, but not available.");
        }

        VkApplicationInfo appInfo{};
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pApplicationName = "Triangle";
        appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.pEngineName = "No Engine";
        appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.apiVersion = VK_API_VERSION_1_0;

        // This struct tells Vulkan driver which global extensions and validation layers we want to use.
        VkInstanceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        createInfo.pApplicationInfo = &appInfo;
        createInfo.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;

        vector<const char *> requiredExtensions = getRequiredExtensions();

        // Temporarily disable this for now
        createInfo.enabledExtensionCount = (uint32_t) requiredExtensions.size();
        createInfo.ppEnabledExtensionNames = requiredExtensions.data();
//        createInfo.enabledExtensionCount = 0;

        // Include Validation layer names and add a debug messenger if validation layers are enabled
        VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
        if (enableValidationLayers) {
            createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
            createInfo.ppEnabledLayerNames = validationLayers.data();

            populateDebugMessengerCreateInfo(debugCreateInfo);
            createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT *) &debugCreateInfo;
        } else {
            createInfo.enabledLayerCount = 0;

            createInfo.pNext = nullptr;
        }

        if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS) {
            throw runtime_error("failed to create instance");
        }

//        /*
//         * To retrieve a list of supported extensions
//         * VkExtentionProperties struct contains the name and version of an extension
//         */
//        uint32_t extensionCount = 0;
//        vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
//        vector<VkExtensionProperties> extensions(extensionCount); // Allocate an array to hold extension properties
//        vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount,
//                                               extensions.data()); // Query for the extension details
//
//        cout << "Extension: " << endl;
//        for (const auto &extension: extensions) {
//            cout << extension.extensionName << endl;
//        }
    }

    /**
     * @brief Get all the required extensions to allow platform-specific communication between the instance and hardware APIs
     *
     * Vulkan is a platform-agnostic API, which means that you need an extension to interface with specific window system (of macOS in this case)
     * We can use glfw function to obtain the extension(s)
     * Typical Vulkan extensions for Window Systems
     * VK_KHR_surface: A core extension that provides an abstraction for creating surfaces.
     * VK_KHR_swapchain: Manages the presentation of images to the surface in a windowing system.
     * Platform-specific Extensions: These include platform-specific extensions like VK_KHR_xcb_surface, VK_KHR_win32_surface, or VK_EXT_metal_surface, which allow Vulkan to work with the corresponding windowing system.
     *
     * @return A vector of required extensions.
     */
    vector<const char *> getRequiredExtensions() {
        uint32_t glfwExtensionCount = 0;
        const char **glfwExtensions;

        glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

        // This step is required for macOS only. As starting from 1.3.216 Vulkan SDK, the VK_KHR_PORTABILITY_subset extension is mandatory.
        vector<const char *> requiredExtensions;
        for (uint32_t i = 0; i < glfwExtensionCount; i++) {
            requiredExtensions.emplace_back(glfwExtensions[i]);
        }

        requiredExtensions.emplace_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);

        // This step is required to set up callback to handle debug messages
        if (enableValidationLayers) {
            requiredExtensions.emplace_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        }

        return requiredExtensions;
    }

    /**
     * @brief Check if all the requested layers are available
     *
     * @return True if all the requested layers are available. False otherwise.
     */
    bool checkValidationLayerSupport() {
        uint32_t layerCount;

        // List all the available layers.
        vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

        vector<VkLayerProperties> availableLayers(layerCount); // Allocate an array to hold the layer details
        vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

        // Check whether all the layers in validation layers exist in available layers
        for (const char *layerName: validationLayers) {
            bool layerFound = false;

            for (const auto &layerProperties: availableLayers) {
                if (strcmp(layerName, layerProperties.layerName) == 0) {
                    layerFound = true;
                    break;
                }
            }

            if (!layerFound) {
                return false;
            }
        }

        return true;
    }
};

int main() {
    TriangleApp app;

    try {
        app.run();
    } catch (const exception &e) {
        cerr << e.what() << endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}