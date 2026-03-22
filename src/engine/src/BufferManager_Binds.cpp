#include "PCH.h"
#include "BufferManager.h"

void BufferManager::BindGPUIndexBuffer(SDL_GPURenderPass* rp, Uint32 offset)
{
    BufferData* data = GetBufferData(DEFAULT_INDEX_BUFFER);
    if (!data) {
        SDL_Log("BindGPUIndexBuffer::Index buffer 'DefaultIndexBuffer' not found");
        return;
    }

    if (data->type != BufferDataType::Static) {
        SDL_Log("Index buffer 'DefaultIndexBuffer' is not static");
        return;
    }

    SDL_GPUBuffer* buf = data->Static.buffer;
    if (!buf) {
        SDL_Log("Index buffer 'DefaultIndexBuffer' has null buffer pointer");
        return;
    }

    SDL_GPUBufferBinding ibind{};
    ibind.buffer = buf;
    ibind.offset = offset;

    SDL_BindGPUIndexBuffer(rp, &ibind, SDL_GPU_INDEXELEMENTSIZE_32BIT);
}

void BufferManager::BindGPUVertexBuffer(SDL_GPURenderPass* rp, Uint32 offset, Uint32 slot)
{
    BufferData* data = GetBufferData(DEFAULT_VERTEX_BUFFER);
    if (!data) {
        SDL_Log("BindGPUVertexBuffer::Vertex buffer 'DefaultVertexBuffer' not found");
        return;
    }

    if (data->type != BufferDataType::Static) {
        SDL_Log("Vertex buffer 'DefaultVertexBuffer' is not static");
        return;
    }

    SDL_GPUBuffer* buf = data->Static.buffer;
    if (!buf) {
        SDL_Log("Vertex buffer 'DefaultVertexBuffer' has null buffer pointer");
        return;
    }

    SDL_GPUBufferBinding vbind{};
    vbind.buffer = buf;
    vbind.offset = offset;

    SDL_BindGPUVertexBuffers(rp, slot, &vbind, 1);
}


void BufferManager::BindGPUVertexStorageBuffers(SDL_GPURenderPass* rp, Uint32 slot, const std::vector<BufferData*>& buffers_data_vec, uint8_t frame)
{
    std::vector<SDL_GPUBuffer*> buffers;
    buffers.reserve(buffers_data_vec.size());

    for (const BufferData* data : buffers_data_vec)
    {
        SDL_GPUBuffer* buf = _GetGPUBufferForFrame(data, frame);
        if (buf)
            buffers.push_back(buf);
        else
            SDL_Log("Null or invalid buffer_data in BindGPUVertexStorageBuffers");
    }

    SDL_BindGPUVertexStorageBuffers(rp, slot, buffers.data(), safe_u32(buffers.size()));
}


void BufferManager::BindGPUVertexStorageBuffers(SDL_GPURenderPass* rp, Uint32 slot, std::initializer_list<const char*> names, uint8_t frame)
{
    std::vector<SDL_GPUBuffer*> buffers;
    buffers.reserve(names.size());

    for (const char* name : names)
    {
        auto it = buffers_data.find(name);
        if (it != buffers_data.end())
        {
            SDL_GPUBuffer* buf = _GetGPUBufferForFrame(it->second.get(), frame);
            if (buf)
                buffers.push_back(buf);
            else
                SDL_Log("Buffer '%s' is invalid (BindGPUVertexStorageBuffers)", name);
        }
        else {
            SDL_Log("BindGPUVertexStorageBuffers::Buffer '%s' not found", name);
        }
    }

    SDL_BindGPUVertexStorageBuffers(rp, slot, buffers.data(), safe_u32(buffers.size()));
}


void BufferManager::BindGPUFragmentStorageBuffers(
    SDL_GPURenderPass* rp,
    Uint32 slot,
    const std::vector<BufferData*>& buffers_data_vec,
    uint8_t render_frame)
{
    std::vector<SDL_GPUBuffer*> buffers;
    buffers.reserve(buffers_data_vec.size());

    for (const BufferData* data : buffers_data_vec)
    {
        SDL_GPUBuffer* buf = _GetGPUBufferForFrame(data, render_frame);
        if (buf)
            buffers.push_back(buf);
        else
            SDL_Log("Null or invalid buffer_data in BindGPUFragmentStorageBuffers");
    }

    SDL_BindGPUFragmentStorageBuffers(rp, slot, buffers.data(), safe_u32(buffers.size()));
}


void BufferManager::BindGPUFragmentStorageBuffers(SDL_GPURenderPass* rp, Uint32 slot, std::initializer_list<const char*> names, uint8_t frame)
{
    std::vector<SDL_GPUBuffer*> buffers;
    buffers.reserve(names.size());

    for (const char* name : names)
    {
        auto it = buffers_data.find(name);
        if (it != buffers_data.end())
        {
            SDL_GPUBuffer* buf = _GetGPUBufferForFrame(it->second.get(), frame);
            if (buf)
                buffers.push_back(buf);
            else
                SDL_Log("Buffer '%s' is invalid (BindGPUFragmentStorageBuffers)", name);
        }
        else {
            SDL_Log("BindGPUFragmentStorageBuffers::Buffer '%s' not found", name);
        }
    }

    SDL_BindGPUFragmentStorageBuffers(rp, slot, buffers.data(), safe_u32(buffers.size()));
}

std::vector<SDL_GPUStorageBufferReadWriteBinding> BufferManager::BuildBindGPUComputeRWBuffers(const std::vector<BufferData*>& buffers_data, uint8_t render_frame)
{
	std::vector<SDL_GPUStorageBufferReadWriteBinding> buffers;
	buffers.reserve(buffers_data.size());

    for (const BufferData* data : buffers_data)
    {
        SDL_GPUBuffer* buf = _GetGPUBufferForFrame(data, render_frame);
        if (buf) {
            //SDL_Log("Compute RW buffer ptr: %p", (void*)buf);
            buffers.push_back({ buf, false });
        }
        else {
            SDL_Log("Null or invalid buffer_data in BindGPUComputeStorageBuffers");
        }
	}
    return buffers;
}

std::vector<SDL_GPUStorageBufferReadWriteBinding> BufferManager::BuildBindGPUComputeRWBuffers(std::initializer_list<const char*> names, uint8_t render_frame)
{
	std::vector<SDL_GPUStorageBufferReadWriteBinding> buffers;
	buffers.reserve(names.size());

    for (const char* name : names)
    {
        auto it = buffers_data.find(name);
        if (it != buffers_data.end())
        {
            SDL_GPUBuffer* buf = _GetGPUBufferForFrame(it->second.get(), render_frame);
            if (buf)
                buffers.push_back({ buf, false });
            else
                SDL_Log("Buffer '%s' is invalid (BindGPUComputeStorageBuffers)", name);
        }
        else {
            SDL_Log("BindGPUComputeStorageBuffers::Buffer '%s' not found", name);
        }
	}
	return buffers;
}

void BufferManager::BindGPUComputeRO_Buffers(SDL_GPUComputePass* cmp, uint32_t slot, const std::vector<BufferData*>& buffers_data, uint8_t frame)
{
    std::vector<SDL_GPUBuffer*> buffers;
    buffers.reserve(buffers_data.size());
    for (const BufferData* data : buffers_data) {
        SDL_GPUBuffer* buf = _GetGPUBufferForFrame(data, frame);
        if (buf) buffers.push_back(buf);
        else SDL_Log("Null buffer in BindGPUComputeROStorageBuffers");
    }

    SDL_BindGPUComputeStorageBuffers(cmp, slot, buffers.data(), safe_u32(buffers.size()));
}