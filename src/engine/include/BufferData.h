#pragma once
#include "config.h"
#include "SDL3/SDL_gpu.h"

enum struct BufferDataType {
	Static,
	Dynamic
};
struct BufferData {
    BufferDataType type = BufferDataType::Static;
    std::string debug_name;

    struct StaticBufferInfo {
        SDL_GPUBuffer* buffer = nullptr;
        Uint32 buffer_size = 0;
        Uint32 used_buffer_size = 0;
    } Static;

    struct DynamicBufferInfo {
        SDL_GPUBuffer* buffers[BUFFERING_LEVEL]{};
        Uint32 buffer_size[BUFFERING_LEVEL]{};
        Uint32 used_buffer_size[BUFFERING_LEVEL]{};
    } Dynamic;

    SDL_GPUBufferUsageFlags usage = SDL_GPU_BUFFERUSAGE_GRAPHICS_STORAGE_READ;
};
