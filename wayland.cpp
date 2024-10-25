#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <ostream>
#include <unistd.h>
#include <string>
#include <sys/types.h>
#include <vector>
#include <vulkan/vulkan_core.h>
#include <wayland-client-core.h>
#include <wayland-client-protocol.h>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_wayland.h>
#include "xdg-shell.h"

#define VK_CHECK(value) \
switch(value){ \
  case VK_SUCCESS                      : break; \
  case VK_NOT_READY                    : fprintf(stderr, "Error in function: VK_NOT_READY\n"); exit(1);break; \
  case VK_TIMEOUT                      : fprintf(stderr, "Error in function: VK_TIMEOUT\n"); exit(1);break; \
  case VK_EVENT_SET                    : fprintf(stderr, "Error in function: VK_EVENT_SET\n"); exit(1);break; \
  case VK_EVENT_RESET                  : fprintf(stderr, "Error in function: VK_EVENT_RESET\n"); exit(1);break; \
  case VK_INCOMPLETE                   : fprintf(stderr, "Error in function: VK_INCOMPLETE\n"); exit(1);break; \
  case VK_ERROR_OUT_OF_HOST_MEMORY     : fprintf(stderr, "Error in function: VK_ERROR_OUT_OF_HOST_MEMORY\n"); exit(1);break; \
  case VK_ERROR_OUT_OF_DEVICE_MEMORY   : fprintf(stderr, "Error in function: VK_ERROR_OUT_OF_DEVICE_MEMORY\n"); exit(1);break; \
  case VK_ERROR_INITIALIZATION_FAILED  : fprintf(stderr, "Error in function: VK_ERROR_INITIALIZATION_FAILED\n"); exit(1);break; \
  case VK_ERROR_DEVICE_LOST            : fprintf(stderr, "Error in function: VK_ERROR_DEVICE_LOST\n"); exit(1);break; \
  case VK_ERROR_MEMORY_MAP_FAILED      : fprintf(stderr, "Error in function: VK_ERROR_MEMORY_MAP_FAILED\n"); exit(1);break; \
  case VK_ERROR_LAYER_NOT_PRESENT      : fprintf(stderr, "Error in function: VK_ERROR_LAYER_NOT_PRESENT\n"); exit(1);break; \
  case VK_ERROR_EXTENSION_NOT_PRESENT  : fprintf(stderr, "Error in function: VK_ERROR_EXTENSION_NOT_PRESENT\n"); exit(1);break; \
  case VK_ERROR_FEATURE_NOT_PRESENT    : fprintf(stderr, "Error in function: VK_ERROR_FEATURE_NOT_PRESENT\n"); exit(1);break; \
  case VK_ERROR_INCOMPATIBLE_DRIVER    : fprintf(stderr, "Error in function: VK_ERROR_INCOMPATIBLE_DRIVER\n"); exit(1);break; \
  case VK_ERROR_TOO_MANY_OBJECTS       : fprintf(stderr, "Error in function: VK_ERROR_TOO_MANY_OBJECTS\n"); exit(1);break; \
  case VK_ERROR_FORMAT_NOT_SUPPORTED   : fprintf(stderr, "Error in function: VK_ERROR_FORMAT_NOT_SUPPORTED\n"); exit(1);break; \
  case VK_ERROR_FRAGMENTED_POOL        : fprintf(stderr, "Error in function: VK_ERROR_FRAGMENTED_POOL\n"); exit(1);break; \
  case VK_ERROR_UNKNOWN                : fprintf(stderr, "Error in function: VK_ERROR_UNKNOWN\n"); exit(1);break; \
  case VK_ERROR_OUT_OF_POOL_MEMORY     : fprintf(stderr, "Error in function: VK_ERROR_OUT_OF_POOL_MEMORY\n"); exit(1);break; \
  case VK_ERROR_INVALID_EXTERNAL_HANDLE: fprintf(stderr, "Error in function: VK_ERROR_INVALID_EXTERNAL_HANDLE\n"); exit(1);break; \
  case VK_ERROR_FRAGMENTATION          : fprintf(stderr, "Error in function: VK_ERROR_FRAGMENTATION\n"); exit(1);break; \
  default                              : fprintf(stderr, "UNKNOWN ERROR"); exit(1); break; \
}


const char* appName = "Wayland vulkan example";

