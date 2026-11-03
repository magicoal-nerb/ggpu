#ifndef GGPU_H
#define GGPU_H

// GGPU, a WIP internal rendering wrapper of Vulkan
// Thanks for checking it out, too :D
// magicoal_nerb

#define GGPU_MAX_BATCH_BUFFERS 64
#define GGPU_MAX_ALLOCATIONS 65536
#define GGPU_FRAMES_IN_FLIGHT 3
#define GGPU_CMD_BUFS_PER_POOL 64
#define GGPU_MAX_SWAPCHAIN_IMAGES 8
#define GGPU_IMAGE_USER_START 16
#define GGPU_MAX_DELETIONS 64
#define GGPU_IMAGE_SWAPCHAIN_START 1

#define GGPU_MAX_TEXTURES 16384
#define GGPU_MAX_COLOR_TARGETS 8

#define _GGPU_PRIVATE static

#ifdef _WIN32
    #define VK_USE_PLATFORM_WIN32_KHR
    #define WIN32_LEAN_AND_MEAN
    #define NOMINMAX

    // Sorry, we kind of have to.
    #include <windows.h>
#endif

#include <vulkan/vulkan.h>

#ifdef GGPU_VALIDATION
#define gpuAssert(expr, message, ...) do { \
    if (!((expr))) { \
        fprintf(stderr, message __VA_OPT__(,) __VA_ARGS__); \
        exit(EXIT_FAILURE); \
    } \
} while (0)
#else
#define gpuAssert(expr, message, ...) do { \
    if (!((expr))) \
        fprintf(stderr, message __VA_OPT__(,) __VA_ARGS__); \
} while (0)
#endif

#include <stdbool.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>

// Determines what graphics queue the buffer or allocated object is located in
typedef enum GpuQueueType { kGraphicsQueue, kComputeQueue, kTransferQueue } GpuQueueType;

// Determines where you memory to live in.
// `kSharedMemory` is mapped CPU data that replicates to the GPU. Use this for staging or small data.
// `kGpuMemory` is mapped GPU data that the CPU cannot see. Use this for bigger buffers.
// `kCpuMemory` is mapped CPU data that is useful in creating a staging buffer to transfer data in objects allocated with `kGpuMemory`
// `kHeapMemory` is reserved specifically for the resource heap. This should not be used by the user.
typedef enum GpuMemoryType { kSharedMemory, kGpuMemory, kCpuMemory, kHeapMemory } GpuMemoryType;

// Determines which shader is being used.
typedef enum GpuShaderType { kVertexShader, kFragmentShader, kComputeShader } GpuShaderType;

// Determines which pipeline is being used.
typedef enum GpuPipelineType { kComputePipeline, kGraphicsPipeline } GpuPipelineType;

// Determines what should happen once a texture is used
// `kLoad` preserves the current texture
// `kClear` clears the current texture
typedef enum GpuLoadOp { kLoad, kClear } GpuLoadOp;

// Determines what should happen once a texture is stored
// `kDontCare` lets this be decided by the graphics drive
// `kStore` commits the data onto a texture
typedef enum GpuStoreOp { kDontCare, kStore } GpuStoreOp;

// Determines what the hardware fragment stage should use for the factor
typedef enum GpuFactor { kZeroFactor, kOneFactor, kSrcColorFactor, kDstColorFactor, kSrcAlphaFactor, kDstAlphaFactor } GpuFactor;

// Determines what math operation the hardware rasterizer should use
typedef enum GpuBlend { kAddBlend, kSubtractBlend, kRevSubtractBlend, kMinBlend, kMaxBlend } GpuBlend;

// Determines the winding order of indices
typedef enum GpuFrontFace { kCounterClockwiseFrontFace, kClockwiseFrontFace } GpuFrontFace;

// Determines how triangles are created.
// `kTriangleTopology` creates triangles as [v0, v1, v2] [v3, v4, v5] ...
// `kFanTopology` creates triangles as [v0, v1, v2], [v0, v2, v3] ...
typedef enum GpuGraphicsToplogy { kTriangleTopology, kFanTopology } GpuGraphicsToplogy;

// Determines which face orientation is hidden from the viewer
typedef enum GpuCullMode { kFrontCulling, kBackCulling, kNoCulling } GpuCullMode;

// Bitflag that determines how the image is being used
typedef enum GpuUsageFlags { kSampledUsage, kStorageUsage, kColorAttachmentUsage, kDepthStencilAttachment } GpuUsageFlags;

// Determines the filter used for the sampler
typedef enum GpuFilter { kNearestFilter, kLinearFilter } GpuFilter;

// Determines the mipmapping mode the sampler uses
typedef enum GpuMipmapMode { kNearestMipmap, kLinearMipmap } GpuMipmapMode;

// Determines how the sampler should address sampling outside of 0..1 UV coordinates
typedef enum GpuAddressMode { kRepeatMode, kMirroredRepeatMode, kClampToEdgeMode, kClampToBorderMode, kMirrorClampToEdgeMode } GpuAddressMode;

// Raw GPU memory address
typedef VkDeviceAddress GpuAddress;

// Defines which part of the rendering stage
// you want to target.
typedef enum GpuStage {
    kTransferStage = 1,
    kComputeStage = 2,
    kFragmentStage = 4,
    kVertexStage = 8,
    kColorOutputStage = 16,
    kDepthOutputStage = 32,
    kAllStage = 0xFFFFFFFF,
} GpuStage;

// Determines what to specifically wait for during a barrier
typedef enum GpuHazard {
    kIndirectHazard = 1,
    kDescriptorHazards = 2,
    kDepthHazard = 3,
} GpuHazard;

// Holds information about the name and shader bytecode. When used in a pipeline,
// `module` will be NULL.
typedef struct GpuShader {
    char* shaderName;
    GpuShaderType shaderType;
    VkShaderModule module;
} GpuShader;

// Describes a sampler object on the GPU
typedef struct GpuSamplerDesc {
    GpuFilter magFilter;
    GpuFilter minFilter;
    GpuMipmapMode mipmapMode;
    GpuAddressMode addressModeU;
    GpuAddressMode addressModeV;
    GpuAddressMode addressModeW;
    VkBorderColor borderColor;
    float maxAnisotropy;
    float maxLod;
} GpuSamplerDesc;

// Controls how images blend with eachother (can be used for transparency, for example.)
typedef struct GpuBlendDesc {
    GpuBlend colorOp;
    GpuFactor srcColorFactor;
    GpuFactor dstColorFactor;
    GpuBlend alphaOp;
    GpuFactor srcAlphaFactor;
    GpuFactor dstAlphaFactor;
    uint8_t colorWriteMask;
} GpuBlendDesc;

// Description of the GPU Texture we want to allocate
typedef struct GpuTextureDesc {
    uint32_t width;
    uint32_t height;
    uint32_t mipLevels;
    VkFormat colorFormat;
    VkImageUsageFlags usageFlags;
} GpuTextureDesc;

// Opaque handle of a Vulkan pipelineo bject
typedef struct GpuPipeline {
    GpuPipelineType pipelineType;
    VkPipeline pipeline;
    VkPipelineLayout layout;
    uint32_t layoutSize;
} GpuPipeline;

// Describes the graphics pipeline state
typedef struct GpuGraphicsDesc {
    GpuGraphicsToplogy topology;
    GpuCullMode cull;
    bool alphaCoverage;
    uint32_t sampleCount;
    VkFormat stencilFormat;
    GpuFrontFace frontFace;

    GpuBlendDesc blendStates[GGPU_MAX_COLOR_TARGETS];
    VkFormat colorTargets[GGPU_MAX_COLOR_TARGETS];

    VkFormat depthFormat;
    VkCompareOp depthCompareOp;

    bool depthBiasEnabled;
    float depthBiasConstant;
    float depthBiasSlopeFactor;
} GpuGraphicsDesc;

// Points to GPU mapped memory, opaque handle of allocation data on the GPU
// and potentially memory mapped data with `cpuAddress` (NULL if it does not exist.)
typedef struct GpuPtr {
    GpuAddress gpuAddress;
    void* cpuAddress;
    uint64_t allocationPoolId;
} GpuPtr;

// Points to CPU mapped memory, typically used as a staging buffer
typedef struct CpuPtr {
    void* cpuAddress;
    GpuPtr gpuAllocation;
} CpuPtr;

// Opaque handle of a Vulkan command buffer
typedef struct GpuCommandBuffer {
    VkCommandBuffer buffer;
    GpuQueueType queueType;
} GpuCommandBuffer;

// Internal usage of an image
typedef struct GpuImageInternal {
    VkImage image;
    VkImageView view;
    VkDeviceMemory memory;
    VkFormat format;
    VkExtent2D extent;
    VkImageAspectFlags aspect;
    uint32_t heapIndex;
    uint32_t mips;
} GpuImageInternal;

// Handle of an image with an index
typedef struct GpuImage {
    uint32_t index;
    bool active;
} GpuImage;

// Used for tracking images before uploading them to the GPU
typedef struct GpuUploadBatch {
    GpuCommandBuffer cmd;
    GpuPtr stagingBuffers[GGPU_MAX_BATCH_BUFFERS];
    GpuImage images[GGPU_MAX_BATCH_BUFFERS];
    uint32_t stagingCount;
} GpuUploadBatch;

// Internal pool item
typedef struct GpuBuffer {
    VkBuffer buffer;
    VkDeviceMemory memory;
    GpuAddress gpu;
    void* cpu;
    bool active;
    uint32_t bytes;
    GpuMemoryType memoryType;
} GpuBuffer;

// Current command pool
typedef struct GpuCommandPool {
    VkCommandBuffer commandBuffers[GGPU_FRAMES_IN_FLIGHT * GGPU_CMD_BUFS_PER_POOL];
    VkCommandPool commandPools[GGPU_FRAMES_IN_FLIGHT];
    uint32_t commandQueue[GGPU_FRAMES_IN_FLIGHT];
    VkQueue queue;
    uint32_t queueFamilyIndex;
} GpuCommandPool;

// Internal implementation of a staging pool
typedef struct GpuStagingPool {
    VkFence stagingFence;

    // Allocation pools...
    GpuBuffer allocationsPool[GGPU_MAX_ALLOCATIONS];
    uint32_t freeSlots[GGPU_MAX_ALLOCATIONS];
    uint32_t freeSlotsCount;
    uint32_t allocPoolSize;
} GpuStagingPool;

// Internal implementation of a swapchain
typedef struct GpuSwapchain {
    VkSwapchainKHR rawSwapchain;
    VkSurfaceKHR surface;
    VkFormat format;
    VkExtent2D extent;
    uint32_t imageCount;
    uint32_t currentImageIndex;

    VkImageLayout imageLayouts[GGPU_MAX_SWAPCHAIN_IMAGES];
    VkSemaphore inFlight[GGPU_FRAMES_IN_FLIGHT];
    VkImageView views[GGPU_MAX_SWAPCHAIN_IMAGES];
    VkFence fences[GGPU_FRAMES_IN_FLIGHT];

    VkSemaphore renderFinished[GGPU_MAX_SWAPCHAIN_IMAGES];
    VkImage images[GGPU_MAX_SWAPCHAIN_IMAGES];
    bool resized;
} GpuSwapchain;

// Opaque handle of a Vulkan semaphore
typedef struct GpuSemaphore {
    VkSemaphore semaphore;
    uint64_t value;
} GpuSemaphore;

// Internal structure for storing images
typedef struct GpuImagePool {
    GpuImageInternal images[GGPU_MAX_TEXTURES];
    uint32_t freeSlots[GGPU_MAX_TEXTURES];
    uint32_t freeSlotsCount;
    uint32_t poolSize;
} GpuImagePool;

// Internal structure for the gpu context object
typedef struct GpuContext {
    // Properties
    VkPhysicalDeviceDescriptorHeapPropertiesEXT heapProperties;
    void (*swapchainCallback)(int width, int height);

    // Devices
    VkPhysicalDevice physicalDevice;
    VkDevice device;

    uint32_t maxTextures;
    uint32_t maxSamplers;

    // Queues
    GpuCommandPool graphicsPool;
    GpuCommandPool transferPool;
    GpuCommandPool computePool;

    GpuStagingPool stagingPool;
    GpuQueueType currentQueueType;
    GpuSwapchain swapchain;
    uint64_t currentFrame;

    // Allocators
    VkInstance instance;

    // Heap for textures and other stuff
    GpuImagePool imagePool;
    GpuPtr resourceHeap;
    GpuPtr samplerHeap;

    GpuImage imageDeletionQueue[GGPU_FRAMES_IN_FLIGHT][GGPU_MAX_DELETIONS];
    uint32_t imageDeletionSize[GGPU_FRAMES_IN_FLIGHT];

    GpuPtr deletionQueue[GGPU_FRAMES_IN_FLIGHT][GGPU_MAX_DELETIONS];
    uint32_t deletionSize[GGPU_FRAMES_IN_FLIGHT];
} GpuContext;

// Color attachments store information about the current
// image and load/store operations
typedef struct GpuColorAttachment {
    GpuImage image;
    GpuLoadOp loadOp;
    GpuStoreOp storeOp;
    VkClearColorValue clearColor;
} GpuColorAttachment;

// Depth attachments store information about the current
// depth image and load/store operations
typedef struct GpuDepthAttachment {
    GpuImage image;
    GpuLoadOp loadOp;
    GpuStoreOp storeOp;
    float clearDepth;
} GpuDepthAttachment;

// Describes what a render pass contains
typedef struct GpuRenderPassDesc {
    GpuColorAttachment colorTargets[GGPU_MAX_COLOR_TARGETS];
    GpuDepthAttachment depthTarget;
} GpuRenderPassDesc;


// Assumes that `path` points to a valid SPIR-V bytecode file
GpuShader gpuLoadShader(char* path, GpuShaderType shaderType);

// Stalls the GPU waiting for the stage `from` to enter `to`
// `hazard` determines which resources were being used in the `from` stage
void gpuBarrierHazards(GpuCommandBuffer commandBuffer, GpuStage from, GpuStage to, GpuHazard hazard);

// Stalls the CPU waiting for the stage `from` to enter `to`, waiting for all resources
// to finish
void gpuBarrier(GpuCommandBuffer commandBuffer, GpuStage from, GpuStage to);

// Creates a semaphore object, which wraps Vulkan's semaphore objects
GpuSemaphore gpuCreateSemaphore(uint64_t value);

// Waits for a semaphore's value to equal `value` with no timeout
void gpuWaitSemaphore(GpuSemaphore semaphore, uint64_t value);

// Initializes the Vulkan and GGPU instance
void gpuInit();

// Frees an image on the next `gpuBeginFrame` or `gpuFlushDeletion`
void gpuFreeImage(GpuImage image);

// Frees an image immediately. Do this if you know that this image is not being accessed
// anywhere
void gpuFreeImageImmediate(GpuImage image);

// Frees up memory on the next `gpuBeginFrame` or `gpuFlushDeletion`
void gpuFree(GpuPtr ptr);

// Frees a block of memory immediately from GPU malloc.
// Throws an error if the buffer does not exist!
void gpuFreeImmediate(GpuPtr gpuPtr);

// Frees all pending images and buffers from `gpuFreeImage` and `gpuFree` respectively
void gpuFlushDeletion();

// Grabs a command buffer for generic GPU work
GpuCommandBuffer gpuStartCommandRecording(GpuQueueType queueType);

// Allocate GPU memory for an array. Throws an error if we run out of pool slots!
GpuPtr gpuMallocAligned(uint32_t bytes, uint32_t alignment, GpuMemoryType memoryType);

// Allocates `bytes` bytes onto the GPU, with its usage depending on `memoryType`
GpuPtr gpuMalloc(uint32_t bytes, GpuMemoryType memoryType);

