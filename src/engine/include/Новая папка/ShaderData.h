#pragma once
#pragma warning(disable: 26495)

#include <vector>
#include <string>
#include <cstddef>

struct BufferData;

struct ShaderData {
    SDL_GPUShader* shader = nullptr;

    // Твой временный буфер (НЕ часть идентичности, только для переноски между функциями)
    Uint8* shader_code = nullptr;
    size_t shader_size = 0;
	Uint32 num_samplers = 0;
};

struct VertexShaderData {
    ShaderData shader_data;
    SDL_GPUVertexBufferDescription vb{};
    std::vector<SDL_GPUVertexAttribute> attributes;
};

struct FragmentShaderData {
    ShaderData shader_data;
};

struct ShaderProgram
{
    VertexShaderData vs;
    std::vector<BufferData*> vertex_shader_buffers;

    FragmentShaderData fs;
    std::vector<BufferData*> fragment_shader_buffers;

    SDL_GPUCullMode cull_mode = SDL_GPU_CULLMODE_BACK;
    bool enable_depth_test = true;
    bool enable_depth_write = true;
    bool enable_stencil_test = false;
    bool has_color_target = true;

    SDL_GPUColorTargetDescription ctd{};
};
