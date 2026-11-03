# ggpu
* vulkan thin wrapper based off of sebastian altonen's [no graphics api](https://sebastianaaltonen.com/blog/no-graphics-api)
* only dependency is vulkan
* *hopefully it's a clean, nice looking C library? :3*

## requirements
* needs relatively new gpu hardware and drivers, particularly from using descriptor heaps and untyped shader pointers
* uses the raw windows api as of right now, might do linux soon?

## features
* custom vulkan gpu memory allocation
* uses pointer objects for shaders (i'm currently using glsl, but you can use slang too)
* dynamic rendering
* unified image layouts
* descriptor heap textures + sampler
* compute pipeline
* graphics pipeline
* double buffering
* win32 windowing + support
* opaque pointer types (also a difference between cpu/gpu mapped stuff, so that reduces another class of memory errors)
* minimalist api surface without compromising much on speed and flexibility

## performance
* performance wise seems the same as a typical rendering setup, mostly through analysis on renderdoc and such
* i'll port over the vulkan examples soon though lol

## notice
* this is still a work in progress, so think of this as a wip. API should be consistent though :)
* also, i'm still fairly new to Vulkan in general, so pull requests fixing the api are well appreciated!
* no ai at all! so please don't do ai contributions

## usage
```c
// put this in some compilation unit, works
// the same as the stb headers, since the library
// is header only
#define GGPU_IMPLEMENTATION
#include "ggpu/ggpu_windows.h"
```

## code example
```c

static void windowCallback(UINT msg, WPARAM kind, LPARAM arg) { ... }
static void createSwapchain(int width, int height) { ... }

// ...

// Initialization :3
gpuCreateWindow("textured triangle", 1072, 1072, windowCallback);
gpuInit();
gpuCreateSurface();
gpuAllocateHeaps(512, 16);
gpuCreateSwapchain(createSwapchain);

// Construct image and IR
GpuImage image = loadImage("hihi.png");
GpuShader vertIr = gpuLoadShader("shaders/triangle.vert.spv", kVertexShader);
GpuShader fragIr = gpuLoadShader("shaders/triangle.frag.spv", kFragmentShader);

GpuPipeline pipeline = gpuCreateGraphicsPipeline((GpuGraphicsDesc) {
    .topology = kTriangleTopology,
    .cull = kNoCulling,
    .frontFace = kCounterClockwiseFrontFace,
    .colorTargets = { ggpu.swapchain.format },
}, sizeof(GfxPushConstants), vertIr, fragIr);

// Upload buffers to vertex pull from
GpuPtr positionBuffer = gpuMalloc(sizeof(kPositions), kSharedMemory);
GpuPtr colorBuffer = gpuMalloc(sizeof(kColors), kSharedMemory);
memcpy(positionBuffer.cpuAddress, kPositions, sizeof(kPositions));
memcpy(colorBuffer.cpuAddress, kColors, sizeof(kColors));

// Upload sampler and texture
GfxPushConstants push = {
    .vertices = positionBuffer.gpuAddress,
    .colors = colorBuffer.gpuAddress,
    .albedoIdx = gpuCreateTexture(image, 0),
    .samplerIdx = gpuCreateSampler((GpuSamplerDesc) {
        .magFilter = kLinearFilter,
        .minFilter = kLinearFilter,
        .mipmapMode = kLinearMipmap,
        .addressModeU = kRepeatMode,
        .addressModeV = kRepeatMode,
        .addressModeW = kRepeatMode,
        .maxAnisotropy = 1.0f,
        .maxLod = 1000.0f,
    }, 0),
};

while (gpuPollEvents()) {
    GpuCommandBuffer cmd = gpuBeginFrame(0.02f, 0.02f, 0.03f, 1.0f);
    if (!cmd.buffer) {
        continue;
    }

    gpuBeginRenderPass(cmd, (GpuRenderPassDesc) {
        .colorTargetCount = 1,
        .colorTargets = {{
            .image = gpuGetSwapchainImage(),
            .loadOp = kClear,
            .storeOp = kStore,
            .clearColor = {{ 0.02f, 0.02f, 0.03f, 1.0f }},
        }},
    });

    gpuDraw(&pipeline, cmd, &push, 6, 1, 0, 0);
    gpuEndRenderPass(cmd);
    gpuEndFrame(cmd);
}

gpuDestroyPipeline(pipeline);
gpuDestroyWindow();
gpuDestroy();
```

## images
*mostly just screenshots of stuff i've made with ggpu*

<img width="578" height="524" alt="image" src="https://github.com/user-attachments/assets/d9a8cd28-5873-43c4-9b76-1f89de86496c" />

<img width="935" height="552" alt="image" src="https://github.com/user-attachments/assets/b8cf75e2-a50b-42a3-be71-e40aa07aec1e" />