// Allocates the texture and sampler heap. Must be called before any GPU work!
void gpuAllocateHeaps(uint32_t maxTextures, uint32_t maxSamplers);

// Submits the recorded commands in `commandBuffer` and stalls the CPU until work is finished
void gpuSubmitBlocking(GpuCommandBuffer commandBuffer);

// Submits the recorded commands in `commandBuffer` without stalling the CPU. Must use a semaphore callback mechanism
void gpuSubmit(GpuCommandBuffer commandBuffer, GpuSemaphore semaphore, uint64_t signalValue);

// Obtains the current swapchain image
GpuImage gpuGetSwapchainImage();

// Allocates a writable image on the GPU that can be visible from a compute shader
uint32_t gpuCreateStorageImage(GpuImage image, uint32_t slotIndex);

// Allocates an empty image on the GPU with the provided user description
GpuImage gpuCreateImage(GpuTextureDesc desc);

// Copies an image from source -> destination (use this for copying stuff over for like temporal stuff)
void gpuCopyImage(GpuCommandBuffer buffer, GpuImage source, GpuImage destination, VkOffset3D start, VkOffset3D end);

// Begins an image upload batch (use this to optimize image uploading)
GpuUploadBatch gpuBeginImageBatch();

// Uploads CPU mapped image data to the GPU
void gpuBatchUploadImage(GpuUploadBatch* batch, GpuImage dst, void* data, uint32_t bytes);

// Commits all of the images onto GPU memory and stalls the CPU
void gpuEndImageBatch(GpuUploadBatch* batch);

// Helper utility that uploads a singular CPU mapped image `data` to the GPU
void gpuUploadImage(GpuImage dst, void* data, uint32_t bytes);

// Equivalent of a gpuDraw, but you can specify the command buffer that is allocated on the GPU
// `data` is the push constant struct
// `offset` is the offset to read the command buffer,
// `drawCount` is the number of objects to draw (can be 0)
// `stride` is the size of each draw command
void gpuDrawIndirectCount(GpuPipeline* pipeline, GpuCommandBuffer commandBuffer, void* data,
    GpuPtr indirectBuffer, uint32_t offset, GpuPtr countBuffer, uint32_t countOffset,
    uint32_t maxDrawCount, uint32_t stride);

// Equivalent of a gpuDraw, but you can specify the command buffer that is allocated on the GPU
// `data` is the push constant struct
// `offset` is the offset to read the command buffer,
// `drawCount` is the number of objects to draw (can be 0)
// `stride` is the size of each draw command
void gpuDrawIndirect(GpuPipeline* pipeline, GpuCommandBuffer commandBuffer, void* data,
    GpuPtr indirectBuffer, uint32_t offset, uint32_t drawCount, uint32_t stride);

// Invokes the hardware rasterizer.
// `data` is the push constant struct
void gpuDraw(GpuPipeline* pipeline, GpuCommandBuffer commandBuffer, void* data,
    uint32_t vertexCount, uint32_t instanceCount,
    uint32_t firstVertex, uint32_t firstInstance);

// Dispatches a compute shader!
// `data` is the push constant struct
void gpuDispatch(GpuPipeline* pipeline, GpuCommandBuffer commandBuffer, void* data, uint32_t x, uint32_t y, uint32_t z);

// Dispatches a compute shader!
// `data` is the push constant struct
// `indirectBuffer` is the command buffer to write to
// `offset` offsets where the indirect buffer is read
void gpuDispatchIndirect(GpuPipeline* pipeline, GpuCommandBuffer commandBuffer, void* data, GpuPtr indirectBuffer, uint32_t offset);

// Creates a sampler with the provided `desc` in `slotIndex` of the sampler heap
uint32_t gpuCreateSampler(GpuSamplerDesc desc, uint32_t slotIndex);

// Call this when you are done writing your image. This sets the `slotIndex` of the current
// texture heap to your specified `image`.
uint32_t gpuCreateTexture(GpuImage image, uint32_t slotIndex);

// Reads `bytes` bytes from a GPU pointer and populates `destCpuData` with exactly `bytes` bytes
void gpuReadback(void* destCpuData, GpuPtr srcGpu, uint32_t bytes);

// Helper function that creates an upload batch object (used to combine allocations)
GpuUploadBatch gpuBeginUploadBatch();

// Adds an already allocated gpu pointer `dst` to be batch copied from the staging `data` buffer.
// The allocated region's length must be >= `bytes`
void gpuBatchUploadBuffer(GpuUploadBatch* batch, GpuPtr dst, void* data, uint32_t bytes);

// Stops the upload batch and frees it at the end
void gpuEndUploadBatch(GpuUploadBatch* batch);

// Helper function that automatically uploads CPU mapped data to a GPU mapped `dst` pointer
void gpuUploadBuffer(GpuPtr dst, void* data, uint32_t bytes);

// Copies `bytes` bytes starting from `src` to `dest`.
// `src` must be a valid pointer on the gpu
// `dest` must be a valid pointer on the gpu
void gpuMemcpy(GpuPtr dest, GpuPtr src, uint32_t bytes);

// Automatically changes pipeline state to rendering
// `gpuBeginFrame` must be called before this
void gpuBeginRenderPass(GpuCommandBuffer cmd, GpuRenderPassDesc desc);

// Ends the current render pass
// Make sure that this is called after `gpuBeginRenderPass`
void gpuEndRenderPass(GpuCommandBuffer cmd);

// Creates a graphics pipeline
// `graphicsPushSize` is the number of bytes of the dedicated push constant (you must send this over on draw!)
// `vertexIr` should be obtained by `gpuLoadShader` and must not be used in a pipeline already
// `fragmentIr` should be obtained by `gpuLoadShader` and must not be used in a pipeline already
GpuPipeline gpuCreateGraphicsPipeline(
    GpuGraphicsDesc rasterDescription,
    uint32_t graphicsPushSize,
    GpuShader vertexIr,
    GpuShader fragmentIr
);

// Destroys the specified pipeline
void gpuDestroyPipeline(GpuPipeline pipeline);

// Creates a compute pipeline.
// `computePushSize` is the number of bytes of the dedicated push constant (you must send this over on compute!)
// `computeIr` should be obtained by `gpuLoadShader` and must not be used in a pipeline already
GpuPipeline gpuCreateComputePipeline(uint32_t computePushSize, GpuShader computeIr);

// Destroys the entire GGPU context and frees up all allocated objects
void gpuDestroy();

// Recreates the swapchain on resize
void gpuRecreateSwapchain();

// Call this at the end of rendering
void gpuEndFrame(GpuCommandBuffer cmd);

// Call this before any other rendering function.
// Automatically calls `gpuFlushDeletion`
GpuCommandBuffer gpuBeginFrame(float r, float g, float b, float a);

#endif

// Below is an implementation of GGPU :3
// Thanks for using!
#ifndef GGPU_IMPLEMENTATION_IMPORTED
#ifdef GGPU_IMPLEMENTATION
#define GGPU_IMPLEMENTATION_IMPORTED

// Create a GPU context
_GGPU_PRIVATE GpuContext ggpu = { 0 };

_GGPU_PRIVATE PFN_vkCmdBindResourceHeapEXT _cmd_bind_resource_heap;
_GGPU_PRIVATE PFN_vkCmdBindSamplerHeapEXT _cmd_bind_sampler_heap;
_GGPU_PRIVATE PFN_vkCmdPushDataEXT _cmd_push_data;
_GGPU_PRIVATE PFN_vkWriteResourceDescriptorsEXT _cmd_write_resource_descriptors;
_GGPU_PRIVATE PFN_vkWriteSamplerDescriptorsEXT _cmd_write_sampler_descriptors;

_GGPU_PRIVATE VKAPI_ATTR VkBool32 VKAPI_CALL _gpu_validation_callback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData
) {
    printf("vulkan_validation(): %s\n", pCallbackData->pMessage);
    return VK_FALSE;
}

_GGPU_PRIVATE uint32_t _gpu_find_memory_type(uint32_t typeBits, VkMemoryPropertyFlags required) {
    VkPhysicalDeviceMemoryProperties props;
    vkGetPhysicalDeviceMemoryProperties(ggpu.physicalDevice, &props);

    for (uint32_t i = 0; i < props.memoryTypeCount; i++) {
        bool typeMatch = (typeBits >> i) & 1;
        bool propMatch = (props.memoryTypes[i].propertyFlags & required) == required;
        if (typeMatch && propMatch) return i;
    }

    gpuAssert(false, "_gpu_find_memory_type(): no suitable memory type found!");
    return 0;
}

_GGPU_PRIVATE VkSurfaceFormatKHR _gpu_choose_surface_format() {
    VkSurfaceKHR surface = ggpu.swapchain.surface;
    uint32_t count = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(ggpu.physicalDevice, surface, &count, NULL);
    VkSurfaceFormatKHR* formats = (VkSurfaceFormatKHR*) malloc(sizeof(VkSurfaceFormatKHR) * count);
    vkGetPhysicalDeviceSurfaceFormatsKHR(ggpu.physicalDevice, surface, &count, formats);

    VkSurfaceFormatKHR chosen = formats[0];
    for (uint32_t i = 0; i < count; i++) {
        if (formats[i].format == VK_FORMAT_B8G8R8A8_SRGB &&
            formats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            chosen = formats[i];
            break;
        }
    }

    free(formats);
    return chosen;
}

_GGPU_PRIVATE VkPresentModeKHR _gpu_choose_present_mode() {
    VkSurfaceKHR surface = ggpu.swapchain.surface;
    uint32_t count = 0;
    vkGetPhysicalDeviceSurfacePresentModesKHR(ggpu.physicalDevice, surface, &count, NULL);
    VkPresentModeKHR* modes = (VkPresentModeKHR*) malloc(sizeof(VkPresentModeKHR) * count);
    vkGetPhysicalDeviceSurfacePresentModesKHR(ggpu.physicalDevice, surface, &count, modes);

    VkPresentModeKHR chosen = VK_PRESENT_MODE_MAILBOX_KHR; // VK_PRESENT_MODE_FIFO_KHR;
    for (uint32_t i = 0; i < count; i++) {
        if (modes[i] == VK_PRESENT_MODE_MAILBOX_KHR) {
            chosen = modes[i];
            break;
        }
    }

    free(modes);
    return chosen;
}

// Creates a Vulkan Instance for us to interop with.
_GGPU_PRIVATE VkInstance _gpu_create_instance() {
#ifdef GGPU_VALIDATION
    const char* kEnabledLayers[] = {
        "VK_LAYER_KHRONOS_validation",
    };
#endif
    const char* kEnabledExtensions[] = {
        "VK_KHR_surface",
        "VK_KHR_win32_surface",
#ifdef GGPU_VALIDATION
        "VK_EXT_debug_utils"
#endif
    };

#ifdef GGPU_VALIDATION
    VkValidationFeatureEnableEXT kEnabledValidationFeatures[4] = {
        VK_VALIDATION_FEATURE_ENABLE_GPU_ASSISTED_RESERVE_BINDING_SLOT_EXT,
        VK_VALIDATION_FEATURE_ENABLE_GPU_ASSISTED_EXT,
        VK_VALIDATION_FEATURE_ENABLE_BEST_PRACTICES_EXT,
        VK_VALIDATION_FEATURE_ENABLE_SYNCHRONIZATION_VALIDATION_EXT
    };
#endif

    VkApplicationInfo applicationInfo = { 0 };
    applicationInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    applicationInfo.apiVersion = VK_API_VERSION_1_3;

    VkDebugUtilsMessengerCreateInfoEXT debugMessagerInfo = { 0 };
    debugMessagerInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    debugMessagerInfo.pfnUserCallback = _gpu_validation_callback;
    debugMessagerInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT;
    debugMessagerInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;

#ifdef GGPU_VALIDATION
    VkValidationFeaturesEXT validationFeaturesInfo = { 0 };
    validationFeaturesInfo.sType = VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT;
    validationFeaturesInfo.pNext = &debugMessagerInfo;
    validationFeaturesInfo.enabledValidationFeatureCount = sizeof(kEnabledValidationFeatures) / sizeof(VkValidationFeatureEnableEXT);
    validationFeaturesInfo.pEnabledValidationFeatures = &kEnabledValidationFeatures[0];
#endif

    VkInstanceCreateInfo instanceCreateInfo = { 0 };
    instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instanceCreateInfo.pApplicationInfo = &applicationInfo;
    instanceCreateInfo.enabledExtensionCount = sizeof(kEnabledExtensions) / sizeof(char*);
    instanceCreateInfo.ppEnabledExtensionNames = kEnabledExtensions;

#ifdef GGPU_VALIDATION
    instanceCreateInfo.pNext = &validationFeaturesInfo;
    instanceCreateInfo.enabledLayerCount = sizeof(kEnabledLayers) / sizeof(char*);
    instanceCreateInfo.ppEnabledLayerNames = kEnabledLayers;
#else
    instanceCreateInfo.enabledLayerCount = 0;
    instanceCreateInfo.ppEnabledLayerNames = NULL;
#endif

    VkInstance instance;
    gpuAssert(vkCreateInstance(&instanceCreateInfo, NULL, &instance) == VK_SUCCESS, "_gpu_create_instance(): could not initialize Vulkan");
    ggpu.instance = instance;

    return instance;
}

_GGPU_PRIVATE void _gpu_register_swapchain_images() {
    GpuSwapchain* sc = &ggpu.swapchain;
    for (uint32_t i = 0; i < sc->imageCount; i++) {
        uint32_t slot = GGPU_IMAGE_SWAPCHAIN_START + i;
        GpuImageInternal* img = &ggpu.imagePool.images[slot];
        img->image = sc->images[i];
        img->view = sc->views[i];
        img->memory = VK_NULL_HANDLE;
        img->format = sc->format;
        img->extent = sc->extent;
        img->aspect = VK_IMAGE_ASPECT_COLOR_BIT;
        img->heapIndex = UINT32_MAX;
    }

    if (ggpu.imagePool.poolSize < GGPU_IMAGE_USER_START)
        ggpu.imagePool.poolSize = GGPU_IMAGE_USER_START;
}

_GGPU_PRIVATE VkPhysicalDevice _gpu_get_physical_device() {
    uint32_t physicalDeviceCount;
    gpuAssert(vkEnumeratePhysicalDevices(ggpu.instance, &physicalDeviceCount, NULL) == VK_SUCCESS, "_gpu_get_physical_device(): no GPUs found!");

    VkPhysicalDevice* physicalDevices = (VkPhysicalDevice*) malloc(sizeof(VkPhysicalDevice) * physicalDeviceCount);
    gpuAssert(vkEnumeratePhysicalDevices(ggpu.instance, &physicalDeviceCount, physicalDevices) == VK_SUCCESS, "_gpu_get_physical_device(): no physical devices found?");

    VkPhysicalDeviceProperties bestProperties = { 0 };
    uint32_t bestPhysicalDevice = 0;
    uint32_t bestScore = 0;

    for (uint32_t i = 0; i < physicalDeviceCount; i++) {
        VkPhysicalDeviceProperties2 physicalDeviceProperties = { 0 };
        physicalDeviceProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
        vkGetPhysicalDeviceProperties2(physicalDevices[i], &physicalDeviceProperties);

        // Check what *is* it exactly?
        uint32_t score = (physicalDeviceProperties.properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU ? 1000 : 0)
            + (physicalDeviceProperties.properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU ? 100 : 0)
            + (physicalDeviceProperties.properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU ? 10 : 0)
            + 1;

        if (bestScore < score) {
            bestScore = score;
            bestPhysicalDevice = i;
            bestProperties = physicalDeviceProperties.properties;
        }
    }

    // Free everything
    VkPhysicalDevice chosen = physicalDevices[bestPhysicalDevice];
#ifdef GGPU_VALIDATION
    printf("_gpu_get_physical_device(): Choosing physical device '%s'\n", bestProperties.deviceName);
#endif
    free(physicalDevices);

    ggpu.physicalDevice = chosen;

    return chosen;
}

