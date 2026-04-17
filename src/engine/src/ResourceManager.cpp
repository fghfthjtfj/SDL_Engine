#include "PCH.h"
#include "ResourceManager.h"
#include "Utils.h"     // если там есть полезные макросы/функции
#include <SDL3/SDL.h>
#include <SDL3/SDL_gpu.h>

ResourceManager::ResourceManager(SDL_GPUDevice* device) {
    dev = device;
    CreateUploadTransferBuffer(BASE_TB_SIZE);
    CreateReadTransferBuffer(BASE_TB_SIZE);
    CreatePrepassDependedTransferBuffer(BASE_TB_SIZE);
}

ResourceManager::~ResourceManager()
{
    if (upload_transfer_buffer)
    {
        SDL_ReleaseGPUTransferBuffer(dev, upload_transfer_buffer);
    }
    if (read_transfer_buffer)
    {
        SDL_ReleaseGPUTransferBuffer(dev, read_transfer_buffer);
    }

    dev = nullptr;
    upload_transfer_buffer = nullptr;
    read_transfer_buffer = nullptr;
    mapped_upload_tb = nullptr;
    mapped_read_tb = nullptr;
}

// ────────────────────────────────────────
// Upload buffer
// ────────────────────────────────────────

void ResourceManager::MapUploadTransferBuffer()
{
    if (!upload_transfer_buffer) return;

    mapped_upload_tb = SDL_MapGPUTransferBuffer(dev, upload_transfer_buffer, false);
    if (!mapped_upload_tb)
    {
        SDL_Log("Failed to map upload transfer buffer: %s", SDL_GetError());
        SDL_ReleaseGPUTransferBuffer(dev, upload_transfer_buffer);
        upload_transfer_buffer = nullptr;
        current_upload_tb_size = 0;
        current_upload_tb_offset = 0;
    }
}

void ResourceManager::UnmapUploadTransferBuffer()
{
    if (upload_transfer_buffer && mapped_upload_tb)
    {
        SDL_UnmapGPUTransferBuffer(dev, upload_transfer_buffer);
        mapped_upload_tb = nullptr;
    }
}

SDL_GPUTransferBuffer* ResourceManager::CreateUploadTransferBuffer(uint32_t size)
{
    SDL_GPUTransferBufferCreateInfo info{};
    info.size = size;
    info.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;

    upload_transfer_buffer = SDL_CreateGPUTransferBuffer(dev, &info);
    if (!upload_transfer_buffer)
    {
        SDL_Log("Failed to create upload transfer buffer (%u bytes): %s",
            size, SDL_GetError());
    }

    return upload_transfer_buffer;
}

void ResourceManager::MapReadTransferBuffer()
{
    if (!read_transfer_buffer) return;

    mapped_read_tb = SDL_MapGPUTransferBuffer(dev, read_transfer_buffer, false);
    if (!mapped_read_tb)
    {
        SDL_Log("Failed to map read transfer buffer: %s", SDL_GetError());
        SDL_ReleaseGPUTransferBuffer(dev, read_transfer_buffer);
        read_transfer_buffer = nullptr;
        current_read_tb_size = 0;
        current_read_tb_offset = 0;
    }
}

void ResourceManager::UnmapReadTransferBuffer()
{
    if (read_transfer_buffer && mapped_read_tb)
    {
        SDL_UnmapGPUTransferBuffer(dev, read_transfer_buffer);
        mapped_read_tb = nullptr;
    }
}

void ResourceManager::MapPrepassDependedTransferBuffer()
{
    if (!prepass_dep_transfer_buffer) return;

    mapped_prepass_dep_tb = SDL_MapGPUTransferBuffer(dev, prepass_dep_transfer_buffer, false);
    if (!mapped_prepass_dep_tb)
    {
        SDL_Log("Failed to map prepass transfer buffer: %s", SDL_GetError());
        SDL_ReleaseGPUTransferBuffer(dev, prepass_dep_transfer_buffer);
        prepass_dep_transfer_buffer = nullptr;
        current_prepass_dep_tb_size = 0;
        current_prepass_dep_tb_size = 0;
    }
}

void ResourceManager::UnmapPrepassDependedTransferBuffer()
{
    if (prepass_dep_transfer_buffer && mapped_prepass_dep_tb)
    {
        SDL_UnmapGPUTransferBuffer(dev, prepass_dep_transfer_buffer);
        mapped_prepass_dep_tb = nullptr;
    }
}

SDL_GPUTransferBuffer* ResourceManager::CreateReadTransferBuffer(uint32_t size)
{
    SDL_GPUTransferBufferCreateInfo info{};
    info.size = size;
    info.usage = SDL_GPU_TRANSFERBUFFERUSAGE_DOWNLOAD;

    read_transfer_buffer = SDL_CreateGPUTransferBuffer(dev, &info);
    if (!read_transfer_buffer)
    {
        SDL_Log("Failed to create read transfer buffer (%u bytes): %s",
            size, SDL_GetError());
    }
    return read_transfer_buffer;
}

SDL_GPUTransferBuffer* ResourceManager::CreatePrepassDependedTransferBuffer(uint32_t size)
{
    SDL_GPUTransferBufferCreateInfo info{};
    info.size = size;
    info.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;

    prepass_dep_transfer_buffer = SDL_CreateGPUTransferBuffer(dev, &info);
    if (!prepass_dep_transfer_buffer)
    {
        SDL_Log("Failed to create upload transfer buffer (%u bytes): %s",
            size, SDL_GetError());
    }

    return prepass_dep_transfer_buffer;
}

void ResourceManager::EnsurePrepassDependedTransferBufferCapacity(uint32_t size)
{
    if (size <= current_prepass_dep_tb_size) return;

    SDL_Log("Growing prepass transfer buffer: from %u to %u bytes",
        current_prepass_dep_tb_size, size);

    if (prepass_dep_transfer_buffer)
    {
        UnmapPrepassDependedTransferBuffer();
        SDL_ReleaseGPUTransferBuffer(dev, prepass_dep_transfer_buffer);
        prepass_dep_transfer_buffer = nullptr;
    }
    MapPrepassDependedTransferBuffer();
    CreatePrepassDependedTransferBuffer(size);
    current_prepass_dep_tb_size = size;
    current_prepass_dep_tb_size = 0;
}

void ResourceManager::EnsureUploadTransferBufferCapacity(uint32_t size)
{
    if (size <= current_upload_tb_size) return;

    SDL_Log("Growing upload transfer buffer: from %u to %u bytes",
        current_upload_tb_size, size);

    if (upload_transfer_buffer)
    {
        UnmapUploadTransferBuffer();
        SDL_ReleaseGPUTransferBuffer(dev, upload_transfer_buffer);
        upload_transfer_buffer = nullptr;
    }

    CreateUploadTransferBuffer(size);
    MapUploadTransferBuffer();
    current_upload_tb_size = size;
    current_upload_tb_offset = 0;
}

void ResourceManager::EnsureReadTransferBufferCapacity(uint32_t size)
{
    if (size <= current_read_tb_size) return;

    SDL_Log("Growing read transfer buffer: from %u to %u bytes",
        current_read_tb_size, size);

    if (read_transfer_buffer)
    {
        UnmapReadTransferBuffer();
        SDL_ReleaseGPUTransferBuffer(dev, read_transfer_buffer);
        read_transfer_buffer = nullptr;
    }

    CreateReadTransferBuffer(size);
    MapReadTransferBuffer();
    current_read_tb_size = size;
    current_read_tb_offset = 0;
}