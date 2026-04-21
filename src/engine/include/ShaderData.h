#pragma once
#include <vector>
#include <string>
#include <cstddef>
#include <SDL3/SDL_gpu.h>

struct BufferData;
struct RenderPassStep;
struct ComputePassStep;

namespace ShaderBase {

    struct ShaderData
    {
        SDL_GPUShader* shader = nullptr;
        size_t shader_size = 0;
        Uint8* shader_code = nullptr;
    };
}

using namespace ShaderBase;

struct PushConstantBinder {
    SDL_GPUCommandBuffer* cb;

    template<typename T>
    void Push(Uint32 slot, const T& data) const {
        SDL_PushGPUComputeUniformData(cb, slot, &data, sizeof(T));
    }
};

struct VertexShaderData {
    ShaderData shader_data;
    std::vector<SDL_GPUVertexAttribute> attributes;
    SDL_GPUVertexBufferDescription vb{};
};

struct FragmentShaderData {
    ShaderData shader_data;
};

struct ComputeShaderData {
	Uint8* spv_code = nullptr;
	size_t spv_size = 0;
    Uint32 threadcount_x = 1;
    Uint32 threadcount_y = 1;
    Uint32 threadcount_z = 1;
    Uint32 num_samplers = 0;
    Uint32 num_readonly_storage_textures = 0;
    Uint32 num_readonly_storage_buffers = 0;
    Uint32 num_readwrite_storage_textures = 0;
    Uint32 num_readwrite_storage_buffers = 0;
    Uint32 num_uniform_buffers = 0;
};

struct ShaderProgramDescription
{
    SDL_GPUCullMode cull_mode = SDL_GPU_CULLMODE_NONE;
    SDL_GPUTextureFormat depth_stencil_format = SDL_GPU_TEXTUREFORMAT_D32_FLOAT;
    RenderPassStep* associated_render_pass = nullptr;
    bool enable_depth_test = true;
    bool enable_depth_write = true;
    bool enable_stencil_test = false;
    bool has_color_target = true;
};

enum class TextureSlotRole {
    Albedo,
    Normal,
};

struct ShaderProgram {
    VertexShaderData vs;
    std::vector<BufferData*> vertex_shader_buffers;

    FragmentShaderData fs;
    std::vector<BufferData*> fragment_shader_buffers;

    // ќжидаемые типы (по назначению) текстур дл€ этого шейдера. Ќапример, если в шейдере есть uniform sampler2D u_albedoTexture, то в required_slots будет TextureSlotRole::Albedo.
	// Expected texture types (by role) for this shader. For example, if the shader has a uniform sampler2D u_albedoTexture, then required_slots will contain TextureSlotRole::Albedo.
    std::vector<TextureSlotRole> required_slots;
	ShaderProgramDescription* spd;
};

struct ComputeShaderProgram {
    ComputeShaderData cs;
    std::vector<BufferData*> rw_storage_buffers;
    std::vector<BufferData*> ro_storage_buffers; 
    ComputePassStep* associated_compute_pass = nullptr;
    std::function<void(const PushConstantBinder&, const void*)> push_func = nullptr;

    template<typename T, typename Fn>
    void BindPushConstants(Fn&& fn) {
        push_func = [fn = std::forward<Fn>(fn)](const PushConstantBinder& binder, const void* raw) {
            fn(binder, *static_cast<const T*>(raw));
        };
    }
};