// For binary testing
_GGPU_PRIVATE bool _gpu_btest(uint32_t a, uint32_t b) {
    return (a & b) == b;
}

// Provides us a queue create info structure for us. The index is in `queueFamilyIndex`
_GGPU_PRIVATE VkDeviceQueueCreateInfo _gpu_find_queue_family(bool graphics, bool compute, bool transfer, uint32_t start) {
    uint32_t familyCount;
    vkGetPhysicalDeviceQueueFamilyProperties(ggpu.physicalDevice, &familyCount, NULL);

    VkQueueFamilyProperties* familyProperties = (VkQueueFamilyProperties*) malloc(sizeof(VkQueueFamilyProperties) * familyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(ggpu.physicalDevice, &familyCount, familyProperties);

    uint32_t graphicsFamily = 0;
    for (graphicsFamily = start; graphicsFamily < familyCount; graphicsFamily++) {
        VkQueueFamilyProperties properties = familyProperties[graphicsFamily];
        if (properties.queueCount == 0) continue;

        // Is it what we are looking for?
        if (_gpu_btest(properties.queueFlags, VK_QUEUE_TRANSFER_BIT) && transfer) break;
        if (_gpu_btest(properties.queueFlags, VK_QUEUE_GRAPHICS_BIT) && graphics) break;
        if (_gpu_btest(properties.queueFlags, VK_QUEUE_COMPUTE_BIT) && compute) break;
    }

    free(familyProperties);

    _GGPU_PRIVATE float priority = 1.0f;
    VkDeviceQueueCreateInfo deviceQueueCreateInfo = { 0 };
    deviceQueueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    deviceQueueCreateInfo.queueFamilyIndex = graphicsFamily;
    deviceQueueCreateInfo.queueCount = 1;
    deviceQueueCreateInfo.pQueuePriorities = &priority;

    return deviceQueueCreateInfo;
}

// Internal use for getting the raw image
_GGPU_PRIVATE GpuImageInternal* _gpu_get_raw_image(GpuImage image) {
    return &ggpu.imagePool.images[image.index];
}

// Creates a command pool
_GGPU_PRIVATE GpuCommandPool _gpu_create_command_pool(uint32_t queueFamilyIndex) {
    GpuCommandPool commandPool = { 0 };
    vkGetDeviceQueue(ggpu.device, queueFamilyIndex, 0, &commandPool.queue);

    VkCommandPoolCreateInfo poolCreateInfo = { 0 };
    poolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolCreateInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT | VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolCreateInfo.queueFamilyIndex = queueFamilyIndex;

    // Create a set amount of stuff
    for (uint32_t i = 0; i < GGPU_FRAMES_IN_FLIGHT; i++) {
        VkCommandPool poolObject;
        gpuAssert(vkCreateCommandPool(ggpu.device, &poolCreateInfo, NULL, &poolObject) == VK_SUCCESS, "_gpu_create_command_pool(): failed to allocate command pool");

        VkCommandBufferAllocateInfo commandBufferCreateInfo = { 0 };
        commandBufferCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        commandBufferCreateInfo.commandBufferCount = GGPU_CMD_BUFS_PER_POOL;
        commandBufferCreateInfo.commandPool = poolObject;
        commandBufferCreateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

        // Bulk create some command buffers...
        gpuAssert(vkAllocateCommandBuffers(
            ggpu.device,
            &commandBufferCreateInfo,
            &commandPool.commandBuffers[GGPU_CMD_BUFS_PER_POOL * i]
        ) == VK_SUCCESS, "_gpu_create_command_pool(): failed to allocate command buffers");

        commandPool.commandPools[i] = poolObject;
        commandPool.commandQueue[i] = 0;
    }

    return commandPool;
}

// Creates the device
_GGPU_PRIVATE VkDevice _gpu_create_device() {
    const char* kRequiredExtensions[] = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
        VK_EXT_SHADER_OBJECT_EXTENSION_NAME,
        VK_KHR_DRAW_INDIRECT_COUNT_EXTENSION_NAME,
        VK_EXT_DESCRIPTOR_HEAP_EXTENSION_NAME, // ?
        VK_KHR_SHADER_UNTYPED_POINTERS_EXTENSION_NAME,
        VK_KHR_SHADER_NON_SEMANTIC_INFO_EXTENSION_NAME,
        VK_KHR_MAINTENANCE_5_EXTENSION_NAME,
        VK_KHR_UNIFIED_IMAGE_LAYOUTS_EXTENSION_NAME
    };

    VkPhysicalDeviceUnifiedImageLayoutsFeaturesKHR unifiedLayouts = { 0 };
    unifiedLayouts.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_UNIFIED_IMAGE_LAYOUTS_FEATURES_KHR;
    unifiedLayouts.unifiedImageLayouts = true;

    VkPhysicalDeviceShaderUntypedPointersFeaturesKHR untypedPointersFeatures = { 0 };
    untypedPointersFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_UNTYPED_POINTERS_FEATURES_KHR;
    untypedPointersFeatures.shaderUntypedPointers = true;
    untypedPointersFeatures.pNext = &unifiedLayouts;

    // For descriptor heap stuff
    VkPhysicalDeviceDescriptorHeapFeaturesEXT descriptorHeap = { 0 };
    descriptorHeap.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_HEAP_FEATURES_EXT;
    descriptorHeap.descriptorHeap = true;
    descriptorHeap.pNext = &untypedPointersFeatures;

    // Specifically towards device creation
    VkPhysicalDeviceVulkan12Features vulkan12Features = { 0 };
    vulkan12Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
    vulkan12Features.runtimeDescriptorArray = true;
    vulkan12Features.vulkanMemoryModel = true;
    vulkan12Features.vulkanMemoryModelDeviceScope = true;
    vulkan12Features.storageBuffer8BitAccess = true;
    vulkan12Features.shaderSampledImageArrayNonUniformIndexing = true;
    vulkan12Features.shaderStorageImageArrayNonUniformIndexing = true;
    vulkan12Features.descriptorBindingSampledImageUpdateAfterBind = true;
    vulkan12Features.descriptorBindingStorageImageUpdateAfterBind = true;
    vulkan12Features.descriptorBindingPartiallyBound = true;
    vulkan12Features.timelineSemaphore = true;
    vulkan12Features.bufferDeviceAddress = true;
    vulkan12Features.drawIndirectCount = true;
    vulkan12Features.scalarBlockLayout = true;
    vulkan12Features.shaderInt8 = true;
    vulkan12Features.pNext = &descriptorHeap;

    VkPhysicalDeviceVulkan11Features vulkan11Features = { 0 };
    vulkan11Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES;
    vulkan11Features.shaderDrawParameters = true;
    vulkan11Features.storageBuffer16BitAccess = true;
    vulkan11Features.pNext = &vulkan12Features;

    VkPhysicalDeviceVulkan13Features vulkan13Features = { 0 };
    vulkan13Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
    vulkan13Features.dynamicRendering = true;
    vulkan13Features.synchronization2 = true;
    vulkan13Features.pNext = &vulkan11Features;

    VkPhysicalDeviceShaderObjectFeaturesEXT shaderObjectFeatures = { 0 };
    shaderObjectFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_OBJECT_FEATURES_EXT;
    shaderObjectFeatures.shaderObject = true;
    shaderObjectFeatures.pNext = &vulkan13Features;

    VkPhysicalDeviceDepthClipEnableFeaturesEXT clipFeatures = { 0 };
    clipFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DEPTH_CLIP_ENABLE_FEATURES_EXT;
    clipFeatures.depthClipEnable = true;
    clipFeatures.pNext = &shaderObjectFeatures;

    VkPhysicalDeviceDynamicRenderingUnusedAttachmentsFeaturesEXT unused = { 0 };
    unused.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_UNUSED_ATTACHMENTS_FEATURES_EXT;
    unused.dynamicRenderingUnusedAttachments = true;
    unused.pNext = &clipFeatures;

    VkPhysicalDeviceFeatures2 features = { 0 };
    features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    features.features.multiDrawIndirect = true;
    features.features.shaderInt64 = true;
    features.features.shaderInt16 = true;
    features.features.vertexPipelineStoresAndAtomics = true;
    features.features.samplerAnisotropy = true;
    features.features.independentBlend = true;
    features.features.imageCubeArray = true;
    features.pNext = &unused;

    VkDeviceQueueCreateInfo graphicsInfo = _gpu_find_queue_family(true, false, false, 0);
    VkDeviceQueueCreateInfo computeInfo = _gpu_find_queue_family(false, true, false, graphicsInfo.queueFamilyIndex + 1);
    VkDeviceQueueCreateInfo transferInfo = _gpu_find_queue_family(false, false, true, computeInfo.queueFamilyIndex + 1);

    VkDeviceQueueCreateInfo queueCreateInfos[3] = { graphicsInfo, computeInfo, transferInfo };

    VkDeviceCreateInfo deviceCreateInfo = { 0 };
    deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceCreateInfo.pNext = &features;
    deviceCreateInfo.queueCreateInfoCount = 3;
    deviceCreateInfo.pQueueCreateInfos = queueCreateInfos;
    deviceCreateInfo.ppEnabledExtensionNames = kRequiredExtensions;
    deviceCreateInfo.enabledExtensionCount = sizeof(kRequiredExtensions) / sizeof(char*);

    VkDevice device;
    gpuAssert(vkCreateDevice(ggpu.physicalDevice, &deviceCreateInfo, NULL, &device) == VK_SUCCESS,
        "_gpu_create_device(): could not create device!");

    // Populate the queues...
    ggpu.device = device;
    ggpu.graphicsPool = _gpu_create_command_pool(queueCreateInfos[0].queueFamilyIndex);
    ggpu.transferPool = _gpu_create_command_pool(queueCreateInfos[2].queueFamilyIndex);
    ggpu.computePool = _gpu_create_command_pool(queueCreateInfos[1].queueFamilyIndex);

    return device;
}

// Helper utility
_GGPU_PRIVATE VkImageAspectFlags _gpu_get_aspect_flags(VkImageCreateInfo imageCreateInfo) {
    VkImageAspectFlags aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT;
    switch (imageCreateInfo.format) {
        case VK_FORMAT_D16_UNORM:
        case VK_FORMAT_D24_UNORM_S8_UINT:
        case VK_FORMAT_D32_SFLOAT:
            aspectFlags = VK_IMAGE_ASPECT_DEPTH_BIT;
    }

    return aspectFlags;
}

// Helper utility
_GGPU_PRIVATE VkPipelineStageFlags2 _gpu_get_stage_flags(GpuStage stage) {
    VkPipelineStageFlags2 flags = 0;

    if (_gpu_btest(stage, kTransferStage)) flags |= VK_PIPELINE_STAGE_2_TRANSFER_BIT;
    if (_gpu_btest(stage, kComputeStage)) flags |= VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
    if (_gpu_btest(stage, kVertexStage)) flags |= VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT;
    if (_gpu_btest(stage, kFragmentStage)) flags |= VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
    if (_gpu_btest(stage, kColorOutputStage)) flags |= VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
    if (_gpu_btest(stage, kDepthOutputStage)) flags |= VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT;

    // Safety fallback if no bits were matched
    if (flags == 0) flags |= VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
    return flags;
}

// Creates synchronization again
_GGPU_PRIVATE void _gpu_create_fences() {
    VkFenceCreateInfo fenceCreateInfo = { 0 };
    fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    vkCreateFence(ggpu.device, &fenceCreateInfo, NULL, &ggpu.stagingPool.stagingFence);
    vkResetFences(ggpu.device, 1, &ggpu.stagingPool.stagingFence);
}

// Helper utility
_GGPU_PRIVATE VkPipelineStageFlags2 _gpu_layout_src_stage(VkImageLayout layout) {
    switch (layout) {
        case VK_IMAGE_LAYOUT_UNDEFINED: return VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
        case VK_IMAGE_LAYOUT_PRESENT_SRC_KHR: return VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
        case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL: return VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
        case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL: return VK_PIPELINE_STAGE_2_COPY_BIT;
        case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL: return VK_PIPELINE_STAGE_2_COPY_BIT;
        case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL: return VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
        case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL: return VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT;
        default: return VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
    }
}

// Helper utility
_GGPU_PRIVATE VkAccessFlags2 _gpu_layout_src_access(VkImageLayout layout) {
    switch (layout) {
        case VK_IMAGE_LAYOUT_UNDEFINED: return VK_ACCESS_2_NONE;
        case VK_IMAGE_LAYOUT_PRESENT_SRC_KHR: return VK_ACCESS_2_NONE;
        case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL: return VK_ACCESS_2_SHADER_READ_BIT;
        case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL: return VK_ACCESS_2_TRANSFER_WRITE_BIT;
        case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL: return VK_ACCESS_2_TRANSFER_READ_BIT;
        case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL: return VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
        case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL: return VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        default: return VK_ACCESS_2_MEMORY_WRITE_BIT;
    }
}

// Creates a basic heap
_GGPU_PRIVATE void _gpu_create_basic_heap() {
    // load descriptor heap proc addrs
    _cmd_bind_resource_heap = (PFN_vkCmdBindResourceHeapEXT) vkGetDeviceProcAddr(ggpu.device, "vkCmdBindResourceHeapEXT");
    _cmd_bind_sampler_heap = (PFN_vkCmdBindSamplerHeapEXT) vkGetDeviceProcAddr(ggpu.device, "vkCmdBindSamplerHeapEXT");
    _cmd_push_data = (PFN_vkCmdPushDataEXT) vkGetDeviceProcAddr(ggpu.device, "vkCmdPushDataEXT");
    _cmd_write_resource_descriptors = (PFN_vkWriteResourceDescriptorsEXT) vkGetDeviceProcAddr(ggpu.device, "vkWriteResourceDescriptorsEXT");
    _cmd_write_sampler_descriptors = (PFN_vkWriteSamplerDescriptorsEXT) vkGetDeviceProcAddr(ggpu.device, "vkWriteSamplerDescriptorsEXT");

    // cache heap properties so gpuCreateImage knows descriptor sizes
    ggpu.heapProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_HEAP_PROPERTIES_EXT;
    VkPhysicalDeviceProperties2 props2 = { 0 };
    props2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
    props2.pNext = &ggpu.heapProperties;
    vkGetPhysicalDeviceProperties2(ggpu.physicalDevice, &props2);
}

// Gets the GPU buffer associated with a GPU pointer
_GGPU_PRIVATE GpuBuffer _gpu_get_buffer(GpuPtr ptr) {
    GpuBuffer* allocationPool = ggpu.stagingPool.allocationsPool;
    return allocationPool[ptr.allocationPoolId];
}

// Binds descriptors
_GGPU_PRIVATE void _gpu_bind_descriptor_heap(GpuCommandBuffer cmd) {
    if (_cmd_bind_resource_heap == NULL) return;

    VkPhysicalDeviceDescriptorHeapPropertiesEXT* props = &ggpu.heapProperties;

    VkBindHeapInfoEXT resourceInfo = { 0 };
    resourceInfo.sType = VK_STRUCTURE_TYPE_BIND_HEAP_INFO_EXT;
    resourceInfo.heapRange.address = ggpu.resourceHeap.gpuAddress;
    resourceInfo.heapRange.size = _gpu_get_buffer(ggpu.resourceHeap).bytes;
    resourceInfo.reservedRangeOffset = 0;
    resourceInfo.reservedRangeSize = props->minResourceHeapReservedRange;
    _cmd_bind_resource_heap(cmd.buffer, &resourceInfo);

    VkBindHeapInfoEXT samplerInfo = { 0 };
    samplerInfo.sType = VK_STRUCTURE_TYPE_BIND_HEAP_INFO_EXT;
    samplerInfo.heapRange.address = ggpu.samplerHeap.gpuAddress;
    samplerInfo.heapRange.size = _gpu_get_buffer(ggpu.samplerHeap).bytes;
    samplerInfo.reservedRangeOffset = 0;
    samplerInfo.reservedRangeSize = props->minSamplerHeapReservedRange;
    _cmd_bind_sampler_heap(cmd.buffer, &samplerInfo);
}

