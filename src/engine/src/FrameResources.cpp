#include "PCH.h"
#include "FrameResources.h"
#include "TextureManager.h"

FrameRecources::FrameRecources()
{
}

void FrameRecources::CreateDepthTexture(TextureManager* tm, Uint32 width, Uint32 height)
{
    SDL_GPUTexture* tex = tm->CreateGPU_Texture(width, height, SDL_GPU_TEXTUREFORMAT_D16_UNORM, SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET);
    if (!tex) {
        SDL_Log("Depth texture creation failed: %s", SDL_GetError());
        return;
    };
    SDL_GPUDepthStencilTargetInfo depthTargetInfo;
    SDL_zero(depthTargetInfo);
    depthTargetInfo.texture = tex;
    depthTargetInfo.clear_depth = 1.0f;
    depthTargetInfo.load_op = SDL_GPU_LOADOP_CLEAR;
    depthTargetInfo.store_op = SDL_GPU_STOREOP_DONT_CARE;
    depthTargetInfo.cycle = false;
    depthTargetInfo.clear_stencil = 0;
    this->depthTargetInfo = depthTargetInfo;

}