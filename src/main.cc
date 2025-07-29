#define VULKAN_HPP_NO_EXCEPTIONS
#define VULKAN_HPP_NO_SMART_HANDLE
#define VULKAN_HPP_ASSERT_ON_RESULT
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_handles.hpp>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <fstream>
#include <iostream>
#include <vector>

const std::vector<const char *> VALIDATION_LAYERS = {"VK_LAYER_KHRONOS_validation"};

#ifndef NDEBUG
const bool ENABLE_VALIDATION_LAYERS = true;
#else
const bool ENABLE_VALIDATION_LAYERS = false;
#endif

struct QueueFamilyIndices
{
    std::optional<uint32_t> graphics_family;
    std::optional<uint32_t> present_family;

    bool is_complete()
    {
        return graphics_family.has_value() && present_family.has_value();
    }
};

struct SwapChainSupportDetails
{
    vk::SurfaceCapabilitiesKHR capabilities;
    std::vector<vk::SurfaceFormatKHR> formats;
    std::vector<vk::PresentModeKHR> present_modes;
};

static std::vector<uint8_t> read_file(const std::string &filename)
{
    std::ifstream f(filename, std::ios::ate | std::ios::binary);
    if (!f.is_open())
    {
        std::cerr << "failed to open file: " << filename << std::endl;
        exit(EXIT_FAILURE);
    }

    size_t file_size = (size_t)f.tellg();
    std::vector<uint8_t> buffer(file_size);

    f.seekg(0);
    f.read((char *)buffer.data(), file_size);
    f.close();

    return buffer;
}

class LearnVulkanApp
{
  public:
    void run()
    {
        init_window();
        init_vulkan();
        main_loop();
        cleanup();
    }

  private:
    GLFWwindow *window;
    vk::Instance instance;

    std::vector<vk::ExtensionProperties> supported_extensions;
    std::vector<vk::LayerProperties> supported_layers;

    vk::PhysicalDevice physical_device;
    vk::SurfaceKHR surface;

    vk::Device device;
    vk::Queue graphics_queue;
    vk::Queue present_queue;

    vk::SwapchainKHR swapchain;
    vk::Format swapchain_image_format;
    vk::Extent2D swapchain_extent;
    std::vector<vk::Image> swapchain_images;
    std::vector<vk::ImageView> swapchain_image_views;

    vk::RenderPass render_pass;
    vk::PipelineLayout pipeline_layout;
    vk::Pipeline graphics_pipeline;

    std::vector<vk::Framebuffer> swapchain_framebuffers;
    vk::CommandPool command_pool;
    vk::CommandBuffer command_buffer;

    vk::Semaphore sem_image_available;
    vk::Semaphore sem_render_finished;
    vk::Fence fence_in_flight;

    void init_vulkan()
    {
        create_instance();
        create_surface();
        pick_physical_device();
        create_logical_device();
        create_swap_chain();
        create_image_views();
        create_render_pass();
        create_graphics_pipeline();
        create_framebuffers();
        create_command_pool();
        create_command_buffer();
        create_sync_objects();
    }

    QueueFamilyIndices find_queue_families(vk::PhysicalDevice device)
    {
        QueueFamilyIndices indices;
        std::vector<vk::QueueFamilyProperties> queueFamilies = device.getQueueFamilyProperties();

        int i = 0;
        for (const auto &queueFamily : queueFamilies)
        {
            if (queueFamily.queueFlags & vk::QueueFlagBits::eGraphics)
            {
                indices.graphics_family = i;
            }

            auto res = device.getSurfaceSupportKHR(i, surface);
            if (res.result == vk::Result::eSuccess && res.value)
            {
                indices.present_family = i;
            }
            else if (res.result != vk::Result::eSuccess)
            {
                std::cerr << "failed to get surface support" << std::endl;
                exit(EXIT_FAILURE);
            }

            i++;
        }

        return indices;
    }