// Initializes a Windows only swapchain object, after calling `gpuCreateSurface`
void gpuCreateSwapchain(void (*callback)(int width, int height));

// Assumes that `path` points to a valid SPIR-V bytecode file
GpuShader gpuLoadShader(char* path, GpuShaderType shaderType) {
    // Read!
    FILE* currentFile = fopen(path, "rb");
    gpuAssert(currentFile != NULL, "gpuLoadShader(): could not read `%s`", path);

    // Read to the end...
    fseek(currentFile, 0, SEEK_END);
    uint32_t size = ftell(currentFile);
    fseek(currentFile, 0, SEEK_SET);

    gpuAssert(size > 0 && (size % 4) == 0, "gpuLoadShader(): `%s` is not valid SPIR-V!", path);

    uint32_t* code = malloc(size);
    fread(code, 1, size, currentFile);
    fclose(currentFile);

    VkShaderModule module;
    VkShaderModuleCreateInfo moduleInfo = { 0 };
    moduleInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    moduleInfo.codeSize = size;
    moduleInfo.pCode = code;

    gpuAssert(vkCreateShaderModule(ggpu.device, &moduleInfo, NULL, &module) == VK_SUCCESS,
        "gpuCreateShaderModule(): failed to create shader module for '%s'", path);
    free(code);

    GpuShader shader = { 0 };
    shader.shaderType = shaderType;
    shader.shaderName = path;
    shader.module = module;

    return shader;
}

// Stalls the GPU waiting for the stage `from` to enter `to`
// `hazard` determines which resources were being used in the `from` stage
void gpuBarrierHazards(GpuCommandBuffer commandBuffer, GpuStage from, GpuStage to, GpuHazard hazard) {
    VkAccessFlags2 srcAccess = 0;
    VkAccessFlags2 dstAccess = 0;

    if (_gpu_btest(hazard, kIndirectHazard)) {
        srcAccess |= VK_ACCESS_2_SHADER_WRITE_BIT;
        dstAccess |= VK_ACCESS_2_INDIRECT_COMMAND_READ_BIT;
    }

    if (_gpu_btest(hazard, kDescriptorHazards)) {
        dstAccess |= VK_ACCESS_2_SHADER_READ_BIT | VK_ACCESS_2_SHADER_STORAGE_READ_BIT;
        srcAccess |= VK_ACCESS_2_SHADER_WRITE_BIT |
                     VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT |
                     VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    }

    if (_gpu_btest(hazard, kDepthHazard)) {
        srcAccess |= VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        dstAccess |= VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    }

    if (hazard == 0) {
        srcAccess = VK_ACCESS_2_MEMORY_WRITE_BIT;
        dstAccess = VK_ACCESS_2_MEMORY_READ_BIT | VK_ACCESS_2_MEMORY_WRITE_BIT;
    }

    VkMemoryBarrier2 memoryBarrier = { 0 };
    memoryBarrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER_2;
    memoryBarrier.srcAccessMask = srcAccess;
    memoryBarrier.srcStageMask = _gpu_get_stage_flags(from);
    memoryBarrier.dstAccessMask = dstAccess;
    memoryBarrier.dstStageMask = _gpu_get_stage_flags(to);

    VkDependencyInfo dependency = { 0 };
    dependency.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
    dependency.memoryBarrierCount = 1;
    dependency.pMemoryBarriers = &memoryBarrier;

    vkCmdPipelineBarrier2(commandBuffer.buffer, &dependency);
}

// Stalls the CPU waiting for the stage `from` to enter `to`, waiting for all resources
// to finish
void gpuBarrier(GpuCommandBuffer commandBuffer, GpuStage from, GpuStage to) {
    gpuBarrierHazards(commandBuffer, from, to, 0);
}

// Creates a semaphore object, which wraps Vulkan's semaphore objects
GpuSemaphore gpuCreateSemaphore(uint64_t value) {
    GpuSemaphore semaphore = { 0 };
    semaphore.value = value;

    VkSemaphoreTypeCreateInfo semaphoreTypeInfo = { 0 };
    semaphoreTypeInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    semaphoreTypeInfo.semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE;
    semaphoreTypeInfo.initialValue = value;

    VkSemaphoreCreateInfo semaphoreCreateInfo = { 0 };
    semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    semaphoreCreateInfo.pNext = &semaphoreTypeInfo;

    vkCreateSemaphore(ggpu.device, &semaphoreCreateInfo, NULL, &semaphore.semaphore);

    return semaphore;
}

// Waits for a semaphore's value to equal `value` with no timeout
void gpuWaitSemaphore(GpuSemaphore semaphore, uint64_t value) {
    VkSemaphoreWaitInfo semaphoreWaitInfo = { 0 };
    semaphoreWaitInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO;
    semaphoreWaitInfo.pSemaphores = &semaphore.semaphore;
    semaphoreWaitInfo.semaphoreCount = 1;
    semaphoreWaitInfo.pValues = &value;

    vkWaitSemaphores(ggpu.device, &semaphoreWaitInfo, UINT64_MAX);
}

// Initializes the Vulkan and GGPU instance
void gpuInit() {
    _gpu_create_instance();
    _gpu_get_physical_device();
    _gpu_create_device();
    _gpu_create_fences();
    _gpu_create_basic_heap();
#ifdef GGPU_VALIDATION
    printf("gpuInit(): initialized Vulkan!\n");
#endif
}

// Frees an image on the next `gpuBeginFrame` or `gpuFlushDeletion`
void gpuFreeImage(GpuImage image) {
    // lol
    if (!image.active) return;

    uint32_t frame = ggpu.currentFrame % GGPU_FRAMES_IN_FLIGHT;
    gpuAssert(ggpu.imageDeletionSize[frame] < GGPU_MAX_DELETIONS,
        "gpuFreeImage(): deletion queue full! increase GGPU_MAX_DELETIONS");
    ggpu.imageDeletionQueue[frame][ggpu.imageDeletionSize[frame]++] = image;
}

// Frees an image immediately. Do this if you know that this image is not being accessed
// anywhere
void gpuFreeImageImmediate(GpuImage image) {
    gpuAssert(image.index >= GGPU_IMAGE_USER_START,
        "gpuFreeImageImmediate(): cannot destroy a swapchain image!");

    GpuImagePool* pool = &ggpu.imagePool;
    GpuImageInternal* entry = &pool->images[image.index];
    gpuAssert(entry->image != VK_NULL_HANDLE, "gpuFreeImageImmediate(): double free!");

    vkDestroyImageView(ggpu.device, entry->view, NULL);
    vkDestroyImage(ggpu.device, entry->image, NULL);
    vkFreeMemory(ggpu.device, entry->memory, NULL);

    *entry = (GpuImageInternal){ 0 };
    pool->freeSlots[pool->freeSlotsCount++] = image.index;
}

// Frees up memory on the next `gpuBeginFrame` or `gpuFlushDeletion`
void gpuFree(GpuPtr ptr) {
    uint32_t frame = ggpu.currentFrame % GGPU_FRAMES_IN_FLIGHT;
    gpuAssert(ggpu.deletionSize[frame] < GGPU_MAX_DELETIONS,
        "gpuFree(): deletion queue full! increase GGPU_MAX_DELETIONS");

    ggpu.deletionQueue[frame][ggpu.deletionSize[frame]++] = ptr;
}

// Frees a block of memory immediately from GPU malloc.
// Throws an error if the buffer does not exist!
void gpuFreeImmediate(GpuPtr gpuPtr) {
    GpuBuffer* entry = &ggpu.stagingPool.allocationsPool[gpuPtr.allocationPoolId];
    gpuAssert(entry->buffer != VK_NULL_HANDLE, "gpuFreeImmediate(): double free!");

    if (entry->cpu != NULL)
        vkUnmapMemory(ggpu.device, entry->memory);

    vkDestroyBuffer(ggpu.device, entry->buffer, NULL);
    vkFreeMemory(ggpu.device, entry->memory, NULL);

    *entry = (GpuBuffer){ 0 };
    ggpu.stagingPool.freeSlots[ggpu.stagingPool.freeSlotsCount++] = gpuPtr.allocationPoolId;
}

// Frees all pending images and buffers from `gpuFreeImage` and `gpuFree` respectively
void gpuFlushDeletion() {
    uint32_t frame = ggpu.currentFrame % GGPU_FRAMES_IN_FLIGHT;
    GpuStagingPool* pool = &ggpu.stagingPool;

    uint32_t queueSize = ggpu.deletionSize[frame];
    for (uint32_t i = 0; i < queueSize; i++)
        gpuFreeImmediate(ggpu.deletionQueue[frame][i]);

    uint32_t imageQueueSize = ggpu.imageDeletionSize[frame];
    for (uint32_t i = 0; i < imageQueueSize; i++)
        gpuFreeImageImmediate(ggpu.imageDeletionQueue[frame][i]);

    ggpu.imageDeletionSize[frame] = 0;
    ggpu.deletionSize[frame] = 0;
}

// Grabs a command buffer for generic GPU work
GpuCommandBuffer gpuStartCommandRecording(GpuQueueType queueType) {
    uint32_t frameIndex = ggpu.currentFrame % GGPU_FRAMES_IN_FLIGHT;
    GpuCommandPool* targetContext = NULL;

    switch (queueType) {
        case kGraphicsQueue: targetContext = &ggpu.graphicsPool; break;
        case kComputeQueue: targetContext = &ggpu.computePool; break;
        case kTransferQueue: targetContext = &ggpu.transferPool; break;
    }

    uint32_t cmdIdx = targetContext->commandQueue[frameIndex];
    gpuAssert(cmdIdx < GGPU_CMD_BUFS_PER_POOL, "gpuStartCommandRecording(): exceeded max commands buffers!");
    targetContext->commandQueue[frameIndex]++;

    VkCommandBuffer commandBuffer = targetContext->commandBuffers[frameIndex * GGPU_CMD_BUFS_PER_POOL + cmdIdx];
    VkCommandBufferBeginInfo beginInfo = { 0 };
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    GpuCommandBuffer gpuCommandBuffer = { 0 };
    gpuCommandBuffer.buffer = commandBuffer;
    gpuCommandBuffer.queueType = queueType;
    ggpu.currentQueueType = queueType;

    return gpuCommandBuffer;
}

// Allocate GPU memory for an array. Throws an error if we run out of pool slots!
GpuPtr gpuMallocAligned(uint32_t bytes, uint32_t alignment, GpuMemoryType memoryType) {
    gpuAssert(bytes > 0, "gpuMallocAligned(): cannot allocate an empty buffer!");

    VkMemoryPropertyFlags propertyFlags = 0;
    VkBufferUsageFlags usageFlags = 0;
    bool needsMapping = false;

    switch (memoryType) {
        case kSharedMemory:
            propertyFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
            needsMapping = true;
            break;
        case kGpuMemory:
            propertyFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
            usageFlags = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT
                | VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
            needsMapping = false;
            break;
        case kCpuMemory:
            propertyFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_CACHED_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
            needsMapping = true;
            break;
        case kHeapMemory:
            propertyFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
            usageFlags = VK_BUFFER_USAGE_DESCRIPTOR_HEAP_BIT_EXT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
            needsMapping = true;
            break;
    }

    // Default usage for everything that isn't a descriptor heap buffer
    if (usageFlags == 0)
        usageFlags = VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT
            | VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;

    VkBufferCreateInfo bufferCreateInfo = { 0 };
    bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferCreateInfo.size = bytes;
    bufferCreateInfo.usage = usageFlags;
    bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VkBuffer buffer;
    gpuAssert(vkCreateBuffer(ggpu.device, &bufferCreateInfo, NULL, &buffer) == VK_SUCCESS,
        "gpuMallocAligned(): vkCreateBuffer failed! size = %u", bytes);

    VkMemoryRequirements memReqs;
    vkGetBufferMemoryRequirements(ggpu.device, buffer, &memReqs);

    // Respect caller alignment on top of Vulkan's own requirement
    VkDeviceSize finalAlignment = memReqs.alignment;
    if (alignment != 0 && alignment > finalAlignment)
        finalAlignment = alignment;

    VkDeviceSize allocSize = (memReqs.size + finalAlignment - 1) & ~(finalAlignment - 1);
    VkPhysicalDeviceMemoryProperties memProps;
    vkGetPhysicalDeviceMemoryProperties(ggpu.physicalDevice, &memProps);

    uint32_t memTypeIndex = UINT32_MAX;
    for (uint32_t i = 0; i < memProps.memoryTypeCount; i++) {
        bool typeBitMatch = (memReqs.memoryTypeBits >> i) & 1;
        bool propMatch = (memProps.memoryTypes[i].propertyFlags & propertyFlags) == propertyFlags;
        if (typeBitMatch && propMatch) {
            memTypeIndex = i;
            break;
        }
    }

    gpuAssert(memTypeIndex != UINT32_MAX,
        "gpuMallocAligned(): no suitable memory type found for flags 0x%x!", propertyFlags);

    // BDA requires DEVICE_ADDRESS_BIT on the allocation itself
    VkMemoryAllocateFlagsInfo allocFlagsInfo = { 0 };
    allocFlagsInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO;
    allocFlagsInfo.flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT;

    VkMemoryAllocateInfo allocInfo = { 0 };
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.pNext = &allocFlagsInfo;
    allocInfo.allocationSize = allocSize;
    allocInfo.memoryTypeIndex = memTypeIndex;

    VkDeviceMemory memory;
    gpuAssert(vkAllocateMemory(ggpu.device, &allocInfo, NULL, &memory) == VK_SUCCESS,
        "gpuMallocAligned(): vkAllocateMemory failed! size = %llu", (unsigned long long)allocSize);

    gpuAssert(vkBindBufferMemory(ggpu.device, buffer, memory, 0) == VK_SUCCESS,
        "gpuMallocAligned(): vkBindBufferMemory failed!");

    void* mapped = NULL;
    if (needsMapping)
        gpuAssert(vkMapMemory(ggpu.device, memory, 0, allocSize, 0, &mapped) == VK_SUCCESS,
            "gpuMallocAligned(): vkMapMemory failed!");

    VkBufferDeviceAddressInfo addressInfo = { 0 };
    addressInfo.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
    addressInfo.buffer = buffer;
    VkDeviceAddress deviceAddress = vkGetBufferDeviceAddress(ggpu.device, &addressInfo);

    GpuStagingPool* stagingPool = &ggpu.stagingPool;
    uint32_t slotId = 0;
    if (stagingPool->freeSlotsCount > 0) {
        slotId = stagingPool->freeSlots[--stagingPool->freeSlotsCount];
    } else {
        gpuAssert(stagingPool->allocPoolSize < GGPU_MAX_ALLOCATIONS,
            "gpuMallocAligned(): out of tracking pool slots!");
        slotId = stagingPool->allocPoolSize++;
    }

    GpuBuffer* poolEntry = &stagingPool->allocationsPool[slotId];
    poolEntry->buffer = buffer;
    poolEntry->memory = memory;
    poolEntry->cpu = mapped;
    poolEntry->gpu = (GpuAddress) deviceAddress;
    poolEntry->bytes = bytes;
    poolEntry->memoryType = memoryType;

    GpuPtr gpuPtr;
    gpuPtr.cpuAddress = mapped;
    gpuPtr.gpuAddress = (GpuAddress) deviceAddress;
    gpuPtr.allocationPoolId = slotId;

    return gpuPtr;
}

