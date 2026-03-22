#pragma once
#pragma warning(disable: 26495)
#include <vector>
#include "MaterialData.h"

struct ModelData;
struct BufferData;

inline constexpr const char* MAIN_PASS = "DefaultMainRenderPass";
inline constexpr const char* SHADOW_PASS = "DefaultShadowRenderPass";

struct PassBatchData {
    ModelData* model = nullptr;
    uint32_t firstInstance = 0;
    uint32_t instanceCount = 0;
};

struct PassBatchesResult {
    std::vector<PassBatchData> pass_batches_data;
    std::vector<std::vector<SDL_GPUTextureSamplerBinding>> texture_bindings;
};

struct RenderCommandData {
    SDL_GPUGraphicsPipeline* pipeline;
    std::vector<BufferData*> vertexStorageBuffers;
    std::vector<BufferData*> fragmentStorageBuffers;
    PassBatchesResult batches;
};

struct RenderPassTexturesData {
    void CreateColorTextureInfo(SDL_GPULoadOp load_op, SDL_GPUStoreOp store_op, SDL_FColor color, Uint32 numColorTargets = 1);
    void CreateDepthTextureInfo(SDL_GPULoadOp load_op, SDL_GPUStoreOp store_op);
    void SetColorTexture(SDL_GPUTexture* tex);
    void SetDepthTexture(SDL_GPUTexture* tex);

    SDL_GPUColorTargetInfo colorTargetInfo{};
    Uint32 numColorTargets = 0;
    SDL_GPUDepthStencilTargetInfo depthTargetInfo{};
};

struct RenderPass {
	MaterialDescription material_requirements;
    RenderPassTexturesData renderPassTexsData;
    std::vector<RenderCommandData> commands;
    std::vector<SDL_GPUTextureSamplerBinding> global_texture_bindings;
    int pass_index = -1;
};