    SwapChainSupportDetails query_swap_chain_support(vk::PhysicalDevice device)
    {
        SwapChainSupportDetails details;
        auto capabilities = device.getSurfaceCapabilitiesKHR(surface);
        if (capabilities.result != vk::Result::eSuccess)
        {
            std::cerr << "failed to get surface capabilities" << std::endl;
            exit(EXIT_FAILURE);
        }
        details.capabilities = capabilities.value;

        auto formats = device.getSurfaceFormatsKHR(surface);
        if (formats.result != vk::Result::eSuccess)
        {
            std::cerr << "failed to get surface formats" << std::endl;
            exit(EXIT_FAILURE);
        }
        details.formats = formats.value;

        auto present_modes = device.getSurfacePresentModesKHR(surface);
        if (present_modes.result != vk::Result::eSuccess)
        {
            std::cerr << "failed to get surface present modes" << std::endl;
            exit(EXIT_FAILURE);
        }
        details.present_modes = present_modes.value;

        return details;
    }

    bool extension_supported(const char *extension_name)
    {
        for (const auto &extension : supported_extensions)
        {
            if (strcmp(extension.extensionName, extension_name) == 0)
            {
                return true;
            }
        }
        return false;
    }

    bool layer_supported(const char *layer_name)
    {
        for (const auto &layer : supported_layers)
        {
            if (strcmp(layer.layerName, layer_name) == 0)
            {
                return true;
            }
        }
        return false;
    }

    static void glfw_error_cb(int error, const char *description)
    {
        std::cerr << "GLFW error: " << description << std::endl;
    }

    void init_window()
    {
        glfwInit();
        std::cout << "GLFW version: " << glfwGetVersionString() << std::endl;

        glfwSetErrorCallback(glfw_error_cb);
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

        window = glfwCreateWindow(800, 600, "Learn Vulkan", nullptr, nullptr);
        if (window == nullptr)
        {
            std::cerr << "failed to create window" << std::endl;
            exit(EXIT_FAILURE);
        }
    }

    void create_instance()
    {
        auto extensions = vk::enumerateInstanceExtensionProperties();
        supported_extensions = extensions.value;

        auto layers = vk::enumerateInstanceLayerProperties();
        supported_layers = layers.value;

        vk::ApplicationInfo app_info{};
        app_info.pApplicationName = "Learn Vulkan";
        app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        app_info.pEngineName = "No Engine";
        app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
        app_info.apiVersion = VK_API_VERSION_1_0;

        vk::InstanceCreateInfo create_info{};
        create_info.pApplicationInfo = &app_info;
        create_info.enabledLayerCount = 0;

        if (!glfwVulkanSupported())
        {
            std::cerr << "GLFW failed to find Vulkan support" << std::endl;
            exit(EXIT_FAILURE);
        }

        uint32_t glfw_extension_count = 0;
        const char **glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extension_count);

        if (glfw_extensions == nullptr)
        {
            std::cerr << "failed to get GLFW Vulkan extensions" << std::endl;
            exit(EXIT_FAILURE);
        }

        std::vector<const char *> required_extensions;

