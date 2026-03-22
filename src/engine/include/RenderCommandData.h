#pragma once
#include <vector>
#include <unordered_map>
#include <SDL3/SDL_gpu.h>
#include "Aliases.h"
#include "MaterialData.h"
#include "OrderedMap.h"

struct SubMeshData;
struct BufferData;
struct TextureData;

class PassManager;

struct ModelBatchData {
    std::vector<uint32_t> pib_sub_buffer;
    uint32_t firstInstance = 0;
    uint32_t instanceCount = 0;
    SubMeshData* submesh = nullptr;
};

struct TextureBatchData {
    std::unordered_map<ModelBatchKey, ModelBatchData> model_batches;
	std::vector<TextureData*> texture_uvl;
    uint32_t indirect_command_index = 0;
};

struct AtlasBatchData {
    std::unordered_map<TextureBatchKey, TextureBatchData> texture_batches;
    std::vector<SDL_GPUTextureSamplerBinding> texture_binding;
};

struct ShaderBatchData {
    std::unordered_map<AtlasBatchKey, AtlasBatchData> atlases_batches;
    std::vector<BufferData*> vertexStorageBuffers;
    std::vector<BufferData*> fragmentStorageBuffers;
    SDL_GPUGraphicsPipeline* pipeline = nullptr;
};

struct RenderPassTexturesInfo {
    void CreateColorTextureInfo(SDL_GPULoadOp load_op, SDL_GPUStoreOp store_op, SDL_FColor color, Uint32 numColorTargets = 1);
    void CreateDepthTextureInfo(SDL_GPULoadOp load_op, SDL_GPUStoreOp store_op);
    void SetColorTexture(SDL_GPUTexture* tex);
    void SetDepthTexture(SDL_GPUTexture* tex);

    SDL_GPUColorTargetInfo colorTargetInfo{};
    Uint32 numColorTargets = 0;
    SDL_GPUDepthStencilTargetInfo depthTargetInfo{};
};

struct RasterizerStateBiasParams {
    float depth_bias_constant_factor = 0.0f;
    float depth_bias_slope_factor = 0.0f;
    float depth_bias_clamp = 0.0f;
    bool enable_depth_bias = false;
};

struct RenderPassStep {
    RenderPassTexturesInfo renderPassTexsData;
    std::unordered_map<ShaderBatchKey, ShaderBatchData> shader_batches;
    std::function<void(SDL_GPUCommandBuffer*, PassManager*, RenderPassStep&)> render_function;
    std::vector<SDL_GPUTextureSamplerBinding> global_texture_bindings;
	RasterizerStateBiasParams rsb_params;
    int pass_index = -1;
};

struct ComputeShaderBatchData {
    std::function<void(const PushConstantBinder&, const void*)> push_func = {};
    std::vector<BufferData*> ro_storage_buffers; // set=0, SDL_BindGPUComputeStorageBuffers
    std::vector<BufferData*> rw_storage_buffers; // set=1, SDL_BeginGPUComputePass
    SDL_GPUComputePipeline* pipeline = nullptr;
};

struct ComputePassStep {
    std::unordered_map<ShaderBatchKey, ComputeShaderBatchData> shader_batches;
    std::function<void(SDL_GPUCommandBuffer*, PassManager*, ComputePassStep&, uint8_t)> compute_function;
    int pass_index = -1;
};