namespace Wayland{
  wl_display    *display;
  wl_registry   *registry;
  wl_compositor *compositor;
  wl_surface    *surface;
  xdg_wm_base   *wm_base;
  xdg_surface   *xdgsurface;
  xdg_toplevel  *top_level;
  uint32_t       width  = 640;
  uint32_t       height = 640;
}

  static void
  registry_handle_global(void *data, struct wl_registry *registry,
	  	uint32_t name, const char *interface, uint32_t version)
  {
	  printf("interface: '%s', version: %d, name: %d\n",
		  	interface, version, name);
    if(strcmp(interface, "wl_compositor") == 0)
      Wayland::compositor = (struct wl_compositor*)wl_registry_bind(registry, name, &wl_compositor_interface, version);
    if(strcmp(interface, xdg_wm_base_interface.name) == 0){
    Wayland::wm_base = (struct xdg_wm_base*)wl_registry_bind(registry, name, &xdg_wm_base_interface, version);
    }
  }

  static void
  registry_handle_global_remove(void *data, struct wl_registry *registry,
	  	uint32_t name)
  {
  }
  static const wl_registry_listener registry_listener = {
	  .global = registry_handle_global,
	  .global_remove = registry_handle_global_remove,
  };

  static void xdg_wm_base_handle_ping(void* data, struct xdg_wm_base *xdg_wm_base, uint32_t serial){
    xdg_wm_base_pong(xdg_wm_base,serial);
  }
  static const xdg_wm_base_listener xdg_wm_base_listener = {
    .ping = xdg_wm_base_handle_ping
  };
  static void xdg_surface_handle_configure(void* data, struct xdg_surface *xdg_surface, uint32_t serial){
    xdg_surface_ack_configure(xdg_surface, serial);
  }
  static const xdg_surface_listener xdg_surface_listener = {
    .configure = xdg_surface_handle_configure
  };  