        for (uint32_t i = 0; i < glfw_extension_count; i++)
        {
            required_extensions.push_back(glfw_extensions[i]);
        }
        required_extensions.push_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
        required_extensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);

        for (const auto &extension : required_extensions)
        {
            // check extension support
            if (!extension_supported(extension))
            {
                std::cerr << "required extension not supported: " << extension << std::endl;
                exit(EXIT_FAILURE);
            }
        }

        create_info.setFlags(vk::InstanceCreateFlagBits::eEnumeratePortabilityKHR);
        create_info.setPEnabledExtensionNames(required_extensions);

        if (ENABLE_VALIDATION_LAYERS)
        {
            // load validation layers
            std::cerr << "validation layers enabled" << std::endl;
            create_info.setPEnabledLayerNames(VALIDATION_LAYERS);

            for (const auto &layer : VALIDATION_LAYERS)
            {
                // check layer support
                if (!layer_supported(layer))
                {
                    std::cerr << "required layer not supported: " << layer << std::endl;
                    exit(EXIT_FAILURE);
                }
            }
        }

        auto res = vk::createInstance(create_info);
        if (res.result != vk::Result::eSuccess)
        {
            std::cerr << "failed to create Vulkan instance: " << vk::to_string(res.result) << std::endl;
            exit(EXIT_FAILURE);
        }

        instance = res.value;
    }

    bool is_device_suitable(VkPhysicalDevice device)
    {
        QueueFamilyIndices indices = find_queue_families(device);
        SwapChainSupportDetails swap_chain_support = query_swap_chain_support(device);

        const bool swap_chain_adequate =
            !swap_chain_support.formats.empty() && !swap_chain_support.present_modes.empty();

        return indices.is_complete() && swap_chain_adequate;
    }

    void pick_physical_device()
    {
        uint32_t device_count = 0;
        vkEnumeratePhysicalDevices(instance, &device_count, nullptr);
        if (device_count == 0)
        {
            std::cerr << "failed to find GPUs with Vulkan support" << std::endl;
            exit(EXIT_FAILURE);
        }

        std::cerr << "found " << device_count << " devices" << std::endl;

        std::vector<VkPhysicalDevice> devices(device_count);
        vkEnumeratePhysicalDevices(instance, &device_count, devices.data());

        for (const auto &device : devices)
        {
            if (is_device_suitable(device))
            {
                physical_device = device;

                auto device_props = VkPhysicalDeviceProperties{};
                vkGetPhysicalDeviceProperties(physical_device, &device_props);

                std::cerr << "picked device: " << device_props.deviceName << std::endl;
                break;
            }
        }

        if (physical_device == VK_NULL_HANDLE)
        {
            std::cerr << "failed to find a suitable GPU" << std::endl;
            exit(EXIT_FAILURE);
        }
    }

    void create_logical_device()
    {
        QueueFamilyIndices indices = find_queue_families(physical_device);
        if (indices.graphics_family != indices.present_family)
        {
            std::cerr << "graphics and present queues are different" << std::endl;
            exit(EXIT_FAILURE);
        }

        vk::DeviceQueueCreateInfo queue_create_info{};
        queue_create_info.queueFamilyIndex = indices.graphics_family.value();
        queue_create_info.queueCount = 1;
        float queuePriority = 1.0f;
        queue_create_info.pQueuePriorities = &queuePriority;

        vk::PhysicalDeviceFeatures device_features{};

        std::vector<const char *> device_extensions = {"VK_KHR_portability_subset", VK_KHR_SWAPCHAIN_EXTENSION_NAME};

        vk::DeviceCreateInfo create_info{};
        create_info.setQueueCreateInfos(queue_create_info);
        create_info.pEnabledFeatures = &device_features;
        create_info.setPEnabledExtensionNames(device_extensions);

        if (ENABLE_VALIDATION_LAYERS)
        {
            create_info.setPEnabledLayerNames(VALIDATION_LAYERS);
        }

        // if (vkCreateDevice(physical_device, &create_info, nullptr, &device) != VK_SUCCESS)
        // {
        //     std::cerr << "failed to create logical device" << std::endl;
        //     exit(EXIT_FAILURE);
        // }

        auto res = physical_device.createDevice(create_info);
        if (res.result != vk::Result::eSuccess)
        {
            std::cerr << "failed to create logical device" << std::endl;
            exit(EXIT_FAILURE);
        }

        device = res.value;

        graphics_queue = device.getQueue(indices.graphics_family.value(), 0);
        present_queue = device.getQueue(indices.present_family.value(), 0);
    }

    void create_surface()
    {
        VkSurfaceKHR surface;
        if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS)
        {
            std::cerr << "failed to create window surface" << std::endl;
            exit(EXIT_FAILURE);
        }

        this->surface = surface;
    }

    vk::SurfaceFormatKHR choose_swap_surface_format(const std::vector<vk::SurfaceFormatKHR> &available_formats)
    {
        for (const auto &available_format : available_formats)
        {
            if (available_format.format == vk::Format::eB8G8R8A8Srgb &&
                available_format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear)
            {
                return available_format;
            }
        }

        return available_formats[0];
    }

    vk::PresentModeKHR choose_swap_present_mode(const std::vector<vk::PresentModeKHR> &available_present_modes)
    {
        for (const auto &available_present_mode : available_present_modes)
        {
            if (available_present_mode == vk::PresentModeKHR::eMailbox)
            {
                return available_present_mode;
            }
        }

        return vk::PresentModeKHR::eFifo;
    }

    VkExtent2D choose_swap_extent(const VkSurfaceCapabilitiesKHR &capabilities)
    {
        if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
        {
            return capabilities.currentExtent;
        }
        else
        {
            int width, height;
            glfwGetFramebufferSize(window, &width, &height);

            VkExtent2D actual_extent = {static_cast<uint32_t>(width), static_cast<uint32_t>(height)};

            actual_extent.width =
                std::clamp(actual_extent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
            actual_extent.height = std::clamp(actual_extent.height, capabilities.minImageExtent.height,
                                              capabilities.maxImageExtent.height);

            return actual_extent;
        }
    }

    void create_swap_chain()
    {
        SwapChainSupportDetails swap_chain_support = query_swap_chain_support(physical_device);

        vk::SurfaceFormatKHR surface_format = choose_swap_surface_format(swap_chain_support.formats);
        vk::PresentModeKHR present_mode = choose_swap_present_mode(swap_chain_support.present_modes);
        vk::Extent2D extent = choose_swap_extent(swap_chain_support.capabilities);

        uint32_t image_count = swap_chain_support.capabilities.minImageCount + 1;
        if (swap_chain_support.capabilities.maxImageCount > 0)
        {
            image_count = std::min(image_count, swap_chain_support.capabilities.maxImageCount);
        }

        vk::SwapchainCreateInfoKHR create_info{};
        create_info.surface = surface;
        create_info.minImageCount = image_count;
        create_info.imageFormat = surface_format.format;
        create_info.imageColorSpace = surface_format.colorSpace;
        create_info.imageExtent = extent;
        create_info.imageArrayLayers = 1;
        create_info.imageUsage = vk::ImageUsageFlagBits::eColorAttachment;
        create_info.imageSharingMode = vk::SharingMode::eExclusive;
        create_info.preTransform = swap_chain_support.capabilities.currentTransform;
        create_info.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;
        create_info.presentMode = present_mode;
        create_info.clipped = VK_TRUE;
        create_info.oldSwapchain = VK_NULL_HANDLE;

        auto res = device.createSwapchainKHR(create_info);
        if (res.result != vk::Result::eSuccess)
        {
            std::cerr << "failed to create swap chain" << std::endl;
            exit(EXIT_FAILURE);
        }
        swapchain = res.value;

        auto swapchain_images_res = device.getSwapchainImagesKHR(swapchain);
        if (swapchain_images_res.result != vk::Result::eSuccess)
        {
            std::cerr << "failed to get swapchain images" << std::endl;
            exit(EXIT_FAILURE);
        }
        swapchain_images = swapchain_images_res.value;

        swapchain_image_format = surface_format.format;
        swapchain_extent = extent;
    }

    void create_image_views()
    {
        swapchain_image_views.resize(swapchain_images.size());
        for (size_t i = 0; i < swapchain_images.size(); i++)
        {
            vk::ImageViewCreateInfo create_info{};
            create_info.image = swapchain_images[i];
            create_info.viewType = vk::ImageViewType::e2D;
            create_info.format = swapchain_image_format;

            create_info.components.r = vk::ComponentSwizzle::eIdentity;
            create_info.components.g = vk::ComponentSwizzle::eIdentity;
            create_info.components.b = vk::ComponentSwizzle::eIdentity;
            create_info.components.a = vk::ComponentSwizzle::eIdentity;

            create_info.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
            create_info.subresourceRange.baseMipLevel = 0;
            create_info.subresourceRange.levelCount = 1;
            create_info.subresourceRange.baseArrayLayer = 0;
            create_info.subresourceRange.layerCount = 1;

            auto res = device.createImageView(create_info);
            if (res.result != vk::Result::eSuccess)
            {
                std::cerr << "failed to create image views" << std::endl;
                exit(EXIT_FAILURE);
            }

            swapchain_image_views[i] = res.value;
        }
    }

    vk::ShaderModule create_shader_module(const std::vector<uint8_t> &code)
    {
        if (code.size() % 4 != 0)
        {
            std::cerr << "shader code size is not a multiple of 4" << std::endl;
            exit(EXIT_FAILURE);
        }

        vk::ShaderModuleCreateInfo createInfo{};
        createInfo.codeSize = code.size();
        createInfo.pCode = reinterpret_cast<const uint32_t *>(code.data());

        auto res = device.createShaderModule(createInfo);
        if (res.result != vk::Result::eSuccess)
        {
            std::cerr << "failed to create shader module" << std::endl;
            exit(EXIT_FAILURE);
        }

        return res.value;
    }

    void create_render_pass()
    {
        vk::AttachmentDescription color_attachment{};
        color_attachment.format = swapchain_image_format;
        color_attachment.samples = vk::SampleCountFlagBits::e1;
        color_attachment.loadOp = vk::AttachmentLoadOp::eClear;
        color_attachment.storeOp = vk::AttachmentStoreOp::eStore;
        color_attachment.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
        color_attachment.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
        color_attachment.initialLayout = vk::ImageLayout::eUndefined;
        color_attachment.finalLayout = vk::ImageLayout::ePresentSrcKHR;

        vk::AttachmentReference color_attachment_ref{};
        color_attachment_ref.attachment = 0;
        color_attachment_ref.layout = vk::ImageLayout::eColorAttachmentOptimal;

        vk::SubpassDescription subpass{};
        subpass.pipelineBindPoint = vk::PipelineBindPoint::eGraphics;
        subpass.setColorAttachments(color_attachment_ref);

        vk::SubpassDependency dependency{};
        dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
        dependency.dstSubpass = 0;
        dependency.srcStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
        dependency.srcAccessMask = vk::AccessFlagBits::eNoneKHR;
        dependency.dstStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
        dependency.dstAccessMask = vk::AccessFlagBits::eColorAttachmentWrite;

        vk::RenderPassCreateInfo render_pass_info{};
        render_pass_info.setAttachments(color_attachment);
        render_pass_info.setSubpasses(subpass);
        render_pass_info.setDependencies(dependency);

        auto res = device.createRenderPass(render_pass_info);
        if (res.result != vk::Result::eSuccess)
        {
            std::cerr << "failed to create render pass" << std::endl;
            exit(EXIT_FAILURE);
        }
        render_pass = res.value;
    }

    void create_graphics_pipeline()
    {
        std::vector<uint8_t> vert_code = read_file("shaders/tri.vert.spv");
        std::vector<uint8_t> frag_code = read_file("shaders/tri.frag.spv");

        vk::ShaderModule vert_module = create_shader_module(vert_code);
        vk::ShaderModule frag_module = create_shader_module(frag_code);

        vk::PipelineShaderStageCreateInfo vert_stage_info{};
        vert_stage_info.stage = vk::ShaderStageFlagBits::eVertex;
        vert_stage_info.module = vert_module;
        vert_stage_info.pName = "main";

        vk::PipelineShaderStageCreateInfo frag_stage_info{};
        frag_stage_info.stage = vk::ShaderStageFlagBits::eFragment;
        frag_stage_info.module = frag_module;
        frag_stage_info.pName = "main";

        vk::PipelineShaderStageCreateInfo shader_stages[] = {vert_stage_info, frag_stage_info};

        vk::DynamicState dynamic_states[] = {vk::DynamicState::eViewport, vk::DynamicState::eScissor};
        vk::PipelineDynamicStateCreateInfo dynamic_state{};
        dynamic_state.setDynamicStates(dynamic_states);

        vk::PipelineVertexInputStateCreateInfo vertex_input_info{};
        vertex_input_info.vertexBindingDescriptionCount = 0;
        vertex_input_info.pVertexBindingDescriptions = nullptr;
        vertex_input_info.vertexAttributeDescriptionCount = 0;
        vertex_input_info.pVertexAttributeDescriptions = nullptr;

        vk::PipelineInputAssemblyStateCreateInfo input_assembly{};
        input_assembly.topology = vk::PrimitiveTopology::eTriangleList;
        input_assembly.primitiveRestartEnable = VK_FALSE;

        vk::Viewport viewport{};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = (float)swapchain_extent.width;
        viewport.height = (float)swapchain_extent.height;
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;

        vk::Rect2D scissor{};
        scissor.offset = vk::Offset2D{0, 0};
        scissor.extent = swapchain_extent;

        vk::PipelineViewportStateCreateInfo viewport_state{};
        viewport_state.setViewports(viewport);
        viewport_state.setScissors(scissor);

        vk::PipelineRasterizationStateCreateInfo rasterizer{};
        rasterizer.depthClampEnable = VK_FALSE;
        rasterizer.rasterizerDiscardEnable = VK_FALSE;
        rasterizer.polygonMode = vk::PolygonMode::eFill;
        rasterizer.lineWidth = 1.0f;
        rasterizer.cullMode = vk::CullModeFlagBits::eBack;
        rasterizer.frontFace = vk::FrontFace::eClockwise;
        rasterizer.depthBiasEnable = VK_FALSE;
        rasterizer.depthBiasConstantFactor = 0.0f;
        rasterizer.depthBiasClamp = 0.0f;
        rasterizer.depthBiasSlopeFactor = 0.0f;

        vk::PipelineMultisampleStateCreateInfo multisampling{};
        multisampling.sampleShadingEnable = VK_FALSE;
        multisampling.rasterizationSamples = vk::SampleCountFlagBits::e1;
        multisampling.minSampleShading = 1.0f;
        multisampling.pSampleMask = nullptr;
        multisampling.alphaToCoverageEnable = VK_FALSE;
        multisampling.alphaToOneEnable = VK_FALSE;

        vk::PipelineColorBlendAttachmentState color_blend_attachment{};
        color_blend_attachment.colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
                                                vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;
        color_blend_attachment.blendEnable = VK_FALSE;
        color_blend_attachment.srcColorBlendFactor = vk::BlendFactor::eOne;
        color_blend_attachment.dstColorBlendFactor = vk::BlendFactor::eZero;
        color_blend_attachment.colorBlendOp = vk::BlendOp::eAdd;
        color_blend_attachment.srcAlphaBlendFactor = vk::BlendFactor::eOne;
        color_blend_attachment.dstAlphaBlendFactor = vk::BlendFactor::eZero;
        color_blend_attachment.alphaBlendOp = vk::BlendOp::eAdd;

        vk::PipelineColorBlendStateCreateInfo color_blending{};
        color_blending.logicOpEnable = VK_FALSE;
        color_blending.logicOp = vk::LogicOp::eCopy;
        color_blending.setAttachments(color_blend_attachment);
        color_blending.blendConstants[0] = 0.0f;
        color_blending.blendConstants[1] = 0.0f;
        color_blending.blendConstants[2] = 0.0f;
        color_blending.blendConstants[3] = 0.0f;

        vk::PipelineLayoutCreateInfo pipeline_layout_info{};
        auto pipeline_layout_res = device.createPipelineLayout(pipeline_layout_info);
        if (pipeline_layout_res.result != vk::Result::eSuccess)
        {
            std::cerr << "failed to create pipeline layout" << std::endl;
            exit(EXIT_FAILURE);
        }
        pipeline_layout = pipeline_layout_res.value;

        vk::GraphicsPipelineCreateInfo pipeline_info{};
        pipeline_info.setStages(shader_stages);
        pipeline_info.pVertexInputState = &vertex_input_info;
        pipeline_info.pInputAssemblyState = &input_assembly;
        pipeline_info.pViewportState = &viewport_state;
        pipeline_info.pRasterizationState = &rasterizer;
        pipeline_info.pMultisampleState = &multisampling;
        pipeline_info.pColorBlendState = &color_blending;
        pipeline_info.pDepthStencilState = nullptr;
        pipeline_info.pDynamicState = &dynamic_state;
        pipeline_info.layout = pipeline_layout;
        pipeline_info.renderPass = render_pass;
        pipeline_info.subpass = 0;

        auto res = device.createGraphicsPipeline(nullptr, pipeline_info);
        if (res.result != vk::Result::eSuccess)
        {
            std::cerr << "failed to create graphics pipeline" << std::endl;
            exit(EXIT_FAILURE);
        }
        graphics_pipeline = res.value;

        device.destroyShaderModule(vert_module);
        device.destroyShaderModule(frag_module);
    }

    void create_framebuffers()
    {
        for (vk::ImageView image_view : swapchain_image_views)
        {
            vk::FramebufferCreateInfo fb_info{};
            fb_info.renderPass = render_pass;
            fb_info.setAttachments(image_view);
            fb_info.width = swapchain_extent.width;
            fb_info.height = swapchain_extent.height;
            fb_info.layers = 1;

            auto res = device.createFramebuffer(fb_info);
            if (res.result != vk::Result::eSuccess)
            {
                std::cerr << "failed to create framebuffer" << std::endl;
                exit(EXIT_FAILURE);
            }

            swapchain_framebuffers.push_back(res.value);
        }
    }

    void create_command_pool()
    {
        QueueFamilyIndices queue_family_indices = find_queue_families(physical_device);

        vk::CommandPoolCreateInfo pool_info{};
        pool_info.flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer;
        pool_info.queueFamilyIndex = queue_family_indices.graphics_family.value();

        auto res = device.createCommandPool(pool_info);
        if (res.result != vk::Result::eSuccess)
        {
            std::cerr << "failed to create command pool" << std::endl;
            exit(EXIT_FAILURE);
        }
        command_pool = res.value;
    }

    void create_command_buffer()
    {
        vk::CommandBufferAllocateInfo alloc_info{};
        alloc_info.commandPool = command_pool;
        alloc_info.level = vk::CommandBufferLevel::ePrimary;
        alloc_info.commandBufferCount = 1;

        auto res = device.allocateCommandBuffers(alloc_info);
        if (res.result != vk::Result::eSuccess || res.value.size() != 1)
        {
            std::cerr << "failed to allocate command buffers" << std::endl;
            exit(EXIT_FAILURE);
        }
        command_buffer = res.value[0];
    }

    void record_command_buffer(vk::CommandBuffer command_buffer, uint32_t image_index)
    {
        vk::CommandBufferBeginInfo begin_info{};
        if (command_buffer.begin(begin_info) != vk::Result::eSuccess)
        {
            std::cerr << "failed to begin recording command buffer" << std::endl;
            exit(EXIT_FAILURE);
        }

        vk::RenderPassBeginInfo render_pass_info{};
        render_pass_info.renderPass = render_pass;
        render_pass_info.framebuffer = swapchain_framebuffers[image_index];
        render_pass_info.renderArea.offset = vk::Offset2D{0, 0};
        render_pass_info.renderArea.extent = swapchain_extent;
        vk::ClearValue clear_value({0.0f, 0.0f, 0.0f, 1.0f});
        render_pass_info.setClearValues(clear_value);

        command_buffer.beginRenderPass(render_pass_info, vk::SubpassContents::eInline);
        command_buffer.bindPipeline(vk::PipelineBindPoint::eGraphics, graphics_pipeline);

        vk::Viewport viewport{};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = (float)swapchain_extent.width;
        viewport.height = (float)swapchain_extent.height;
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        command_buffer.setViewport(0, viewport);

        vk::Rect2D scissor{};
        scissor.offset = vk::Offset2D{0, 0};
        scissor.extent = swapchain_extent;
        command_buffer.setScissor(0, scissor);

        command_buffer.draw(3, 1, 0, 0);
        command_buffer.endRenderPass();

        if (command_buffer.end() != vk::Result::eSuccess)
        {
            std::cerr << "failed to record command buffer" << std::endl;
            exit(EXIT_FAILURE);
        }
    }

    void create_sync_objects()
    {
        auto sem_img_res = device.createSemaphore({});
        if (sem_img_res.result != vk::Result::eSuccess)
        {
            std::cerr << "failed to create image available semaphore" << std::endl;
            exit(EXIT_FAILURE);
        }
        sem_image_available = sem_img_res.value;

        auto sem_render_res = device.createSemaphore({});
        if (sem_render_res.result != vk::Result::eSuccess)
        {
            std::cerr << "failed to create render finished semaphore" << std::endl;
            exit(EXIT_FAILURE);
        }
        sem_render_finished = sem_render_res.value;

        auto fence_res = device.createFence({vk::FenceCreateFlagBits::eSignaled});
        if (fence_res.result != vk::Result::eSuccess)
        {
            std::cerr << "failed to create in flight fence" << std::endl;
            exit(EXIT_FAILURE);
        }

        fence_in_flight = fence_res.value;
    }

    void main_loop()
    {
        while (!glfwWindowShouldClose(window))
        {
            glfwPollEvents();
            draw_frame();
        }
    }

    void draw_frame()
    {
        auto res_wait = device.waitForFences(fence_in_flight, VK_TRUE, std::numeric_limits<uint64_t>::max());
        if (res_wait != vk::Result::eSuccess)
        {
            std::cerr << "failed to wait for fence" << std::endl;
            exit(EXIT_FAILURE);
        }
        device.resetFences(fence_in_flight);

        uint32_t image_index;
        auto res_next = device.acquireNextImageKHR(swapchain, std::numeric_limits<uint64_t>::max(), sem_image_available,
                                                   nullptr, &image_index);
        if (res_next != vk::Result::eSuccess)
        {
            std::cerr << "failed to acquire next image" << std::endl;
            exit(EXIT_FAILURE);
        }

        command_buffer.reset(vk::CommandBufferResetFlags());
        record_command_buffer(command_buffer, image_index);

        vk::PipelineStageFlags wait_stage = vk::PipelineStageFlagBits::eColorAttachmentOutput;
        vk::SubmitInfo submit_info{};
        submit_info.setWaitSemaphores(sem_image_available);
        submit_info.setWaitDstStageMask(wait_stage);
        submit_info.setCommandBuffers(command_buffer);
        submit_info.setSignalSemaphores(sem_render_finished);

        if (graphics_queue.submit(submit_info, fence_in_flight) != vk::Result::eSuccess)
        {
            std::cerr << "failed to submit draw command buffer" << std::endl;
            exit(EXIT_FAILURE);
        }

        vk::PresentInfoKHR present_info{};
        present_info.setWaitSemaphores(sem_render_finished);
        present_info.setSwapchains(swapchain);
        present_info.setImageIndices(image_index);

        if (present_queue.presentKHR(present_info) != vk::Result::eSuccess)
        {
            std::cerr << "failed to present image" << std::endl;
            exit(EXIT_FAILURE);
        }
    }

    void cleanup()
    {
        auto res = device.waitIdle();
        if (res != vk::Result::eSuccess)
        {
            std::cerr << "failed to wait for device idle" << std::endl;
            exit(EXIT_FAILURE);
        }

        device.destroySemaphore(sem_render_finished);
        device.destroySemaphore(sem_image_available);
        device.destroyFence(fence_in_flight);

        device.destroyCommandPool(command_pool);

        for (auto fb : swapchain_framebuffers)
        {
            device.destroyFramebuffer(fb);
        }

        device.destroyPipeline(graphics_pipeline);
        device.destroyPipelineLayout(pipeline_layout);
        device.destroyRenderPass(render_pass);

        for (auto image_view : swapchain_image_views)
        {
            device.destroyImageView(image_view);
        }

        device.destroySwapchainKHR(swapchain);
        instance.destroySurfaceKHR(surface);
        device.destroy();
        instance.destroy();

        glfwDestroyWindow(window);
        glfwTerminate();
    }
};

int main()
{
    LearnVulkanApp app;
    app.run();

    return EXIT_SUCCESS;
}