#include "PCH.h"
#include "RenderCommandData.h"

void RenderPassTexturesInfo::CreateColorTextureInfo(SDL_GPULoadOp load_op, SDL_GPUStoreOp store_op, SDL_FColor color, SDL_GPUTextureFormat format, Uint32 num_color_targets)
{
	colorTargetInfo.load_op = load_op;
	colorTargetInfo.store_op = store_op;
	colorTargetInfo.clear_color = color;
	numColorTargets = num_color_targets;
	color_format = format;
}

void RenderPassTexturesInfo::CreateDepthTextureInfo(SDL_GPULoadOp load_op, SDL_GPUStoreOp store_op, SDL_GPUTextureFormat format)
{
	depthTargetInfo.clear_depth = 1.0f;
	depthTargetInfo.clear_stencil = 0;
	depthTargetInfo.load_op = load_op;
	depthTargetInfo.store_op = store_op;
	depthTargetInfo.cycle = true;
	depthTargetInfo.stencil_load_op = SDL_GPU_LOADOP_DONT_CARE;
	depthTargetInfo.stencil_store_op = SDL_GPU_STOREOP_DONT_CARE;
	depth_format = format;
}

void RenderPassTexturesInfo::SetColorTexture(SDL_GPUTexture* tex)
{
	colorTargetInfo.texture = tex;
}

void RenderPassTexturesInfo::SetDepthTexture(SDL_GPUTexture* tex)
{
	if (depthTargetInfo.texture) {

	}
	depthTargetInfo.texture = tex;
}