std::vector<char> readfile(const char* path){
  std::ifstream input(path);
  std::vector<char> buffer;
  input.seekg(0, std::ios::end);
  ssize_t size = input.tellg();
  buffer.resize(size);
  input.seekg(0, std::ios::beg);
  input.read(buffer.data(), size);
  return buffer;
}
namespace Vulkan{
  // Struct that represents a physical device
  struct PhysicalDevice {
    VkPhysicalDevice                     vPhyDev;
    VkPhysicalDeviceProperties           vPhyDevProperties;
    std::vector<VkQueueFamilyProperties> vQueueFamilyProperties;
    std::vector<VkSurfaceFormatKHR>      vSurfaceFormat;
    VkPhysicalDeviceFeatures             vFeatures;
    VkSurfaceCapabilitiesKHR             vSurfaceCapabilities;
    VkPhysicalDeviceMemoryProperties     vPhyDevMemoryProperties;
    std::vector<VkPresentModeKHR>        vPresentMode;
  };
  struct QueueIndices {
    uint32_t presentQueue;
    uint32_t graphicsQueue;
  };
  // Struct that represents a Swapchain image
  struct SwapChainImage {
    VkImage         vImage;
    VkImageView     vImageView;
    VkFramebuffer   vFramebuffer;
  };
  struct Frame {
    VkCommandBuffer vCommandBuffer;
    VkFence         vFence;
    VkSemaphore     vFinishRender;
    VkSemaphore     vImageReady;
  };
  // Struct that represents a Swapchain with its images and format
  struct SwapChain {
    VkSwapchainKHR              vSwapchain;
    std::vector<SwapChainImage> vSwapchainImages;
    VkFormat                    vImageFormat;
    VkExtent2D                  vExtent;
  };
  // Global Vk objects
  VkSurfaceKHR                vSurface  = VK_NULL_HANDLE;
  VkInstance                  vInstance = VK_NULL_HANDLE;
  VkDebugUtilsMessengerEXT    vMessenger= VK_NULL_HANDLE;
  std::vector<PhysicalDevice> vAvailablePhyDev;
  PhysicalDevice             *vSelectedPhyDev;
  VkDevice                    vDevice;
  QueueIndices                vIndices;
  SwapChain                   vSwapchain;
  VkPipelineLayout            vPipelineLayout;
  VkPipeline                  vPipeline;
  VkRenderPass                vRenderPass;
  VkCommandPool               vCommandPool;
  VkQueue                     vQueues[2];
  Frame                       vFrames[2];
  std::vector<const char*>    vLayernames = {
    "VK_LAYER_KHRONOS_validation"
  };
  std::vector<const char*>    vExtensionnames = {
    VK_KHR_SURFACE_EXTENSION_NAME,
    "VK_KHR_wayland_surface",
    "VK_EXT_debug_utils",
  };

// Debug callback
uint32_t DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT severity,
                       VkDebugUtilsMessageTypeFlagsEXT type,
                       const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
                       void* pUserData){
  fprintf(stderr, "Debug Message: %s\n", pCallbackData->pMessage);
  return VK_FALSE;
}
// Lookup table to convert types to actual printable string
const char* deviceTypeString(VkPhysicalDeviceType type){
  switch(type){
    case VK_PHYSICAL_DEVICE_TYPE_OTHER          : return "Other";
    case VK_PHYSICAL_DEVICE_TYPE_CPU            : return "CPU graphics";
    case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU    : return "Virtual Graphics Unit";
    case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU : return "Integrated Graphics Unit";
    case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU   : return "Discrete Graphhics Unit";
    default: return "Nothing";
  }
}
// Enumerate devices and fill the vAvailablePhyDev vector with the filled PhysicalDev struct
void getPhysicalDevices(){
  uint32_t nPhyDev;
  VK_CHECK(vkEnumeratePhysicalDevices(vInstance, &nPhyDev, NULL));
  VkPhysicalDevice phyDevs[nPhyDev];
  vAvailablePhyDev.resize(nPhyDev);
  VK_CHECK(vkEnumeratePhysicalDevices(vInstance, &nPhyDev, phyDevs));
  for(int i = 0; i<nPhyDev; i++){
    vAvailablePhyDev[i].vPhyDev = phyDevs[i];

    vkGetPhysicalDeviceProperties(phyDevs[i], &vAvailablePhyDev[i].vPhyDevProperties);
    fprintf(stdout, "GPU name: %s, Type: %s \n", vAvailablePhyDev[i].vPhyDevProperties.deviceName, deviceTypeString(vAvailablePhyDev[i].vPhyDevProperties.deviceType));
    
    uint32_t nQueueFamilies;
    vkGetPhysicalDeviceQueueFamilyProperties(phyDevs[i], &nQueueFamilies, NULL);
    vAvailablePhyDev[i].vQueueFamilyProperties.resize(nQueueFamilies);
    vkGetPhysicalDeviceQueueFamilyProperties(phyDevs[i], &nQueueFamilies, vAvailablePhyDev[i].vQueueFamilyProperties.data());
    
    VK_CHECK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(phyDevs[i], vSurface, &vAvailablePhyDev[i].vSurfaceCapabilities));
    
    vkGetPhysicalDeviceMemoryProperties(phyDevs[i], &vAvailablePhyDev[i].vPhyDevMemoryProperties);
    
    uint32_t nSurfacePresentModes;
    vkGetPhysicalDeviceSurfacePresentModesKHR(phyDevs[i], vSurface, &nSurfacePresentModes, NULL);
    vAvailablePhyDev[i].vPresentMode.resize(nSurfacePresentModes);
    vkGetPhysicalDeviceSurfacePresentModesKHR(phyDevs[i], vSurface, &nSurfacePresentModes, vAvailablePhyDev[i].vPresentMode.data());

    uint32_t nSurfaceFormat;
    vkGetPhysicalDeviceSurfaceFormatsKHR(phyDevs[i], vSurface, &nSurfaceFormat, NULL);
    vAvailablePhyDev[i].vSurfaceFormat.resize(nSurfaceFormat);
    vkGetPhysicalDeviceSurfaceFormatsKHR(phyDevs[i], vSurface, &nSurfaceFormat, vAvailablePhyDev[i].vSurfaceFormat.data());
    
    vkGetPhysicalDeviceFeatures(phyDevs[i], &vAvailablePhyDev[i].vFeatures);
  }
}
// Gets a physical device struct and flags bitmask and check if the queue has the flags specified in the bitmask ;; returns the queue family index
uint32_t selectQueueByPropriety(PhysicalDevice &phydev, VkQueueFlags flags, VkBool32 supportspresent){
  uint32_t index = 0;
  if(supportspresent){
    for(auto &x : phydev.vQueueFamilyProperties){
      vkGetPhysicalDeviceSurfaceSupportKHR(phydev.vPhyDev, index, vSurface, &supportspresent);
      if((x.queueFlags & flags) == flags && supportspresent){
        return index;
      }
      index ++;
    }
  }else{
    for(auto &x : phydev.vQueueFamilyProperties){
      if((x.queueFlags & flags) == flags){
        return index;
      }
      index ++;
    }
  }
  return -1;
}
// Checks if extension is supported
VkBool32 checkExtensions(std::string extocheck){
  uint32_t numextension;
  vkEnumerateDeviceExtensionProperties(vSelectedPhyDev->vPhyDev, nullptr , &numextension, NULL);
  std::vector<VkExtensionProperties> extensionprop(numextension);
  vkEnumerateDeviceExtensionProperties(vSelectedPhyDev->vPhyDev, nullptr, &numextension, extensionprop.data());

  for(auto &x : extensionprop){
    if(x.extensionName == extocheck){
      return true;
    }
  }

  return false;
}
// Creates a Vulkan logical device
void createLogicalDevice(){
  vIndices.graphicsQueue = selectQueueByPropriety(*vSelectedPhyDev, VK_QUEUE_GRAPHICS_BIT, false);
  fprintf(stdout, "qindex = %d \n", vIndices.graphicsQueue);
  vIndices.presentQueue  = selectQueueByPropriety(*vSelectedPhyDev, 0, true);
  fprintf(stdout, "pindex = %d \n", vIndices.presentQueue);
  if(vIndices.graphicsQueue == -1){
    fprintf(stdout, "No suitable index found for bitmask\n");
    exit(1);
  }
  float queuepriority[2] = {1.0f, 1.0f};
  if(vIndices.graphicsQueue == vIndices.presentQueue){
    VkDeviceQueueCreateInfo queuecreateinfo[1]={{
      .sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
      .pNext            = NULL,
      .flags            = 0,
      .queueFamilyIndex = vIndices.graphicsQueue,
      .queueCount       = 2,
      .pQueuePriorities = queuepriority,
    }};
    std::vector<const char*> extensions;
    if(checkExtensions(VK_KHR_SWAPCHAIN_EXTENSION_NAME)){
      extensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
    }else{
      fprintf(stdout, "GPU doesn't support swap chain\n");
    }
    VkPhysicalDeviceFeatures features = {
      .shaderFloat64 = vSelectedPhyDev->vFeatures.shaderFloat64,
      .shaderInt64   = vSelectedPhyDev->vFeatures.shaderInt64
    };
    VkDeviceCreateInfo devCreateInfo = {
    .sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
    .pNext                   = NULL,
    .flags                   = 0,
    .queueCreateInfoCount    = 1,
    .pQueueCreateInfos       = queuecreateinfo,
    .enabledExtensionCount   = static_cast<uint32_t>(extensions.size()),
    .ppEnabledExtensionNames = extensions.data(),
    .pEnabledFeatures        = &features,
    };
    VK_CHECK(vkCreateDevice(vSelectedPhyDev->vPhyDev, &devCreateInfo, NULL, &vDevice));
    vkGetDeviceQueue(vDevice, vIndices.graphicsQueue, 0, &vQueues[0]);
    vkGetDeviceQueue(vDevice, vIndices.presentQueue, 1, &vQueues[1]);
  }
}
// Creates the debug callback
void createDebugcb(){
  VkDebugUtilsMessengerCreateInfoEXT vDebugMessengerInfo = {
    .sType           = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
    .pNext           = NULL,
    .flags           = 0,
    .messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
                       VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT |
                       VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT,
    .messageType     = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                       VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT,
    .pfnUserCallback = &DebugCallback,
    .pUserData       = NULL,
  };
  PFN_vkCreateDebugUtilsMessengerEXT vkCreateDebugUtilsMessengerEXT = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(vInstance, "vkCreateDebugUtilsMessengerEXT");
  if(!vkCreateDebugUtilsMessengerEXT){
    fprintf(stderr, "Cannot find vkCreateDebugUtilsMessengerEXT in the instance\n");
    exit(1);
  }
  VK_CHECK(vkCreateDebugUtilsMessengerEXT(vInstance, &vDebugMessengerInfo, NULL, &vMessenger));
  fprintf(stdout, "Messenger created\n");
}
// Check if the format is available on the gpu
VkSurfaceFormatKHR getSurfaceFormat(VkFormat format){
  for(auto &x : vSelectedPhyDev->vSurfaceFormat){
    if(x.format == format){
      fprintf(stdout, "Found requested surface format\n");
      return x;
    }
  }
  fprintf(stdout, "Requested surface format not found, defaulting to the first format");
  return vSelectedPhyDev->vSurfaceFormat[0];
}
// Check if the present mode is supported by the gpu
VkPresentModeKHR getPresentMode(VkPresentModeKHR presentMode){
  for(auto &x : vSelectedPhyDev->vPresentMode){
    if(x == presentMode){
      return x;
    }
  }
  return VK_PRESENT_MODE_FIFO_KHR;
}
// Creates a swapchain and its imageviews
void createSwapChain(){
  uint32_t imageCount;
  uint32_t imageCount0;
  VkSurfaceFormatKHR format = getSurfaceFormat(VK_FORMAT_B8G8R8A8_UNORM);
  VkExtent2D         extent = {Wayland::width, Wayland::height};
  
  
  imageCount = vSelectedPhyDev->vSurfaceCapabilities.minImageCount + 1;
  VkSwapchainCreateInfoKHR swapchainCreateinfo = {
    .sType            = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
    .surface          = vSurface,
    .minImageCount    = imageCount,
    .imageFormat      = format.format,
    .imageColorSpace  = format.colorSpace,
    .imageExtent      = extent,
    .imageArrayLayers = 1,
    .imageUsage       = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
    .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
    .preTransform     = vSelectedPhyDev->vSurfaceCapabilities.currentTransform,
    .compositeAlpha   = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
    .presentMode      = getPresentMode(VK_PRESENT_MODE_MAILBOX_KHR),
    .clipped          = VK_TRUE,
    .oldSwapchain     = VK_NULL_HANDLE
  };
  VK_CHECK(vkCreateSwapchainKHR(vDevice, &swapchainCreateinfo, nullptr, &vSwapchain.vSwapchain));
  
  vkGetSwapchainImagesKHR(vDevice, vSwapchain.vSwapchain, &imageCount0, NULL);
  vSwapchain.vSwapchainImages.resize(imageCount0);
  VkImage images[imageCount0];
  vkGetSwapchainImagesKHR(vDevice, vSwapchain.vSwapchain, &imageCount0, images);
  for(int i = 0; i< imageCount0; i++){
    vSwapchain.vSwapchainImages[i].vImage = images[i];
  }
  vSwapchain.vImageFormat = format.format;
  vSwapchain.vExtent      = extent;
  for(auto &x : vSwapchain.vSwapchainImages){
    VkImageViewCreateInfo imageviewcreateinfo = {
      .sType    = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
      .image    = x.vImage,
      .viewType = VK_IMAGE_VIEW_TYPE_2D,
      .format   = vSwapchain.vImageFormat,
      .components = {VK_COMPONENT_SWIZZLE_IDENTITY,VK_COMPONENT_SWIZZLE_IDENTITY,VK_COMPONENT_SWIZZLE_IDENTITY,VK_COMPONENT_SWIZZLE_IDENTITY},
      .subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT,0,1,0,1}
    };
    vkCreateImageView(vDevice, &imageviewcreateinfo, nullptr, &x.vImageView);
  }
}
// Returns a shader module from a path
VkShaderModule createShader(const char* path){
  std::vector<char> shaderbyte = readfile(path);

  VkShaderModuleCreateInfo shadercreateinfo = {
    .sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
    .codeSize = shaderbyte.size(),
    .pCode    = reinterpret_cast<uint32_t*>(shaderbyte.data())
  };
  VkShaderModule shadermodule;
  VK_CHECK(vkCreateShaderModule(vDevice, &shadercreateinfo,nullptr,&shadermodule));
  return shadermodule;
}
// Creates the global render pass
void createRenderPass(){
  VkAttachmentDescription attachmentdescription = {
    .format         = vSwapchain.vImageFormat,
    .samples        = VK_SAMPLE_COUNT_1_BIT,
    .loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR,
    .storeOp        = VK_ATTACHMENT_STORE_OP_STORE,
    .stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
    .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
    .initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED,
    .finalLayout    = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
  };
  VkAttachmentReference attachmentreference = {
    .attachment = 0,
    .layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
  };
  VkSubpassDescription subpass = {
    .pipelineBindPoint    = VK_PIPELINE_BIND_POINT_GRAPHICS,
    .colorAttachmentCount = 1,
    .pColorAttachments    = &attachmentreference
  };
  VkSubpassDependency subpassdependency = {
    .srcSubpass    = VK_SUBPASS_EXTERNAL,
    .dstSubpass    = 0,

    .srcStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
    .dstStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,

    .srcAccessMask = 0,
    .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT
  };
  VkRenderPassCreateInfo renderpassinfo  =
  {
    .sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
    .attachmentCount = 1,
    .pAttachments    = &attachmentdescription,
    .subpassCount    = 1,
    .pSubpasses      = &subpass,
    .dependencyCount = 1,
    .pDependencies   = &subpassdependency
  };
  VK_CHECK(vkCreateRenderPass(vDevice, &renderpassinfo, NULL, &vRenderPass));
}
// Creates the pipeline
void createPipeline(){
  VkShaderModule vertex   = createShader("vshader.spv");
  VkShaderModule fragment = createShader("fshader.spv");

  VkPipelineShaderStageCreateInfo vertexstageinfo = {
    .sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
    .stage  = VK_SHADER_STAGE_VERTEX_BIT,
    .module = vertex,
    .pName  = "main",
  };
  VkPipelineShaderStageCreateInfo fragmentstageinfo = {
    .sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
    .stage  = VK_SHADER_STAGE_FRAGMENT_BIT,
    .module = fragment,
    .pName  = "main",
  };

  VkPipelineShaderStageCreateInfo shaderstage[] = {vertexstageinfo,fragmentstageinfo};

  // Dynamic states (set for resizing i guess)
  VkDynamicState dystates[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
  VkPipelineDynamicStateCreateInfo dynamicstate = {
    .sType             = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
    .dynamicStateCount = 2,
    .pDynamicStates    = dystates
  };
  // Vertex data interpretation (Analog to Vao in opengl)
  VkPipelineVertexInputStateCreateInfo vertexinfostate = {
    .sType                           = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
    .vertexBindingDescriptionCount   = 0,
    .pVertexBindingDescriptions      = nullptr,
    .vertexAttributeDescriptionCount = 0,
    .pVertexAttributeDescriptions    = nullptr
  };
  // Primitives to draw
  VkPipelineInputAssemblyStateCreateInfo inputasmstate = {
    .sType                  = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
    .topology               = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
    .primitiveRestartEnable = VK_FALSE
  };
  VkViewport viewport  ={
    .x        = 0,
    .y        = 0,
    .width    = static_cast<float>(vSwapchain.vExtent.width),
    .height   = static_cast<float>(vSwapchain.vExtent.height),
    .minDepth = 0.0f,
    .maxDepth = 0.0f
  };
  VkRect2D scissor = {
    .offset = {0,0},
    .extent = vSwapchain.vExtent,
  };

  VkPipelineViewportStateCreateInfo viewportstate = {
    .sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
    .viewportCount = 1,
    .pViewports    = &viewport,
    .scissorCount  = 1,
    .pScissors     = &scissor
  };

  VkPipelineRasterizationStateCreateInfo rasterizationstate = {
    .sType                   = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
    .depthClampEnable        = VK_FALSE,
    .rasterizerDiscardEnable = VK_FALSE,
    .polygonMode             = VK_POLYGON_MODE_FILL,
    .cullMode                = VK_CULL_MODE_FRONT_BIT,
    .frontFace               = VK_FRONT_FACE_COUNTER_CLOCKWISE,
    .depthBiasEnable         = VK_FALSE,
    .depthBiasConstantFactor = 0.0f,
    .depthBiasClamp          = 0.0f,
    .depthBiasSlopeFactor    = 0.0f,
    .lineWidth               = 1.0f
  };
  VkPipelineLayoutCreateInfo pipelinelayoutcreate = {
    .sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
    .setLayoutCount         = 0,
    .pSetLayouts            = nullptr,
    .pushConstantRangeCount = 0,
    .pPushConstantRanges    = nullptr
  };
  VK_CHECK(vkCreatePipelineLayout(vDevice, &pipelinelayoutcreate, nullptr, &vPipelineLayout));
  VkGraphicsPipelineCreateInfo graphicspipelineinfo = {
    .sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
    .stageCount          = 2,
    .pStages             = shaderstage,
    .pVertexInputState   = &vertexinfostate,
    .pInputAssemblyState = &inputasmstate,
    .pTessellationState  = NULL,
    .pViewportState      = &viewportstate,
    .pRasterizationState = &rasterizationstate,
    .pMultisampleState   = NULL,
    .pDepthStencilState  = NULL,
    .pColorBlendState    = NULL,
    .pDynamicState       = &dynamicstate,
    .layout              = vPipelineLayout,
    .renderPass          = vRenderPass,
    .subpass             = 0,
    .basePipelineHandle  = VK_NULL_HANDLE,
    .basePipelineIndex   = -1
  };

  vkCreateGraphicsPipelines(vDevice, VK_NULL_HANDLE, 1, &graphicspipelineinfo, NULL, &vPipeline);


  vkDestroyShaderModule(vDevice, vertex,nullptr);
  vkDestroyShaderModule(vDevice, fragment,nullptr);
}
// Create the framebuffers 
void createFramebuffers(){
  for(auto &x : vSwapchain.vSwapchainImages){
    VkFramebufferCreateInfo framebufferinfo = {
      .sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
      .renderPass      = vRenderPass,
      .attachmentCount = 1,
      .pAttachments    = &x.vImageView,
      .width           = Wayland::width,
      .height          = Wayland::height,
      .layers          = 1
    };
    VK_CHECK(vkCreateFramebuffer(vDevice, &framebufferinfo, NULL, &x.vFramebuffer));
  }
}
// Create the command pool
void createCommandPool(){
  VkCommandPoolCreateInfo commandpoolinfo ={
    .sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
    .flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
    .queueFamilyIndex = vIndices.graphicsQueue
  };
  VK_CHECK(vkCreateCommandPool(vDevice, &commandpoolinfo, nullptr, &vCommandPool));
  for(auto &x : vFrames){
    VkCommandBufferAllocateInfo commandbufferallocateinfo = {
      .sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
      .commandPool        = vCommandPool,
      .level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
      .commandBufferCount = 1
    };
    VK_CHECK(vkAllocateCommandBuffers(vDevice, &commandbufferallocateinfo, &x.vCommandBuffer));
  }
}
void createSyncobjects(){
  for (auto &x : vFrames) {
    VkFenceCreateInfo fenceinfo = {
      .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
      .pNext = nullptr,
      .flags = VK_FENCE_CREATE_SIGNALED_BIT
    };
    vkCreateFence(vDevice, &fenceinfo, nullptr, &x.vFence);


    VkSemaphoreCreateInfo semaphoreinfo ={
      .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
      .pNext = nullptr,
      .flags = 0
    };
    vkCreateSemaphore(vDevice, &semaphoreinfo, nullptr, &x.vImageReady);
    vkCreateSemaphore(vDevice, &semaphoreinfo, nullptr, &x.vFinishRender);
  }
}
// Main draw loop
void drawCommands(Frame* frame, uint32_t imageIndex){
  VkCommandBuffer usedCommandBuffer = frame->vCommandBuffer;
  VkCommandBufferBeginInfo commandbufferbegininfo = {
    .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
    .flags = 0,
    .pInheritanceInfo = nullptr
  };
  VK_CHECK(vkBeginCommandBuffer(usedCommandBuffer, &commandbufferbegininfo));
  
  VkRect2D renderarea;
  renderarea.extent = {vSwapchain.vExtent};
  renderarea.offset = {0,0};
  VkClearValue clearvalue;
  clearvalue.color = {{0.0,0.0,0.0,1.0}};
  VkRenderPassBeginInfo renderpassbegininfo = {
    .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
    .renderPass = vRenderPass,
    .framebuffer = vSwapchain.vSwapchainImages[imageIndex].vFramebuffer,
    .renderArea = renderarea,
    .clearValueCount = 1,
    .pClearValues = &clearvalue
  };
  vkCmdBeginRenderPass(usedCommandBuffer, &renderpassbegininfo, VK_SUBPASS_CONTENTS_INLINE);

  vkCmdBindPipeline(usedCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vPipeline);
  VkViewport viewport = {
    .x        = 0,
    .y        = 0,
    .width    = static_cast<float>(Wayland::width),
    .height   = static_cast<float>(Wayland::height),
    .minDepth = 0.0f,
    .maxDepth = 1.0f
  };
  vkCmdSetViewport(usedCommandBuffer,0,1,&viewport);
  VkOffset2D offset = {0,0,};
  VkRect2D scissor = {
    .offset = offset,
    .extent = vSwapchain.vExtent   
  };
  vkCmdSetScissor(usedCommandBuffer, 0, 1, &scissor);
  vkCmdDraw(usedCommandBuffer, 3, 1, 0, 0);
  vkCmdEndRenderPass(usedCommandBuffer);
  VK_CHECK(vkEndCommandBuffer(usedCommandBuffer));
}



void draw(uint32_t frameIndex){
  Frame* currentFrame = &vFrames[frameIndex];
  vkWaitForFences(vDevice, 1, &currentFrame->vFence, VK_TRUE, UINT64_MAX);
  vkResetFences(vDevice, 1, &currentFrame->vFence);
  uint32_t imageIndex;
  VK_CHECK(vkAcquireNextImageKHR(vDevice,vSwapchain.vSwapchain,UINT64_MAX,currentFrame->vImageReady,VK_NULL_HANDLE,&imageIndex));
  vkResetCommandBuffer(currentFrame->vCommandBuffer, 0);

  drawCommands(currentFrame,imageIndex);
  VkPipelineStageFlags waitstage = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
  VkSubmitInfo submitinfo ={
    .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
    .waitSemaphoreCount = 1,
    .pWaitSemaphores = &currentFrame->vImageReady,
    .pWaitDstStageMask = &waitstage,
    .commandBufferCount = 1,
    .pCommandBuffers = &currentFrame->vCommandBuffer,
    .signalSemaphoreCount = 1,
    .pSignalSemaphores = &currentFrame->vFinishRender
  };
  vkQueueSubmit(vQueues[0], 1, &submitinfo, currentFrame->vFence);
  VkPresentInfoKHR presentinfo = {
    .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
    .waitSemaphoreCount = 1,
    .pWaitSemaphores = &currentFrame->vFinishRender,
    .swapchainCount = 1,
    .pSwapchains = &vSwapchain.vSwapchain,
    .pImageIndices = &imageIndex,
    .pResults = nullptr
  };
  vkQueuePresentKHR(vQueues[1], &presentinfo);
}
// Sets up vulkan
void initvulkan(){
  VkApplicationInfo appinfo = {};
  appinfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
  appinfo.pApplicationName = appName;
  appinfo.apiVersion = VK_API_VERSION_1_3;
  appinfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
  appinfo.pEngineName = "No Engine";
  appinfo.engineVersion = VK_MAKE_VERSION(1,0,0);

  VkInstanceCreateInfo vInstanceInfo = {};
  vInstanceInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
  vInstanceInfo.pApplicationInfo = &appinfo;
  vInstanceInfo.pNext = NULL;
  vInstanceInfo.flags = 0;
  vInstanceInfo.enabledLayerCount = 1;
  vInstanceInfo.ppEnabledLayerNames = vLayernames.data();
  vInstanceInfo.enabledExtensionCount = 3;
  vInstanceInfo.ppEnabledExtensionNames = vExtensionnames.data();
  
  VK_CHECK(vkCreateInstance(&vInstanceInfo, nullptr, &vInstance));
  createDebugcb();
  fprintf(stdout, "Vulkan instance created\n");
  VkWaylandSurfaceCreateInfoKHR vSurfaceInfo ={
    .sType = VK_STRUCTURE_TYPE_WAYLAND_SURFACE_CREATE_INFO_KHR,
    .pNext = NULL,
    .flags = 0,
    .display = Wayland::display,
    .surface = Wayland::surface,
  };
  VK_CHECK(vkCreateWaylandSurfaceKHR(vInstance, &vSurfaceInfo, NULL, &vSurface));
  getPhysicalDevices(); 
  vSelectedPhyDev = &vAvailablePhyDev[0];
  createLogicalDevice();
  createSwapChain();
  createRenderPass();
  createPipeline();
  createFramebuffers();
  createCommandPool();
  createSyncobjects();
}

void destroySyncobjects(){
  for(auto &x : vFrames){
    vkWaitForFences(vDevice, 1, &x.vFence, VK_TRUE, UINT64_MAX);
    vkDestroyFence(vDevice, x.vFence, nullptr);
    vkDestroySemaphore(vDevice, x.vFinishRender, nullptr);
    vkDestroySemaphore(vDevice, x.vImageReady, nullptr);
}}
void destroyFramebuffers(){
  for(auto &x : vSwapchain.vSwapchainImages){
    vkDestroyFramebuffer(vDevice, x.vFramebuffer, nullptr);
  }
}
void destroySwapchain(){
  for(auto &x : vSwapchain.vSwapchainImages){
    vkDestroyImageView(vDevice, x.vImageView, nullptr);
  }
  vkDestroySwapchainKHR(vDevice, vSwapchain.vSwapchain, nullptr);
}
void destroyPipeline(){
  vkDestroyPipeline(vDevice, vPipeline, NULL);
  vkDestroyPipelineLayout(vDevice, vPipelineLayout, nullptr);
  vkDestroyRenderPass(vDevice, vRenderPass, NULL);
}
void destroyCommands(){
  vkDestroyCommandPool(vDevice, vCommandPool, nullptr);
}
// Cleans up vulkan
void destroyvulkan(){
  PFN_vkDestroyDebugUtilsMessengerEXT vkDestroyDebugUtilsMessengerEXT = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(vInstance, "vkDestroyDebugUtilsMessengerEXT");
  if(!vkDestroyDebugUtilsMessengerEXT){
    fprintf(stderr, "Cannot find vkDestroyDebugUtilsMessengerEXT in the instance\n");
    exit(1);
  }
  destroySyncobjects();
  destroyFramebuffers();
  destroySwapchain();
  destroyPipeline();
  destroyCommands();
  vkDestroyDevice(vDevice, nullptr);
  vkDestroyDebugUtilsMessengerEXT(vInstance, vMessenger, nullptr);
  vkDestroySurfaceKHR(vInstance, vSurface, NULL);
  vkDestroyInstance(vInstance, nullptr);
  fprintf(stdout, "Cleanup completed \n");
}
}





