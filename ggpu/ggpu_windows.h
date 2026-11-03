#ifndef GGPU_WINDOWS_H
#define GGPU_WINDOWS_H

// This should be included last! >:3
#include "ggpu.h"

typedef struct GpuWindow {
    HINSTANCE hinstance;
    uint32_t width;
    uint32_t height;
    bool shouldClose;
    bool resized;
    HWND hwnd;

    void (*callback)(UINT msg, WPARAM param, LPARAM lparam);
} GpuWindow;


// Call this after `gpuInit()` and `gpuCreateWindow()`
void gpuCreateSurface();

// Initializes a Windows only swapchain object, after calling `gpuCreateSurface`
void gpuCreateSwapchain(void (*callback)(int width, int height));

// Creates a generic Win32 window with the specified dimensions and name
void gpuCreateWindow(const char* title, uint32_t width, uint32_t height, void (*callback)(UINT msg, WPARAM param, LPARAM lparam));

// Destroys the current window.
// Must be called before `gpuDestroy`
void gpuDestroyWindow();

// Returns false when the window is closed
bool gpuPollEvents();
#endif // GGPU_WINDOWS_H

#ifndef GGPU_IMPLEMENTATION_IMPORTED_WIN32
#ifdef GGPU_IMPLEMENTATION
#define GGPU_IMPLEMENTATION_IMPORTED_WIN32

static GpuWindow ggpuWindow = { 0 };

static LRESULT CALLBACK _gpu_window_proc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_CLOSE:
        case WM_DESTROY:
            ggpuWindow.shouldClose = true;
            PostQuitMessage(0);
            return 0;
        case WM_SIZE:
            ggpuWindow.width = LOWORD(lParam);
            ggpuWindow.height = HIWORD(lParam);
            ggpu.swapchain.resized = true;
            return 0;
    }

    if (ggpuWindow.callback) ggpuWindow.callback(msg, wParam, lParam);
    return DefWindowProcA(hwnd, msg, wParam, lParam);
}

// Call this after `gpuInit()` and `gpuCreateWindow()`
void gpuCreateSurface() {
    VkWin32SurfaceCreateInfoKHR surfaceInfo = { 0 };
    surfaceInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    surfaceInfo.hwnd = ggpuWindow.hwnd;
    surfaceInfo.hinstance = ggpuWindow.hinstance;

    gpuAssert(vkCreateWin32SurfaceKHR(ggpu.instance, &surfaceInfo, NULL,
        &ggpu.swapchain.surface) == VK_SUCCESS,
        "gpuCreateSurface(): failed to create Win32 Vulkan surface!");
}

// Initializes a Windows only swapchain object, after calling `gpuCreateSurface`
void gpuCreateSwapchain(void (*callback)(int width, int height)) {
    GpuSwapchain* sc = &ggpu.swapchain;
    VkSurfaceKHR surface = sc->surface;
    ggpu.swapchainCallback = callback;

    VkSurfaceCapabilitiesKHR caps;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(ggpu.physicalDevice, surface, &caps);

    VkSurfaceFormatKHR fmt = _gpu_choose_surface_format();
    VkPresentModeKHR mode = _gpu_choose_present_mode();

    VkExtent2D extent = caps.currentExtent;
    if (extent.width == 0xFFFFFFFF) {
        // Alright, we have to choose our stuff
        extent.width = ggpuWindow.width;
        extent.height = ggpuWindow.height;
        extent.width = extent.width < caps.minImageExtent.width ? caps.minImageExtent.width : extent.width;
        extent.width = extent.width > caps.maxImageExtent.width ? caps.maxImageExtent.width : extent.width;
        extent.height = extent.height < caps.minImageExtent.height ? caps.minImageExtent.height : extent.height;
        extent.height = extent.height > caps.maxImageExtent.height ? caps.maxImageExtent.height : extent.height;
    }

    uint32_t imageCount = caps.minImageCount + 1;
    if (caps.maxImageCount > 0 && imageCount > caps.maxImageCount)
        imageCount = caps.maxImageCount;

    gpuAssert(imageCount <= GGPU_MAX_SWAPCHAIN_IMAGES, "gpuCreateSwapchain(): image count exceeds GGPU_MAX_SWAPCHAIN_IMAGES!");

    VkSwapchainCreateInfoKHR info = { 0 };
    VkSwapchainKHR oldSwapchain = ggpu.swapchain.rawSwapchain;

    info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    info.surface = surface;
    info.minImageCount = imageCount;
    info.imageFormat = fmt.format;
    info.imageColorSpace = fmt.colorSpace;
    info.imageExtent = extent;
    info.imageArrayLayers = 1;
    info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    info.preTransform = caps.currentTransform;
    info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    info.presentMode = mode;
    info.clipped = VK_TRUE;
    info.oldSwapchain = oldSwapchain;

    gpuAssert(vkCreateSwapchainKHR(ggpu.device, &info, NULL, &sc->rawSwapchain) == VK_SUCCESS,
        "gpuCreateSwapchain(): vkCreateSwapchainKHR failed!");

    sc->format = fmt.format;
    sc->extent = extent;
    sc->imageCount = imageCount;

    // Retrieve images
    vkGetSwapchainImagesKHR(ggpu.device, sc->rawSwapchain, &sc->imageCount, NULL);
    vkGetSwapchainImagesKHR(ggpu.device, sc->rawSwapchain, &sc->imageCount, sc->images);

    // Create one view per swapchain image
    for (uint32_t i = 0; i < sc->imageCount; i++) {
        VkImageViewCreateInfo viewInfo = { 0 };
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = sc->images[i];
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = sc->format;
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.layerCount = 1;
        gpuAssert(vkCreateImageView(ggpu.device, &viewInfo, NULL, &sc->views[i]) == VK_SUCCESS,
            "gpuCreateSwapchain(): failed to create image view %u!", i);
    }

    // Sync stuff
    if (sc->inFlight[0] == VK_NULL_HANDLE) {
        VkSemaphoreCreateInfo semInfo = { 0 };
        semInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        // Create semaphores for context related stuff
        for (uint32_t i = 0; i < GGPU_FRAMES_IN_FLIGHT; i++)
            vkCreateSemaphore(ggpu.device, &semInfo, NULL, &sc->inFlight[i]);

        for (uint32_t i = 0; i < GGPU_MAX_SWAPCHAIN_IMAGES; i++)
            vkCreateSemaphore(ggpu.device, &semInfo, NULL, &sc->renderFinished[i]);

        // Per-frame fences, pre-signaled so frame 0 doesn't deadlock on wait
        VkFenceCreateInfo fenceInfo = { 0 };
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
        for (uint32_t i = 0; i < GGPU_FRAMES_IN_FLIGHT; i++)
            vkCreateFence(ggpu.device, &fenceInfo, NULL, &sc->fences[i]);
    }

    if (oldSwapchain != VK_NULL_HANDLE) {
        vkDestroySwapchainKHR(ggpu.device, oldSwapchain, NULL);
    }

    _gpu_register_swapchain_images();
    callback(caps.currentExtent.width, caps.currentExtent.height);
#ifdef GGPU_VALIDATION
    printf("gpuCreateSwapchain(): %ux%u, %u images, format %d\n",
        extent.width, extent.height, sc->imageCount, sc->format);
#endif
}

