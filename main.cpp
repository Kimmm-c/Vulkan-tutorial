#define GLFW_INCLUDE_VULKAN

#include <glfw3.h>

#include <iostream>
#include <stdexcept>
#include <cstdlib>
#include <vector>
#include <cstring>

using namespace std;

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
VkResult CreateDebugUtilsMessengerEXT(VkInstance instance
                                      , const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo
                                      , const VkAllocationCallbacks* pAllocator
                                      , VkDebugUtilsMessengerEXT* pDebugMessenger) {
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
void DestroyDebugUtilsMessengerEXT(VkInstance instance
                                   , VkDebugUtilsMessengerEXT debugMessenger
                                   , const VkAllocationCallbacks* pAllocator) {
    // Similar to vkCreateDebugUtilsMessengerEXT, vkDestroyDebugUtilsMessengerEXT has to be loaded using vkGetInstanceProcAddr.
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr) {
        func(instance, debugMessenger, pAllocator);
    }
}

class TriangleApp {
    GLFWwindow *window;
    VkInstance instance;
    VkDebugUtilsMessengerEXT debugMessenger;

    // Conditionally turn on the validation layers if the program is compiled in debug mode. Otherwise, turn off the validation layers to improve performance.
    const vector<const char *> validationLayers{"VK_LAYER_KHRONOS_validation"};
#ifdef NDEBUG
    const bool enableValidationLayers = false;
#else
    const bool enableValidationLayers = true;
#endif // NDEBUG

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
            VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity
            , VkDebugUtilsMessageTypeFlagsEXT messageType
            , const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData
            , void* pUserData) {
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
        setupDebugMessenger();

        // Pick a physical device (a graphic card) the supports the Vulkan library features.
        pickPhysicalDevice();
    }

    void pickPhysicalDevice() {
        VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;

        // Get all available graphics cards
        uint32_t deviceCount = 0;
        vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

        if (deviceCount == 0) {
            throw runtime_error("Failed to find GPUs with Vulkan support");
        }

        // Allocate an array to hold the devices and retrieve device details
        vector<VkPhysicalDevice> devices(deviceCount);
        vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

        // Assign only the valid physical device to physicalDevice
        for (const auto& device : devices) {
            if (isDeviceSuitable(device)) {
                physicalDevice = device;
                break;
            }
        }

        // Raise an error if unable to find a suitable GPU for the instance.
        if (physicalDevice == VK_NULL_HANDLE) {
            throw runtime_error("Failed to find a suitable GPU");
        }
    }

    bool isDeviceSuitable(VkPhysicalDevice) {
        return true;
    }

    void setupDebugMessenger() {
        if (!enableValidationLayers) return;

        VkDebugUtilsMessengerCreateInfoEXT createInfo;
        populateDebugMessengerCreateInfo(createInfo);

        if (CreateDebugUtilsMessengerEXT(instance, &createInfo, nullptr, &debugMessenger) != VK_SUCCESS) {
            throw runtime_error("Failed to set up debug messenger");
        };
    }

    void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo) {
        createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
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

        auto requiredExtensions = getRequiredExtensions();

        createInfo.enabledExtensionCount = (uint32_t) requiredExtensions.size();
        createInfo.ppEnabledExtensionNames = requiredExtensions.data();

        // Include Validation layer names and add a debug messenger if validation layers are enabled
        VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
        if (enableValidationLayers) {
            createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
            createInfo.ppEnabledLayerNames = validationLayers.data();

            populateDebugMessengerCreateInfo(debugCreateInfo);
            createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*) &debugCreateInfo;
        } else {
            createInfo.enabledLayerCount = 0;

            createInfo.pNext = nullptr;
        }

        if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS) {
            throw runtime_error("failed to create instance");
        }

        /*
         * To retrieve a list of supported extensions
         * VkExtentionProperties struct contains the name and version of an extension
         */
        uint32_t extensionCount = 0;
        vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
        vector<VkExtensionProperties> extensions(extensionCount); // Allocate an array to hold extension properties
        vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount,
                                               extensions.data()); // Query for the extension details

        cout << "Extension: " << endl;
        for (const auto &extension: extensions) {
            cout << extension.extensionName << endl;
        }
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
    vector<const char*> getRequiredExtensions() {
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
        for (const char* layerName : validationLayers) {
            bool layerFound = false;

            for (const auto& layerProperties : availableLayers) {
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