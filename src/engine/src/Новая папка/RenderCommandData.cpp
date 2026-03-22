#include "PCH.h"
#include "RenderCommandData.h"

void RenderPassTexturesData::CreateColorTextureInfo(SDL_GPULoadOp load_op, SDL_GPUStoreOp store_op, SDL_FColor color, Uint32 numColorTargets)
{
	colorTargetInfo.load_op = load_op;
	colorTargetInfo.store_op = store_op;
	colorTargetInfo.clear_color = color;
	this->numColorTargets = numColorTargets;
}

void RenderPassTexturesData::CreateDepthTextureInfo(SDL_GPULoadOp load_op, SDL_GPUStoreOp store_op)
{
	depthTargetInfo.clear_depth = 1.0f;
	depthTargetInfo.clear_stencil = 0;
	depthTargetInfo.load_op = load_op;
	depthTargetInfo.store_op = store_op;
	depthTargetInfo.cycle = true;
	depthTargetInfo.stencil_load_op = SDL_GPU_LOADOP_DONT_CARE;
	depthTargetInfo.stencil_store_op = SDL_GPU_STOREOP_DONT_CARE;
}

void RenderPassTexturesData::SetColorTexture(SDL_GPUTexture* tex)
{
	colorTargetInfo.texture = tex;
}

void RenderPassTexturesData::SetDepthTexture(SDL_GPUTexture* tex)
{
	depthTargetInfo.texture = tex;
}