// Creates a generic Win32 window with the specified dimensions and name
void gpuCreateWindow(const char* title, uint32_t width, uint32_t height, void (*callback)(UINT msg, WPARAM param, LPARAM lparam)) {
    ggpuWindow.hinstance = GetModuleHandleA(NULL);
    ggpuWindow.width = width;
    ggpuWindow.height = height;
    ggpuWindow.callback = callback;

    WNDCLASSEXA wc = { 0 };
    wc.cbSize = sizeof(WNDCLASSEXA);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = _gpu_window_proc;
    wc.hInstance = ggpuWindow.hinstance;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.lpszClassName = "ggpu_window";
    RegisterClassExA(&wc);

    // Adjust so client area is exactly width x height
    RECT rect = { 0, 0, (LONG)width, (LONG)height };
    AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, FALSE);

    ggpuWindow.hwnd = CreateWindowExA(
        0, "ggpu_window", title,
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        rect.right - rect.left,
        rect.bottom - rect.top,
        NULL, NULL, ggpuWindow.hinstance, NULL
    );

    gpuAssert(ggpuWindow.hwnd != NULL, "gpuCreateWindow(): failed to create Win32 window!");
    ShowWindow(ggpuWindow.hwnd, SW_SHOW);
    UpdateWindow(ggpuWindow.hwnd);
}

// Destroys the current window.
// Must be called before `gpuDestroy`
void gpuDestroyWindow() {
    // Swapchain images and views
    vkDeviceWaitIdle(ggpu.device);

    for (uint32_t i = 0; i < ggpu.swapchain.imageCount; i++)
        vkDestroyImageView(ggpu.device, ggpu.swapchain.views[i], NULL);
    vkDestroySwapchainKHR(ggpu.device, ggpu.swapchain.rawSwapchain, NULL);

    // Sync primitives
    for (uint32_t i = 0; i < GGPU_FRAMES_IN_FLIGHT; i++) {
        vkDestroySemaphore(ggpu.device, ggpu.swapchain.inFlight[i], NULL);
        vkDestroyFence(ggpu.device, ggpu.swapchain.fences[i], NULL);
        ggpu.swapchain.inFlight[i] = VK_NULL_HANDLE;
        ggpu.swapchain.fences[i] = VK_NULL_HANDLE;
    }

    for (uint32_t i = 0; i < GGPU_MAX_SWAPCHAIN_IMAGES; i++)
        vkDestroySemaphore(ggpu.device, ggpu.swapchain.renderFinished[i], NULL);

    vkDestroySurfaceKHR(ggpu.instance, ggpu.swapchain.surface, NULL);
    DestroyWindow(ggpuWindow.hwnd);
    ggpuWindow = (GpuWindow){ 0 };
}

// Returns false when the window is closed
bool gpuPollEvents() {
    MSG msg = { 0 };
    while (PeekMessageA(&msg, NULL, 0, 0, PM_REMOVE)) {
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }
    return !ggpuWindow.shouldClose;
}

#endif
#endif