// Allocates `bytes` bytes onto the GPU, with its usage depending on `memoryType`
GpuPtr gpuMalloc(uint32_t bytes, GpuMemoryType memoryType) {
    return gpuMallocAligned(bytes, 0, memoryType);
}

// Allocates the texture and sampler heap. Must be called before any GPU work!
void gpuAllocateHeaps(uint32_t maxTextures, uint32_t maxSamplers) {
    VkPhysicalDeviceDescriptorHeapPropertiesEXT* props = &ggpu.heapProperties;
    VkDeviceSize resourceHeapSize = props->minResourceHeapReservedRange + maxTextures * props->imageDescriptorSize;
    VkDeviceSize samplerHeapSize = props->minSamplerHeapReservedRange + maxSamplers * props->samplerDescriptorSize;

    // align to heap alignment requirements
    resourceHeapSize = (resourceHeapSize + props->resourceHeapAlignment - 1) & ~(props->resourceHeapAlignment - 1);
    samplerHeapSize = (samplerHeapSize  + props->samplerHeapAlignment - 1) & ~(props->samplerHeapAlignment - 1);
    ggpu.resourceHeap = gpuMalloc((uint32_t) resourceHeapSize, kHeapMemory);
    ggpu.samplerHeap = gpuMalloc((uint32_t) samplerHeapSize, kHeapMemory);
    ggpu.maxTextures = maxTextures;
    ggpu.maxSamplers = maxSamplers;
}

// Submits the recorded commands in `commandBuffer` and stalls the CPU until work is finished
void gpuSubmitBlocking(GpuCommandBuffer commandBuffer) {
    gpuAssert(commandBuffer.buffer != VK_NULL_HANDLE, "gpuSubmitBlocking(): you passed in a null command buffer...");
    gpuAssert(vkEndCommandBuffer(commandBuffer.buffer) == VK_SUCCESS, "gpuSubmitBlocking(): failed to end command buffer recording");

    // Alright, which queue do I use?
    VkQueue targetQueue = VK_NULL_HANDLE;
    switch (commandBuffer.queueType) {
        case kGraphicsQueue: targetQueue = ggpu.graphicsPool.queue; break;
        case kComputeQueue: targetQueue = ggpu.computePool.queue; break;
        case kTransferQueue: targetQueue = ggpu.transferPool.queue; break;
    }

    // Try submitting stuff over...
    VkSubmitInfo submitInfo = { 0 };
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer.buffer;

    VkFence fence = ggpu.stagingPool.stagingFence;
    vkResetFences(ggpu.device, 1, &fence);

    VkResult result = vkQueueSubmit(targetQueue, 1, &submitInfo, fence);
    gpuAssert(result == VK_SUCCESS,
        "gpuSubmitBlocking(): hardware driver rejected your command queue submission! got code=%d", result);

    // Stall the CPU...
    vkWaitForFences(ggpu.device, 1, &fence, VK_TRUE, UINT64_MAX);
}

// Submits the recorded commands in `commandBuffer` without stalling the CPU. Must use a semaphore callback mechanism
void gpuSubmit(GpuCommandBuffer commandBuffer, GpuSemaphore semaphore, uint64_t signalValue) {
    gpuAssert(commandBuffer.buffer != VK_NULL_HANDLE, "gpuSubmit(): you passed in a null command buffer...");
    gpuAssert(vkEndCommandBuffer(commandBuffer.buffer) == VK_SUCCESS, "gpuSubmit(): failed to end command buffer recording");

    // Alright, which queue do I use?
    VkQueue targetQueue = VK_NULL_HANDLE;
    switch (commandBuffer.queueType) {
        case kGraphicsQueue: targetQueue = ggpu.graphicsPool.queue; break;
        case kComputeQueue: targetQueue = ggpu.computePool.queue; break;
        case kTransferQueue: targetQueue = ggpu.transferPool.queue; break;
    }

    VkTimelineSemaphoreSubmitInfo timeline = { 0 };
    timeline.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
    timeline.signalSemaphoreValueCount = 1;
    timeline.pSignalSemaphoreValues = &signalValue;

    // Try submitting stuff over...
    VkSubmitInfo submitInfo = { 0 };
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer.buffer;
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = &semaphore.semaphore;

    // VkFence fence = ggpu.stagingPool.stagingFence;
    gpuAssert(vkQueueSubmit(targetQueue, 1, &submitInfo, VK_NULL_HANDLE) == VK_SUCCESS,
        "gpuSubmit(): hardware driver rejected your command queue submission!");
}

// Obtains the current swapchain image
GpuImage gpuGetSwapchainImage() {
    return (GpuImage){ .index = GGPU_IMAGE_SWAPCHAIN_START + ggpu.swapchain.currentImageIndex };
}

// Allocates a writable image on the GPU that can be visible from a compute shader
uint32_t gpuCreateStorageImage(GpuImage image, uint32_t slotIndex) {
    VkPhysicalDeviceDescriptorHeapPropertiesEXT* props = &ggpu.heapProperties;
    VkDeviceSize imageSize = props->imageDescriptorSize;
    VkDeviceSize offset = props->minResourceHeapReservedRange + slotIndex * imageSize;
    uint8_t* heapCpu = (uint8_t*) ggpu.resourceHeap.cpuAddress + offset;

    GpuImageInternal* img = &ggpu.imagePool.images[image.index];

    VkImageViewCreateInfo viewInfo = { 0 };
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = img->image;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = img->format;
    viewInfo.subresourceRange.aspectMask = img->aspect;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = img->mips;
    viewInfo.subresourceRange.layerCount = 1;

    VkImageDescriptorInfoEXT imageDesc = { 0 };
    imageDesc.sType = VK_STRUCTURE_TYPE_IMAGE_DESCRIPTOR_INFO_EXT;
    imageDesc.pView = &viewInfo;
    imageDesc.layout = VK_IMAGE_LAYOUT_GENERAL;

    VkResourceDescriptorInfoEXT resourceInfo = { 0 };
    resourceInfo.sType = VK_STRUCTURE_TYPE_RESOURCE_DESCRIPTOR_INFO_EXT;
    resourceInfo.type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    resourceInfo.data.pImage = &imageDesc;

    VkHostAddressRangeEXT dest = { 0 };
    dest.address = (void*) heapCpu;
    dest.size = imageSize;
    _cmd_write_resource_descriptors(ggpu.device, 1, &resourceInfo, &dest);

    return (uint32_t)(offset / imageSize);
}

