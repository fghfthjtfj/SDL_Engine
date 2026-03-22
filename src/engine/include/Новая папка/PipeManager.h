#pragma once
#include <unordered_map>
#include <SDL3/SDL.h>
#include <SDL3/SDL_gpu.h>

class TextureManager;
struct ShaderProgram;

class PipeManager
{
public:
	PipeManager(SDL_GPUDevice* device, SDL_Window* win);
	SDL_GPUGraphicsPipeline* GetOrCreatePipeline(ShaderProgram* sp);
	SDL_GPUColorTargetDescription MakeDefaultColorTarget();

	SDL_GPUColorTargetDescription MakeNoColorTarget();

	SDL_GPUDepthStencilTargetInfo depthTargetInfo{};
	~PipeManager();

private:
	std::unordered_map<ShaderProgram*, SDL_GPUGraphicsPipeline*> pipelines;

	SDL_Window* win;
	SDL_GPUDevice* dev;

};