int
main(int argc, char *argv[])
{
  Wayland::display = wl_display_connect(NULL);
  if (!Wayland::display) {
    fprintf(stderr, "Failed to connect to Wayland display.\n");
    return 1;
  }
  fprintf(stderr, "Connection established!\n");
  Wayland::registry = wl_display_get_registry(Wayland::display);
  wl_registry_add_listener(Wayland::registry, &registry_listener, NULL);
  wl_display_roundtrip(Wayland::display);
  xdg_wm_base_add_listener(Wayland::wm_base, &xdg_wm_base_listener, NULL);
  Wayland::surface    = wl_compositor_create_surface(Wayland::compositor);
  Wayland::xdgsurface = xdg_wm_base_get_xdg_surface(Wayland::wm_base, Wayland::surface);
  Wayland::top_level  = xdg_surface_get_toplevel(Wayland::xdgsurface); 
  xdg_toplevel_set_title(Wayland::top_level, appName);
  xdg_toplevel_set_app_id(Wayland::top_level, appName);

  xdg_surface_add_listener(Wayland::xdgsurface, &xdg_surface_listener, NULL);
  
  wl_surface_commit(Wayland::surface);
  wl_display_roundtrip(Wayland::display);

  Vulkan::initvulkan();
  uint32_t frameindex = 0;
  for(int i = 0; i < 50; i++){
    Vulkan::draw(frameindex);
    frameindex = (frameindex + 1) % 2;
  }
  sleep(5);
  Vulkan::destroyvulkan();
  wl_display_disconnect(Wayland::display);
  return 0;
}