// Allocates an empty image on the GPU with the provided user description
GpuImage gpuCreateImage(GpuTextureDesc desc) {
    gpuAssert(desc.width > 0 && desc.height > 0, "gpuCreateImage(): width and height must be > 0!");
    gpuAssert(desc.colorFormat != VK_FORMAT_UNDEFINED, "gpuCreateImage(): format must not be undefined!");

    // determine aspect from format
    VkImageAspectFlags aspect = VK_IMAGE_ASPECT_COLOR_BIT;
    switch (desc.colorFormat) {
        case VK_FORMAT_D16_UNORM:
        case VK_FORMAT_D32_SFLOAT:
        case VK_FORMAT_D24_UNORM_S8_UINT:
            aspect = VK_IMAGE_ASPECT_DEPTH_BIT;
            break;
        default: break;
    }

    // create the image
    VkImageCreateInfo imageInfo = { 0 };
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.format = desc.colorFormat;
    imageInfo.extent.width = desc.width;
    imageInfo.extent.height = desc.height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = max(desc.mipLevels, 1);
    imageInfo.arrayLayers = 1;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.usage = desc.usageFlags;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    VkDeviceImageMemoryRequirements requirements = { 0 };
    requirements.sType = VK_STRUCTURE_TYPE_DEVICE_IMAGE_MEMORY_REQUIREMENTS;
    requirements.pCreateInfo = &imageInfo;
    requirements.planeAspect = _gpu_get_aspect_flags(imageInfo);

    VkMemoryRequirements2 memoryReqs = { 0 };
    memoryReqs.sType = VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2;

    VkImage image;
    gpuAssert(vkCreateImage(ggpu.device, &imageInfo, NULL, &image) == VK_SUCCESS,
        "gpuCreateImage(): vkCreateImage failed!");

    // allocate memory
    vkGetDeviceImageMemoryRequirements(ggpu.device, &requirements, &memoryReqs);

    VkMemoryAllocateInfo allocInfo = { 0 };
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memoryReqs.memoryRequirements.size;
    allocInfo.memoryTypeIndex = _gpu_find_memory_type(memoryReqs.memoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    VkDeviceMemory memory;
    gpuAssert(vkAllocateMemory(ggpu.device, &allocInfo, NULL, &memory) == VK_SUCCESS,
        "gpuCreateImage(): vkAllocateMemory failed!");
    gpuAssert(vkBindImageMemory(ggpu.device, image, memory, 0) == VK_SUCCESS,
        "gpuCreateImage(): vkBindImageMemory failed!");

    // create the view
    static VkImageViewCreateInfo viewInfo = { 0 };
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = image;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = desc.colorFormat;
    viewInfo.subresourceRange.aspectMask = aspect;
    viewInfo.subresourceRange.levelCount = imageInfo.mipLevels;
    viewInfo.subresourceRange.layerCount = 1;

    VkImageView view;
    gpuAssert(vkCreateImageView(ggpu.device, &viewInfo, NULL, &view) == VK_SUCCESS,
        "gpuCreateImage(): vkCreateImageView failed!");

    // claim a slot from the pool
    GpuImagePool* pool = &ggpu.imagePool;
    uint32_t slotId = 0;
    if (pool->freeSlotsCount > 0) {
        slotId = pool->freeSlots[--pool->freeSlotsCount];
    } else {
        gpuAssert(pool->poolSize < ggpu.maxTextures, "gpuCreateImage(): out of image pool slots!");
        slotId = pool->poolSize++;
    }

    GpuImageInternal* entry = &pool->images[slotId];
    entry->image = image;
    entry->view = view;
    entry->memory = memory;
    entry->mips = max(desc.mipLevels, 1);
    entry->format = desc.colorFormat;
    entry->extent = (VkExtent2D){ desc.width, desc.height };
    entry->aspect = aspect;
    entry->heapIndex = UINT32_MAX;

    if (desc.usageFlags & VK_IMAGE_USAGE_SAMPLED_BIT) {
        uint32_t heapSlot = ggpu.imagePool.poolSize - GGPU_IMAGE_USER_START;
        entry->heapIndex = heapSlot;

        VkDescriptorGetInfoEXT descriptorInfo = { 0 };
        descriptorInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_GET_INFO_EXT;
        descriptorInfo.type = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
        descriptorInfo.data.pSampledImage = &(VkDescriptorImageInfo){
            .imageView = view,
            .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        };
    }

    GpuCommandBuffer cmd = gpuStartCommandRecording(kGraphicsQueue);
    VkImageMemoryBarrier2 toGeneral = { 0 };
    toGeneral.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
    toGeneral.srcStageMask = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT;
    toGeneral.srcAccessMask = VK_ACCESS_2_NONE;
    toGeneral.dstStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
    toGeneral.dstAccessMask = VK_ACCESS_2_MEMORY_READ_BIT | VK_ACCESS_2_MEMORY_WRITE_BIT;
    toGeneral.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    toGeneral.newLayout = VK_IMAGE_LAYOUT_GENERAL;
    toGeneral.image = entry->image;
    toGeneral.subresourceRange.aspectMask = entry->aspect;
    toGeneral.subresourceRange.levelCount = 1;
    toGeneral.subresourceRange.layerCount = 1;

    VkDependencyInfo dep = { 0 };
    dep.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
    dep.imageMemoryBarrierCount = 1;
    dep.pImageMemoryBarriers = &toGeneral;
    vkCmdPipelineBarrier2(cmd.buffer, &dep);
    gpuSubmitBlocking(cmd);

    return (GpuImage){ .index = slotId, .active = true };
}

// Copies an image from source -> destination (use this for copying stuff over for like temporal stuff)
void gpuCopyImage(GpuCommandBuffer buffer, GpuImage source, GpuImage destination, VkOffset3D start, VkOffset3D end) {
    VkImageCopy region = { 0 };
    region.extent = (VkExtent3D) {
        .width = end.x - start.x,
        .height = end.y - start.y,
        .depth = end.z - start.z + 1
    };

    region.srcSubresource.aspectMask = _gpu_get_raw_image(source)->aspect;
    region.srcSubresource.mipLevel = 0;
    region.srcSubresource.baseArrayLayer = 0;
    region.srcSubresource.layerCount = 1;
    region.srcOffset = start;

    region.dstSubresource.aspectMask = _gpu_get_raw_image(destination)->aspect;
    region.dstSubresource.mipLevel = 0;
    region.dstSubresource.baseArrayLayer = 0;
    region.dstSubresource.layerCount = 1;
    region.dstOffset = start;

    vkCmdCopyImage(
        buffer.buffer,
        _gpu_get_raw_image(source)->image,
        VK_IMAGE_LAYOUT_GENERAL,
        _gpu_get_raw_image(destination)->image,
        VK_IMAGE_LAYOUT_GENERAL,
        1,
        &region
    );
}

// Begins an image upload batch (use this to optimize image uploading)
GpuUploadBatch gpuBeginImageBatch() {
    GpuUploadBatch batch = { 0 };
    batch.cmd = gpuStartCommandRecording(kGraphicsQueue);
    return batch;
}

// Uploads CPU mapped image data to the GPU
void gpuBatchUploadImage(GpuUploadBatch* batch, GpuImage dst, void* data, uint32_t bytes) {
    GpuImageInternal* img = _gpu_get_raw_image(dst);
    GpuPtr staging = gpuMallocAligned(bytes, ggpu.heapProperties.resourceHeapAlignment, kSharedMemory);
    memcpy(staging.cpuAddress, data, bytes);

    batch->stagingBuffers[batch->stagingCount] = staging;
    batch->images[batch->stagingCount] = dst;
    batch->stagingCount++;
}

// Commits all of the images onto GPU memory and stalls the CPU
void gpuEndImageBatch(GpuUploadBatch* batch) {
    // UNDEFINED -> GENERAL
    VkImageMemoryBarrier2 barriers[GGPU_MAX_BATCH_BUFFERS] = { 0 };
    for (uint32_t i = 0; i < batch->stagingCount; i++) {
        GpuImageInternal* img = _gpu_get_raw_image(batch->images[i]);
        barriers[i].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
        barriers[i].srcStageMask = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT;
        barriers[i].srcAccessMask = VK_ACCESS_2_NONE;
        barriers[i].dstStageMask = VK_PIPELINE_STAGE_2_COPY_BIT;
        barriers[i].dstAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
        barriers[i].oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        barriers[i].newLayout = VK_IMAGE_LAYOUT_GENERAL;
        barriers[i].image = img->image;
        barriers[i].subresourceRange.aspectMask = img->aspect;
        barriers[i].subresourceRange.levelCount = img->mips;
        barriers[i].subresourceRange.layerCount = 1;
    }

    VkDependencyInfo dep = { 0 };
    dep.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
    dep.imageMemoryBarrierCount = batch->stagingCount;
    dep.pImageMemoryBarriers = barriers;
    vkCmdPipelineBarrier2(batch->cmd.buffer, &dep);

    // Copy everything over...
    for (uint32_t i = 0; i < batch->stagingCount; i++) {
        GpuImageInternal* img = _gpu_get_raw_image(batch->images[i]);
        GpuBuffer* stagingBuf = &ggpu.stagingPool.allocationsPool[batch->stagingBuffers[i].allocationPoolId];

        VkBufferImageCopy region = { 0 };
        region.imageSubresource.aspectMask = img->aspect;
        region.imageSubresource.layerCount = 1;
        region.imageExtent.width = img->extent.width;
        region.imageExtent.height = img->extent.height;
        region.imageExtent.depth = 1;
        vkCmdCopyBufferToImage(batch->cmd.buffer, stagingBuf->buffer, img->image,
            VK_IMAGE_LAYOUT_GENERAL, 1, &region);
    }

    for (uint32_t i = 0; i < batch->stagingCount; i++) {
        GpuImageInternal* img = _gpu_get_raw_image(batch->images[i]);
        int32_t mipWidth = (int32_t)img->extent.width;
        int32_t mipHeight = (int32_t)img->extent.height;

        VkImageMemoryBarrier2 barrier0 = { 0 };
        barrier0.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
        barrier0.srcStageMask = VK_PIPELINE_STAGE_2_COPY_BIT | VK_PIPELINE_STAGE_2_TRANSFER_BIT;
        barrier0.srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
        barrier0.dstStageMask = VK_PIPELINE_STAGE_2_BLIT_BIT;
        barrier0.dstAccessMask = VK_ACCESS_2_TRANSFER_READ_BIT;
        barrier0.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
        barrier0.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier0.image = img->image;
        barrier0.subresourceRange.aspectMask = img->aspect;
        barrier0.subresourceRange.baseMipLevel = 0;
        barrier0.subresourceRange.levelCount = 1;
        barrier0.subresourceRange.baseArrayLayer = 0;
        barrier0.subresourceRange.layerCount = 1;

        VkDependencyInfo dep0 = { 0 };
        dep0.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
        dep0.imageMemoryBarrierCount = 1;
        dep0.pImageMemoryBarriers = &barrier0;
        vkCmdPipelineBarrier2(batch->cmd.buffer, &dep0);

        for (uint32_t mip = 1; mip < img->mips; mip++) {
            VkImageMemoryBarrier2 prepareDst = { 0 };
            prepareDst.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
            prepareDst.srcStageMask = VK_PIPELINE_STAGE_2_NONE;
            prepareDst.srcAccessMask = VK_ACCESS_2_NONE;
            prepareDst.dstStageMask = VK_PIPELINE_STAGE_2_BLIT_BIT;
            prepareDst.dstAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
            prepareDst.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            prepareDst.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            prepareDst.image = img->image;
            prepareDst.subresourceRange.aspectMask = img->aspect;
            prepareDst.subresourceRange.baseMipLevel = mip;
            prepareDst.subresourceRange.levelCount = 1;
            prepareDst.subresourceRange.baseArrayLayer = 0;
            prepareDst.subresourceRange.layerCount = 1;

            VkDependencyInfo depPrepare = { 0 };
            depPrepare.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
            depPrepare.imageMemoryBarrierCount = 1;
            depPrepare.pImageMemoryBarriers = &prepareDst;
            vkCmdPipelineBarrier2(batch->cmd.buffer, &depPrepare);

            VkImageBlit blit = { 0 };
            blit.srcSubresource.aspectMask = img->aspect;
            blit.srcSubresource.mipLevel = mip - 1;
            blit.srcSubresource.baseArrayLayer = 0;
            blit.srcSubresource.layerCount  = 1;
            blit.srcOffsets[0] = (VkOffset3D){ 0, 0, 0 };
            blit.srcOffsets[1] = (VkOffset3D){ mipWidth, mipHeight, 1 };

            blit.dstSubresource.aspectMask = img->aspect;
            blit.dstSubresource.mipLevel = mip;
            blit.dstSubresource.baseArrayLayer = 0;
            blit.dstSubresource.layerCount = 1;
            blit.dstOffsets[0] = (VkOffset3D){ 0, 0, 0 };
            blit.dstOffsets[1] = (VkOffset3D){
                mipWidth > 1 ? mipWidth / 2 : 1,
                mipHeight > 1 ? mipHeight / 2 : 1,
                1
            };

            vkCmdBlitImage(
                batch->cmd.buffer,
                img->image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                img->image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                1, &blit,
                VK_FILTER_LINEAR
            );

            VkImageMemoryBarrier2 dstToSrc = { 0 };
            dstToSrc.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
            dstToSrc.srcStageMask = VK_PIPELINE_STAGE_2_BLIT_BIT;
            dstToSrc.srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
            dstToSrc.dstStageMask = VK_PIPELINE_STAGE_2_BLIT_BIT;
            dstToSrc.dstAccessMask = VK_ACCESS_2_TRANSFER_READ_BIT;
            dstToSrc.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            dstToSrc.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            dstToSrc.image = img->image;
            dstToSrc.subresourceRange.aspectMask = img->aspect;
            dstToSrc.subresourceRange.baseMipLevel = mip;
            dstToSrc.subresourceRange.levelCount = 1;
            dstToSrc.subresourceRange.baseArrayLayer = 0;
            dstToSrc.subresourceRange.layerCount = 1;

            VkDependencyInfo depDstToSrc = { 0 };
            depDstToSrc.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
            depDstToSrc.imageMemoryBarrierCount = 1;
            depDstToSrc.pImageMemoryBarriers = &dstToSrc;
            vkCmdPipelineBarrier2(batch->cmd.buffer, &depDstToSrc);

            // Halve dimensions for next mip level
            if (mipWidth > 1) mipWidth /= 2;
            if (mipHeight > 1) mipHeight /= 2;
        }

        VkImageMemoryBarrier2 finalLayout = { 0 };
        finalLayout.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
        finalLayout.srcStageMask = VK_PIPELINE_STAGE_2_BLIT_BIT;
        finalLayout.srcAccessMask = VK_ACCESS_2_TRANSFER_READ_BIT;
        finalLayout.dstStageMask = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
        finalLayout.dstAccessMask = VK_ACCESS_2_SHADER_READ_BIT;
        finalLayout.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        finalLayout.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        finalLayout.image = img->image;
        finalLayout.subresourceRange.aspectMask = img->aspect;
        finalLayout.subresourceRange.baseMipLevel = 0;
        finalLayout.subresourceRange.levelCount = img->mips; // Covers all generated mips at once
        finalLayout.subresourceRange.baseArrayLayer = 0;
        finalLayout.subresourceRange.layerCount = 1;

        VkDependencyInfo depFinal = { 0 };
        depFinal.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
        depFinal.imageMemoryBarrierCount = 1;
        depFinal.pImageMemoryBarriers = &finalLayout;
        vkCmdPipelineBarrier2(batch->cmd.buffer, &depFinal);
    }

    // free all staging buffers now that GPU is done
    gpuSubmitBlocking(batch->cmd);
    for (uint32_t i = 0; i < batch->stagingCount; i++)
        gpuFreeImmediate(batch->stagingBuffers[i]);
}

// Helper utility that uploads a singular CPU mapped image `data` to the GPU
void gpuUploadImage(GpuImage dst, void* data, uint32_t bytes) {
    GpuUploadBatch batch = gpuBeginImageBatch();
    gpuBatchUploadImage(&batch, dst, data, bytes);
    gpuEndImageBatch(&batch);
}

// Equivalent of a gpuDraw, but you can specify the command buffer that is allocated on the GPU
// `data` is the push constant struct
// `offset` is the offset to read the command buffer,
// `drawCount` is the number of objects to draw (can be 0)
// `stride` is the size of each draw command
void gpuDrawIndirectCount(GpuPipeline* pipeline, GpuCommandBuffer commandBuffer, void* data,
    GpuPtr indirectBuffer, uint32_t offset, GpuPtr countBuffer, uint32_t countOffset,
    uint32_t maxDrawCount, uint32_t stride) {
    gpuAssert(commandBuffer.queueType == kGraphicsQueue, "gpuDrawIndirect(): command buffer provided is not in the draw queue!");

    VkCommandBuffer buffer = commandBuffer.buffer;

    // Bind and dispatch!
    vkCmdBindPipeline(buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->pipeline);

    VkPushDataInfoEXT pushInfo = { 0 };
    pushInfo.sType = VK_STRUCTURE_TYPE_PUSH_DATA_INFO_EXT;
    pushInfo.offset = 0;
    pushInfo.data.address = (const void*) data;
    pushInfo.data.size = pipeline->layoutSize;
    _cmd_push_data(buffer, &pushInfo);

    vkCmdDrawIndirectCount(
        buffer,
        _gpu_get_buffer(indirectBuffer).buffer, offset,
        _gpu_get_buffer(countBuffer).buffer, countOffset,
        maxDrawCount,
        stride
    );
}

// Equivalent of a gpuDraw, but you can specify the command buffer that is allocated on the GPU
// `data` is the push constant struct
// `offset` is the offset to read the command buffer,
// `drawCount` is the number of objects to draw (can be 0)
// `stride` is the size of each draw command
void gpuDrawIndirect(GpuPipeline* pipeline, GpuCommandBuffer commandBuffer, void* data,
    GpuPtr indirectBuffer, uint32_t offset, uint32_t drawCount, uint32_t stride) {
    gpuAssert(commandBuffer.queueType == kGraphicsQueue, "gpuDrawIndirect(): command buffer provided is not in the draw queue!");

    VkCommandBuffer buffer = commandBuffer.buffer;

    // Bind and dispatch!
    vkCmdBindPipeline(buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->pipeline);

    VkPushDataInfoEXT pushInfo = { 0 };
    pushInfo.sType = VK_STRUCTURE_TYPE_PUSH_DATA_INFO_EXT;
    pushInfo.offset = 0;
    pushInfo.data.address = (const void*) data;
    pushInfo.data.size = pipeline->layoutSize;
    _cmd_push_data(buffer, &pushInfo);

    vkCmdDrawIndirect(buffer, _gpu_get_buffer(indirectBuffer).buffer, offset, drawCount, stride);
}

// Invokes the hardware rasterizer.
// `data` is the push constant struct
void gpuDraw(GpuPipeline* pipeline, GpuCommandBuffer commandBuffer, void* data,
    uint32_t vertexCount, uint32_t instanceCount,
    uint32_t firstVertex, uint32_t firstInstance) {
    gpuAssert(commandBuffer.queueType == kGraphicsQueue, "gpuDraw(): command buffer provided is not in the draw queue!");

    VkCommandBuffer buffer = commandBuffer.buffer;

    // Bind and dispatch!
    vkCmdBindPipeline(buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->pipeline);

    VkPushDataInfoEXT pushInfo = { 0 };
    pushInfo.sType = VK_STRUCTURE_TYPE_PUSH_DATA_INFO_EXT;
    pushInfo.offset = 0;
    pushInfo.data.address = (const void*) data;
    pushInfo.data.size = pipeline->layoutSize;
    _cmd_push_data(buffer, &pushInfo);

    vkCmdDraw(buffer, vertexCount, instanceCount, firstVertex, firstInstance);
}

// Dispatches a compute shader!
// `data` is the push constant struct
void gpuDispatch(GpuPipeline* pipeline, GpuCommandBuffer commandBuffer, void* data, uint32_t x, uint32_t y, uint32_t z) {
    gpuAssert(commandBuffer.queueType == kComputeQueue || commandBuffer.queueType == kGraphicsQueue,
        "gpuDispatch(): command buffer provided is not in the compute queue!");

    VkCommandBuffer buffer = commandBuffer.buffer;

    // Bind and dispatch!
    vkCmdBindPipeline(buffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline->pipeline);

    VkPushDataInfoEXT pushInfo = { 0 };
    pushInfo.sType = VK_STRUCTURE_TYPE_PUSH_DATA_INFO_EXT;
    pushInfo.offset = 0;
    pushInfo.data.address = (const void*) data;
    pushInfo.data.size = pipeline->layoutSize;
    _cmd_push_data(buffer, &pushInfo);

    vkCmdDispatch(buffer, x, y, z);
}

// Dispatches a compute shader!
// `data` is the push constant struct
// `indirectBuffer` is the command buffer to write to
// `offset` offsets where the indirect buffer is read
void gpuDispatchIndirect(GpuPipeline* pipeline, GpuCommandBuffer commandBuffer, void* data, GpuPtr indirectBuffer, uint32_t offset) {
    gpuAssert(commandBuffer.queueType == kComputeQueue || commandBuffer.queueType == kGraphicsQueue,
        "gpuDispatchIndirect(): command buffer provided is not in the compute queue!");

    VkCommandBuffer buffer = commandBuffer.buffer;

    // Bind and dispatch!
    _gpu_bind_descriptor_heap(commandBuffer);
    vkCmdBindPipeline(buffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline->pipeline);

    VkPushDataInfoEXT pushInfo = { 0 };
    pushInfo.sType = VK_STRUCTURE_TYPE_PUSH_DATA_INFO_EXT;
    pushInfo.offset = 0;
    pushInfo.data.address = (const void*) data;
    pushInfo.data.size = pipeline->layoutSize;
    _cmd_push_data(buffer, &pushInfo);

    vkCmdDispatchIndirect(buffer, _gpu_get_buffer(indirectBuffer).buffer, (VkDeviceSize)offset);

    // We submit the command and wait!
    gpuSubmitBlocking(commandBuffer);
}

// Creates a sampler with the provided `desc` in `slotIndex` of the sampler heap
uint32_t gpuCreateSampler(GpuSamplerDesc desc, uint32_t slotIndex) {
    VkPhysicalDeviceDescriptorHeapPropertiesEXT* props = &ggpu.heapProperties;

    VkDeviceSize samplerSize = props->samplerDescriptorSize;
    VkDeviceSize offset = props->minSamplerHeapReservedRange + slotIndex * samplerSize;
    uint8_t* heapCpu = (uint8_t*) ggpu.samplerHeap.cpuAddress + offset;

    VkSamplerCreateInfo samplerInfo = { 0 };
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = (VkFilter) desc.magFilter;
    samplerInfo.minFilter = (VkFilter) desc.minFilter;
    samplerInfo.mipmapMode = (VkSamplerMipmapMode) desc.mipmapMode;
    samplerInfo.addressModeU = (VkSamplerAddressMode) desc.addressModeU;
    samplerInfo.addressModeV = (VkSamplerAddressMode) desc.addressModeV;
    samplerInfo.addressModeW = (VkSamplerAddressMode) desc.addressModeW;
    samplerInfo.anisotropyEnable = desc.maxAnisotropy != 1.0f;
    samplerInfo.borderColor = desc.borderColor;
    samplerInfo.maxAnisotropy = desc.maxAnisotropy;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = desc.maxLod;

    VkHostAddressRangeEXT dest = { 0 };
    dest.address = (void*) heapCpu;
    dest.size = samplerSize;

    gpuAssert(_cmd_write_sampler_descriptors(ggpu.device, 1, &samplerInfo, &dest) == VK_SUCCESS,
        "gpuCreateSampler(): failed to allocate sampler on sampler heap");

    return (uint32_t)(offset / samplerSize);
}

// Call this when you are done writing your image. This sets the `slotIndex` of the current
// texture heap to your specified `image`.
uint32_t gpuCreateTexture(GpuImage image, uint32_t slotIndex) {
    gpuAssert(image.active, "gpuCreateTexture(): this texture was not active!");
    VkPhysicalDeviceDescriptorHeapPropertiesEXT* props = &ggpu.heapProperties;

    VkDeviceSize imageSize = props->imageDescriptorSize;
    VkDeviceSize offset = props->minResourceHeapReservedRange + slotIndex * imageSize;

    uint8_t* heapCpu = (uint8_t*) ggpu.resourceHeap.cpuAddress + offset;
    GpuImageInternal* img = &ggpu.imagePool.images[image.index];

    VkImageViewCreateInfo viewInfo = { 0 };
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = img->image;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = img->format;
    viewInfo.subresourceRange.aspectMask = img->aspect;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = img->mips;
    viewInfo.subresourceRange.layerCount = 1;

    static VkImageDescriptorInfoEXT imageDesc = { 0 };
    imageDesc.sType = VK_STRUCTURE_TYPE_IMAGE_DESCRIPTOR_INFO_EXT;
    imageDesc.pView = &viewInfo;
    imageDesc.layout = VK_IMAGE_LAYOUT_GENERAL;

    VkResourceDescriptorInfoEXT resourceInfo = { 0 };
    resourceInfo.sType = VK_STRUCTURE_TYPE_RESOURCE_DESCRIPTOR_INFO_EXT;
    resourceInfo.type = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
    resourceInfo.data.pImage = &imageDesc;

    VkHostAddressRangeEXT dest = { 0 };
    dest.address = (void*) heapCpu;
    dest.size = imageSize;
    gpuAssert(_cmd_write_resource_descriptors(ggpu.device, 1, &resourceInfo, &dest) == VK_SUCCESS,
        "gpuCreateTexture(): failed to allocate texture on texture heap");

    return (uint32_t)(offset / imageSize);
}

// Reads `bytes` bytes from a GPU pointer and populates `destCpuData` with exactly `bytes` bytes
void gpuReadback(void* destCpuData, GpuPtr srcGpu, uint32_t bytes) {
    if (destCpuData == NULL)
        return;

    GpuStagingPool* pool = &ggpu.stagingPool;
    GpuBuffer* srcAlloc = &pool->allocationsPool[srcGpu.allocationPoolId];
    uintptr_t srcOffset = (uintptr_t)srcGpu.gpuAddress - (uintptr_t)srcAlloc->gpu;

    if (srcAlloc->memoryType == kSharedMemory || srcAlloc->memoryType == kCpuMemory) {
        gpuAssert(srcAlloc->cpu != NULL, "gpuReadback(): host visible buffer has null cpu mapping!");

        if (srcAlloc->memoryType == kCpuMemory) {
            VkMappedMemoryRange range = { 0 };
            range.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
            range.memory = srcAlloc->memory;
            range.offset = srcOffset;
            range.size = bytes;
            vkInvalidateMappedMemoryRanges(ggpu.device, 1, &range);
        }

        memcpy(destCpuData, (uint8_t*)srcAlloc->cpu + srcOffset, bytes);
        return;
    }

    // Create a staging buffer to copy to...
    GpuPtr stagingPtr = gpuMalloc(bytes, kCpuMemory);
    GpuBuffer* stagingAlloc = &pool->allocationsPool[stagingPtr.allocationPoolId];

    GpuCommandBuffer cmd = gpuStartCommandRecording(kTransferQueue);
    VkBufferCopy region = { 0 };
    region.srcOffset = srcOffset;
    region.dstOffset = 0;
    region.size = bytes;
    vkCmdCopyBuffer(cmd.buffer, srcAlloc->buffer, stagingAlloc->buffer, 1, &region);
    gpuSubmitBlocking(cmd);

    // Invalidate staging buffer cache before reading
    VkMappedMemoryRange range = { 0 };
    range.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
    range.memory = stagingAlloc->memory;
    range.offset = 0;
    range.size = bytes;
    vkInvalidateMappedMemoryRanges(ggpu.device, 1, &range);

    memcpy(destCpuData, stagingAlloc->cpu, bytes);
    gpuFreeImmediate(stagingPtr);
}

// Helper function that creates an upload batch object (used to combine allocations)
GpuUploadBatch gpuBeginUploadBatch() {
    GpuUploadBatch batch = { 0 };
    batch.cmd = gpuStartCommandRecording(kGraphicsQueue);
    return batch;
}

// Adds an already allocated gpu pointer `dst` to be batch copied from the staging `data` buffer.
// The allocated region's length must be >= `bytes`
void gpuBatchUploadBuffer(GpuUploadBatch* batch, GpuPtr dst, void* data, uint32_t bytes) {
    if (bytes == 0) return;

    GpuPtr staging = gpuMalloc(bytes, kSharedMemory);
    memcpy(staging.cpuAddress, data, bytes);
    gpuAssert(batch->stagingCount < GGPU_MAX_BATCH_BUFFERS, "gpuBatchUploadBuffer(): batched too many things!");

    batch->stagingBuffers[batch->stagingCount++] = staging;

    GpuBuffer* stagingBuf = &ggpu.stagingPool.allocationsPool[staging.allocationPoolId];
    GpuBuffer* dstBuf = &ggpu.stagingPool.allocationsPool[dst.allocationPoolId];

    VkBufferCopy region = { 0 };
    region.srcOffset = 0;
    region.dstOffset = 0;
    region.size = bytes;

    vkCmdCopyBuffer(batch->cmd.buffer, stagingBuf->buffer, dstBuf->buffer, 1, &region);
}

// Stops the upload batch and frees it at the end
void gpuEndUploadBatch(GpuUploadBatch* batch) {
    gpuSubmitBlocking(batch->cmd);
    for (uint32_t i = 0; i < batch->stagingCount; i++)
        gpuFreeImmediate(batch->stagingBuffers[i]);
}

// Helper function that automatically uploads CPU mapped data to a GPU mapped `dst` pointer
void gpuUploadBuffer(GpuPtr dst, void* data, uint32_t bytes) {
    GpuUploadBatch batch = gpuBeginUploadBatch();
    gpuBatchUploadBuffer(&batch, dst, data, bytes);
    gpuEndUploadBatch(&batch);
}

// Copies `bytes` bytes starting from `src` to `dest`.
// `src` must be a valid pointer on the gpu
// `dest` must be a valid pointer on the gpu
void gpuMemcpy(GpuPtr dest, GpuPtr src, uint32_t bytes) {
    GpuCommandBuffer cmdBuffer = gpuStartCommandRecording(kTransferQueue);
    gpuAssert(bytes != 0, "gpuMemcpy(): cannot memcpy nothing!");

    // Grab what buffer we use
    GpuStagingPool* pool = &ggpu.stagingPool;
    GpuBuffer* destAlloc = &pool->allocationsPool[dest.allocationPoolId];
    GpuBuffer* srcAlloc = &pool->allocationsPool[src.allocationPoolId];

    // Get the offsets for the command copy
    uintptr_t destOffset = (uintptr_t)dest.gpuAddress - (uintptr_t)destAlloc->gpu;
    uintptr_t srcOffset = (uintptr_t)src.gpuAddress - (uintptr_t)srcAlloc->gpu;

    // Make sure that we're not copying over too much!
    uintptr_t toCopy = 0;
    if (srcOffset < srcAlloc->bytes && destOffset < destAlloc->bytes) {
        uintptr_t srcRemaining = srcAlloc->bytes - srcOffset;
        uintptr_t destRemaining = destAlloc->bytes - destOffset;
        uintptr_t minRemaining = (srcRemaining < destRemaining) ? srcRemaining : destRemaining;
        toCopy = ((uintptr_t)bytes < minRemaining) ? (uintptr_t)bytes : minRemaining;
    }

    // Looks like we would've crashed!
    if (toCopy == 0) return;

    VkBufferCopy region = { 0 };
    region.srcOffset = srcOffset;
    region.dstOffset = destOffset;
    region.size = toCopy;

    vkCmdCopyBuffer(cmdBuffer.buffer, srcAlloc->buffer, destAlloc->buffer, 1, &region);
    gpuSubmitBlocking(cmdBuffer);
}

// Automatically changes pipeline state to rendering
// `gpuBeginFrame` must be called before this
void gpuBeginRenderPass(GpuCommandBuffer cmd, GpuRenderPassDesc desc) {
    // Batch all barriers into one call
    VkImageMemoryBarrier2 barriers[GGPU_MAX_COLOR_TARGETS + 1] = { 0 };

    // build rendering attachments
    VkRenderingAttachmentInfo colorAttachments[GGPU_MAX_COLOR_TARGETS] = { 0 };
    uint32_t colorTargetCount = 0;

    for (uint32_t i = 0; i < GGPU_MAX_COLOR_TARGETS; i++) {
        GpuColorAttachment* t = &desc.colorTargets[i];
        if (t->image.index == 0) break;

        GpuImageInternal* img = _gpu_get_raw_image(t->image);
        colorTargetCount++;

        VkRenderingAttachmentInfo attachmentInfo = { 0 };
        attachmentInfo.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
        attachmentInfo.imageView = img->view;
        attachmentInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
        attachmentInfo.loadOp = t->loadOp == kClear ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD;
        attachmentInfo.storeOp = t->storeOp == kStore ? VK_ATTACHMENT_STORE_OP_STORE : VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachmentInfo.clearValue.color = t->clearColor;

        colorAttachments[i] = attachmentInfo;
    }

    VkRenderingAttachmentInfo depthAttachment = { 0 };
    if (desc.depthTarget.image.index != 0) {
        GpuImageInternal* img = _gpu_get_raw_image(desc.depthTarget.image);

        depthAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
        depthAttachment.imageView = img->view;
        depthAttachment.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
        depthAttachment.loadOp = desc.depthTarget.loadOp == kClear ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD;
        depthAttachment.storeOp = desc.depthTarget.storeOp == kStore ? VK_ATTACHMENT_STORE_OP_STORE : VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depthAttachment.clearValue.depthStencil.depth = desc.depthTarget.clearDepth;
    }

    VkRenderingInfo renderingInfo = { 0 };
    renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
    renderingInfo.renderArea.extent = colorTargetCount > 0
        ? _gpu_get_raw_image(desc.colorTargets[0].image)->extent
        : _gpu_get_raw_image(desc.depthTarget.image)->extent;

    renderingInfo.layerCount = 1;
    renderingInfo.colorAttachmentCount = colorTargetCount;
    renderingInfo.pColorAttachments = colorAttachments;
    renderingInfo.pDepthAttachment = desc.depthTarget.image.index != 0 ? &depthAttachment : NULL;

    VkExtent2D extents = renderingInfo.renderArea.extent;
    VkViewport vp = { 0 };
    gpuAssert(extents.width > 0 && extents.height > 0,
        "gpuBeginRenderPass(): zero extent! colorTargetCount=%u depthIndex=%u",
        colorTargetCount, desc.depthTarget.image.index);
    vp.width = (float)extents.width;
    vp.height = (float)extents.height;
    vp.minDepth = 0.0f;
    vp.maxDepth = 1.0f;

    VkRect2D scissor = { 0 };
    scissor.extent = extents;

    vkCmdSetViewport(cmd.buffer, 0, 1, &vp);
    vkCmdSetScissor(cmd.buffer, 0, 1, &scissor);
    vkCmdBeginRendering(cmd.buffer, &renderingInfo);
}

// Ends the current render pass
// Make sure that this is called after `gpuBeginRenderPass`
void gpuEndRenderPass(GpuCommandBuffer cmd) {
    vkCmdEndRendering(cmd.buffer);
}

// Creates a graphics pipeline
// `graphicsPushSize` is the number of bytes of the dedicated push constant (you must send this over on draw!)
// `vertexIr` should be obtained by `gpuLoadShader` and must not be used in a pipeline already
// `fragmentIr` should be obtained by `gpuLoadShader` and must not be used in a pipeline already
GpuPipeline gpuCreateGraphicsPipeline(
    GpuGraphicsDesc rasterDescription,
    uint32_t graphicsPushSize,
    GpuShader vertexIr,
    GpuShader fragmentIr
) {
    VkPipelineShaderStageCreateInfo stageInfos[2] = { 0 };
    stageInfos[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stageInfos[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    stageInfos[0].module = vertexIr.module;
    stageInfos[0].pName = "main";
    stageInfos[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stageInfos[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    stageInfos[1].module = fragmentIr.module;
    stageInfos[1].pName = "main";

    // Look ma, no bindings!
    VkPipelineVertexInputStateCreateInfo vertexInputInfo = { 0 };
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

    VkPrimitiveTopology topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    switch (rasterDescription.topology) {
        case kTriangleTopology: topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST; break;
        case kFanTopology: topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN; break;
    }

    VkPipelineInputAssemblyStateCreateInfo assemblyCreateInfo = { 0 };
    assemblyCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    assemblyCreateInfo.topology = topology;

    VkPipelineViewportStateCreateInfo viewportStateCreateInfo = { 0 };
    viewportStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportStateCreateInfo.viewportCount = 1;
    viewportStateCreateInfo.scissorCount = 1;

    // Probably not really necessary but yeah
    VkPipelineRasterizationStateCreateInfo rasterizationStateCreateInfo = { 0 };
    rasterizationStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizationStateCreateInfo.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizationStateCreateInfo.lineWidth = 1.0f;
    rasterizationStateCreateInfo.depthBiasEnable = rasterDescription.depthBiasEnabled;
    rasterizationStateCreateInfo.depthBiasConstantFactor = rasterDescription.depthBiasConstant;
    rasterizationStateCreateInfo.depthBiasSlopeFactor = rasterDescription.depthBiasSlopeFactor;

    switch (rasterDescription.cull) {
        case kNoCulling: rasterizationStateCreateInfo.cullMode = VK_CULL_MODE_NONE; break;
        case kBackCulling: rasterizationStateCreateInfo.cullMode = VK_CULL_MODE_BACK_BIT; break;
        case kFrontCulling: rasterizationStateCreateInfo.cullMode = VK_CULL_MODE_FRONT_BIT; break;
    }

    switch (rasterDescription.frontFace) {
        case kClockwiseFrontFace: rasterizationStateCreateInfo.frontFace = VK_FRONT_FACE_CLOCKWISE; break;
        case kCounterClockwiseFrontFace: rasterizationStateCreateInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE; break;
    }

    VkSampleCountFlagBits sampleCount = VK_SAMPLE_COUNT_1_BIT;
    if (rasterDescription.sampleCount > 1) {
        sampleCount = (VkSampleCountFlagBits) rasterDescription.sampleCount;
    }

    VkPipelineMultisampleStateCreateInfo multisampleState = { 0 };
    multisampleState.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampleState.rasterizationSamples = sampleCount;
    multisampleState.alphaToCoverageEnable = rasterDescription.alphaCoverage ? VK_TRUE : VK_FALSE;

    // if caller provides blend state use it, otherwise default to opaque write
    VkPipelineColorBlendStateCreateInfo colorBlendInfo = { 0 };
    uint32_t numBlendStates = 0;
    colorBlendInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlendInfo.attachmentCount = 0;

    if (rasterDescription.blendStates != NULL) {
        for (int i = 0; i < GGPU_MAX_COLOR_TARGETS; i++) {
            if (rasterDescription.blendStates[i].colorWriteMask != 0) numBlendStates++;
            else break;
        }

        VkPipelineColorBlendAttachmentState* states = calloc(numBlendStates, sizeof(VkPipelineColorBlendAttachmentState));
        const VkBlendFactor factors[] = { VK_BLEND_FACTOR_ZERO, VK_BLEND_FACTOR_ONE, VK_BLEND_FACTOR_SRC_COLOR, VK_BLEND_FACTOR_DST_COLOR, VK_BLEND_FACTOR_SRC_ALPHA, VK_BLEND_FACTOR_DST_ALPHA };
        const VkBlendOp ops[] = { VK_BLEND_OP_ADD, VK_BLEND_OP_SUBTRACT, VK_BLEND_OP_REVERSE_SUBTRACT, VK_BLEND_OP_MIN, VK_BLEND_OP_MAX };

        for (int i = 0; i < numBlendStates; i++) {
            GpuBlendDesc b = rasterDescription.blendStates[i];
            VkPipelineColorBlendAttachmentState blend = { 0 };
            blend.blendEnable = VK_FALSE;
            blend.srcColorBlendFactor = factors[b.srcColorFactor];
            blend.dstColorBlendFactor = factors[b.dstColorFactor];
            blend.colorBlendOp = ops[b.colorOp];
            blend.srcAlphaBlendFactor = factors[b.srcAlphaFactor];
            blend.dstAlphaBlendFactor = factors[b.dstAlphaFactor];
            blend.alphaBlendOp = ops[b.alphaOp];
            blend.colorWriteMask = b.colorWriteMask;

            states[i] = blend;
        }

        colorBlendInfo.attachmentCount = numBlendStates;
        colorBlendInfo.pAttachments = states;
    }

    VkDynamicState dynamicStates[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
    VkPipelineDynamicStateCreateInfo dynamicState = { 0 };
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = 2;
    dynamicState.pDynamicStates = dynamicStates;

    // Depth test is driven by whether a depth format is provided
    VkFormat depthFormat = rasterDescription.depthFormat;
    VkPipelineDepthStencilStateCreateInfo depthStencilState = { 0 };
    depthStencilState.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencilState.depthTestEnable = depthFormat != VK_FORMAT_UNDEFINED ? VK_TRUE : VK_FALSE;
    depthStencilState.depthWriteEnable = depthFormat != VK_FORMAT_UNDEFINED ? VK_TRUE : VK_FALSE;
    depthStencilState.depthCompareOp = rasterDescription.depthCompareOp;

    uint32_t numColorTargets = 0;
    for (uint32_t i = 0; i < GGPU_MAX_COLOR_TARGETS; i++) {
        if (rasterDescription.colorTargets[i] != 0) numColorTargets++;
        else break;
    }

    gpuAssert(numColorTargets == numBlendStates, "gpuCreateGraphicsPipeline(): got %d color targets but only %d blend states were specified",
        numColorTargets, numBlendStates);

    // Use dynamic rendering :D
    VkPipelineRenderingCreateInfo renderInfo = { 0 };
    renderInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR;
    renderInfo.colorAttachmentCount = numBlendStates;
    renderInfo.pColorAttachmentFormats = rasterDescription.colorTargets;
    renderInfo.depthAttachmentFormat = depthFormat;

    VkPipelineCreateFlags2CreateInfoKHR heapFlags = { 0 };
    heapFlags.sType = VK_STRUCTURE_TYPE_PIPELINE_CREATE_FLAGS_2_CREATE_INFO_KHR;
    heapFlags.flags = VK_PIPELINE_CREATE_2_DESCRIPTOR_HEAP_BIT_EXT;
    heapFlags.pNext = &renderInfo;

    VkGraphicsPipelineCreateInfo pipelineInfo = { 0 };
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.pNext = &heapFlags;
    pipelineInfo.flags = 0;
    pipelineInfo.stageCount = 2;
    pipelineInfo.layout = VK_NULL_HANDLE;
    pipelineInfo.pStages = stageInfos;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &assemblyCreateInfo;
    pipelineInfo.pViewportState = &viewportStateCreateInfo;
    pipelineInfo.pRasterizationState = &rasterizationStateCreateInfo;
    pipelineInfo.pMultisampleState = &multisampleState;
    pipelineInfo.pColorBlendState = &colorBlendInfo;
    pipelineInfo.pDepthStencilState = &depthStencilState;
    pipelineInfo.pDynamicState = &dynamicState;

    VkPipeline graphicsPipeline;
    gpuAssert(vkCreateGraphicsPipelines(ggpu.device, VK_NULL_HANDLE, 1, &pipelineInfo, NULL, &graphicsPipeline) == VK_SUCCESS,
        "gpuCreateGraphicsPipeline(): failed to create graphics pipeline!");

    vkDestroyShaderModule(ggpu.device, vertexIr.module, NULL);
    vkDestroyShaderModule(ggpu.device, fragmentIr.module, NULL);

    if (numBlendStates != 0) free((void*) colorBlendInfo.pAttachments);

    GpuPipeline pipeline = { 0 };
    pipeline.pipeline = graphicsPipeline;
    pipeline.pipelineType = kGraphicsPipeline;
    pipeline.layoutSize = graphicsPushSize;
    pipeline.layout = VK_NULL_HANDLE;

    return pipeline;
}

// Destroys the specified pipeline
void gpuDestroyPipeline(GpuPipeline pipeline) {
    vkDeviceWaitIdle(ggpu.device);
    if (pipeline.pipelineType == kComputePipeline)
        vkDestroyPipelineLayout(ggpu.device, pipeline.layout, NULL);
    vkDestroyPipeline(ggpu.device, pipeline.pipeline, NULL);
}

// Creates a compute pipeline.
// `computePushSize` is the number of bytes of the dedicated push constant (you must send this over on compute!)
// `computeIr` should be obtained by `gpuLoadShader` and must not be used in a pipeline already
GpuPipeline gpuCreateComputePipeline(uint32_t computePushSize, GpuShader computeIr) {
    VkPushConstantRange pushConstantRange = { 0 };
    pushConstantRange.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    pushConstantRange.offset = 0;
    pushConstantRange.size = computePushSize;

    VkPipelineCreateFlags2CreateInfoKHR heapFlags = { 0 };
    heapFlags.sType = VK_STRUCTURE_TYPE_PIPELINE_CREATE_FLAGS_2_CREATE_INFO_KHR;
    heapFlags.flags = VK_PIPELINE_CREATE_2_DESCRIPTOR_HEAP_BIT_EXT;

    VkComputePipelineCreateInfo pipelineInfo = { 0 };
    pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    pipelineInfo.layout = VK_NULL_HANDLE;
    pipelineInfo.pNext = &heapFlags;
    pipelineInfo.stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    pipelineInfo.stage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    pipelineInfo.stage.module = computeIr.module;
    pipelineInfo.stage.pName = "main";

    VkPipeline computePipeline;
    gpuAssert(vkCreateComputePipelines(ggpu.device, VK_NULL_HANDLE, 1, &pipelineInfo, NULL, &computePipeline) == VK_SUCCESS,
        "gpuCreateComputePipeline(): failed to create compute pipeline!");

    vkDestroyShaderModule(ggpu.device, computeIr.module, NULL);

    GpuPipeline pipeline = { 0 };
    pipeline.pipeline = computePipeline;
    pipeline.pipelineType = kComputePipeline;
    pipeline.layoutSize = computePushSize;

    return pipeline;
}

// Destroys the entire GGPU context and frees up all allocated objects
void gpuDestroy() {
    // Wait for fences for like a second
    for (uint32_t i = 0; i < GGPU_FRAMES_IN_FLIGHT; i++) {
        if (ggpu.swapchain.fences[i] != VK_NULL_HANDLE)
            vkWaitForFences(ggpu.device, 1, &ggpu.swapchain.fences[i], VK_TRUE, 1000000000ULL);
    }

    if (ggpu.stagingPool.stagingFence != VK_NULL_HANDLE)
        vkWaitForFences(ggpu.device, 1, &ggpu.stagingPool.stagingFence, VK_TRUE, 1000000000ULL);

    vkDeviceWaitIdle(ggpu.device);
    for (uint32_t i = GGPU_IMAGE_USER_START; i < ggpu.imagePool.poolSize; i++) {
        GpuImageInternal* img = &ggpu.imagePool.images[i];
        if (img->image == VK_NULL_HANDLE) continue;

        vkDestroyImageView(ggpu.device, img->view, NULL);
        vkDestroyImage(ggpu.device, img->image, NULL);
        vkFreeMemory(ggpu.device, img->memory, NULL);
    }

    // Free heap buffers
    if (ggpu.resourceHeap.allocationPoolId != 0) gpuFreeImmediate(ggpu.resourceHeap);
    if (ggpu.samplerHeap.allocationPoolId != 0) gpuFreeImmediate(ggpu.samplerHeap);

    // Free any remaining buffers in the staging pool!!
    for (uint32_t i = 0; i < ggpu.stagingPool.allocPoolSize; i++) {
        GpuBuffer* buf = &ggpu.stagingPool.allocationsPool[i];
        if (buf->buffer == VK_NULL_HANDLE) continue;
        if (buf->cpu != NULL)
            vkUnmapMemory(ggpu.device, buf->memory);
        vkDestroyBuffer(ggpu.device, buf->buffer, NULL);
        vkFreeMemory(ggpu.device, buf->memory, NULL);
    }

    // Staging fence
    vkDestroyFence(ggpu.device, ggpu.stagingPool.stagingFence, NULL);

    // Command pools
    for (uint32_t i = 0; i < GGPU_FRAMES_IN_FLIGHT; i++) {
        vkDestroyCommandPool(ggpu.device, ggpu.graphicsPool.commandPools[i], NULL);
        vkDestroyCommandPool(ggpu.device, ggpu.computePool.commandPools[i], NULL);
        vkDestroyCommandPool(ggpu.device, ggpu.transferPool.commandPools[i], NULL);
    }

    vkDestroyDevice(ggpu.device, NULL);
    vkDestroyInstance(ggpu.instance, NULL);

#ifdef GGPU_VALIDATION
    printf("gpuDestroy(): cleaned up!\n");
#endif
}

// Recreates the swapchain on resize
void gpuRecreateSwapchain() {
    // Stall on minimized window
    VkSurfaceCapabilitiesKHR caps;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(ggpu.physicalDevice,
        ggpu.swapchain.surface, &caps);

    if (caps.currentExtent.width == 0 || caps.currentExtent.height == 0)
        return;

    vkDeviceWaitIdle(ggpu.device);

    for (uint32_t i = 0; i < GGPU_FRAMES_IN_FLIGHT; i++) {
        vkDestroySemaphore(ggpu.device, ggpu.swapchain.inFlight[i], NULL);
        ggpu.swapchain.inFlight[i] = VK_NULL_HANDLE;
        vkDestroyFence(ggpu.device, ggpu.swapchain.fences[i], NULL);
        ggpu.swapchain.fences[i] = VK_NULL_HANDLE;
    }

    for (uint32_t i = 0; i < GGPU_MAX_SWAPCHAIN_IMAGES; i++) {
        vkDestroySemaphore(ggpu.device, ggpu.swapchain.renderFinished[i], NULL);
        ggpu.swapchain.renderFinished[i] = VK_NULL_HANDLE;
    }

    gpuCreateSwapchain(ggpu.swapchainCallback);
    ggpu.swapchain.resized = false;
}

// Call this at the end of rendering
void gpuEndFrame(GpuCommandBuffer cmd) {
    GpuSwapchain* sc = &ggpu.swapchain;
    uint32_t frame = ggpu.currentFrame % GGPU_FRAMES_IN_FLIGHT;
    uint32_t image = sc->currentImageIndex;
    GpuImageInternal* scImg = &ggpu.imagePool.images[GGPU_IMAGE_SWAPCHAIN_START + image];

    // COLOR_ATTACHMENT_OPTIMAL -> PRESENT_SRC_KHR
    VkImageMemoryBarrier2 toPresent = { 0 };
    toPresent.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
    toPresent.srcStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
    toPresent.srcAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
    toPresent.dstStageMask = VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT;
    toPresent.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
    toPresent.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    toPresent.image = sc->images[image];
    toPresent.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    toPresent.subresourceRange.levelCount = 1;
    toPresent.subresourceRange.layerCount = 1;

    VkDependencyInfo dep = { 0 };
    dep.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
    dep.imageMemoryBarrierCount = 1;
    dep.pImageMemoryBarriers = &toPresent;

    vkCmdPipelineBarrier2(cmd.buffer, &dep);
    vkEndCommandBuffer(cmd.buffer);

    VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    VkSubmitInfo submit = { 0 };
    submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit.waitSemaphoreCount = 1;
    submit.pWaitSemaphores = &sc->inFlight[frame];
    submit.pWaitDstStageMask = &waitStage;
    submit.commandBufferCount = 1;
    submit.pCommandBuffers = &cmd.buffer;
    submit.signalSemaphoreCount = 1;
    submit.pSignalSemaphores = &sc->renderFinished[image];

    VkResult result = vkQueueSubmit(ggpu.graphicsPool.queue, 1, &submit, sc->fences[frame]);
    gpuAssert(result == VK_SUCCESS,
        "gpuEndFrame(): vkQueueSubmit failed! got code = %d", result);

    VkPresentInfoKHR present = { 0 };
    present.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    present.waitSemaphoreCount = 1;
    present.pWaitSemaphores = &sc->renderFinished[image];
    present.swapchainCount = 1;
    present.pSwapchains = &sc->rawSwapchain;
    present.pImageIndices = &image;

    VkResult pres = vkQueuePresentKHR(ggpu.graphicsPool.queue, &present);
    if (pres == VK_ERROR_OUT_OF_DATE_KHR || pres == VK_SUBOPTIMAL_KHR || ggpu.swapchain.resized)
        gpuRecreateSwapchain();
    else
        gpuAssert(pres == VK_SUCCESS, "gpuEndFrame(): vkQueuePresentKHR failed!");

    vkWaitForFences(ggpu.device, 1, &sc->fences[frame], VK_TRUE, UINT64_MAX);
    ggpu.currentFrame++;
}

// Call this before any other rendering function.
// Automatically calls `gpuFlushDeletion`
GpuCommandBuffer gpuBeginFrame(float r, float g, float b, float a) {
    gpuFlushDeletion();

    GpuSwapchain* sc = &ggpu.swapchain;
    uint32_t frame = ggpu.currentFrame % GGPU_FRAMES_IN_FLIGHT;

    // Wait for this slot's previous frame to finish
    vkWaitForFences(ggpu.device, 1, &sc->fences[frame], VK_TRUE, UINT64_MAX);

    // Acquire next swapchain image
    uint32_t imageIndex;
    VkResult acq = vkAcquireNextImageKHR(ggpu.device, sc->rawSwapchain,
        UINT64_MAX, sc->inFlight[frame], VK_NULL_HANDLE, &imageIndex);

    if (acq == VK_ERROR_OUT_OF_DATE_KHR) {
        gpuRecreateSwapchain();
        GpuCommandBuffer buff = { 0 };
        return buff;
    }

    gpuAssert(acq == VK_SUCCESS || acq == VK_SUBOPTIMAL_KHR,
        "gpuBeginFrame(): vkAcquireNextImageKHR failed!");

    vkResetFences(ggpu.device, 1, &sc->fences[frame]);
    ggpu.swapchain.currentImageIndex = imageIndex;

    vkResetCommandPool(ggpu.device, ggpu.graphicsPool.commandPools[frame], 0);
    ggpu.graphicsPool.commandQueue[frame] = 0;

    GpuCommandBuffer cmd = gpuStartCommandRecording(kGraphicsQueue);

    VkImageMemoryBarrier2 toGeneral = { 0 };
    toGeneral.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
    toGeneral.srcStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
    toGeneral.srcAccessMask = VK_ACCESS_2_NONE;
    toGeneral.dstStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
    toGeneral.dstAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
    toGeneral.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    toGeneral.newLayout = VK_IMAGE_LAYOUT_GENERAL;
    toGeneral.image = sc->images[imageIndex];
    toGeneral.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    toGeneral.subresourceRange.levelCount = 1;
    toGeneral.subresourceRange.layerCount = 1;

    VkDependencyInfo dep = { 0 };
    dep.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
    dep.imageMemoryBarrierCount = 1;
    dep.pImageMemoryBarriers = &toGeneral;
    vkCmdPipelineBarrier2(cmd.buffer, &dep);
    _gpu_bind_descriptor_heap(cmd);

    return cmd;
}

#endif
#endif