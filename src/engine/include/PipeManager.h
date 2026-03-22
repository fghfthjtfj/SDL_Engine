#pragma once
#include <unordered_map>
#include <SDL3/SDL.h>
#include <SDL3/SDL_gpu.h>

class ShaderManager;
struct ShaderProgram;
struct ComputeShaderProgram;

class PipeManager
{
public:
	PipeManager(SDL_GPUDevice* device, SDL_Window* win);
	void CreateGraphicsPiplenes(ShaderManager* sm);
	void CreateComputePipelines(ShaderManager* sm);

	SDL_GPUColorTargetDescription MakeDefaultColorTarget();
	SDL_GPUColorTargetDescription MakeNoColorTarget();

	SDL_GPUGraphicsPipeline* GetGraphicPipeline(ShaderProgram* sp);
	SDL_GPUComputePipeline* GetComputePipeline(ComputeShaderProgram* sp);

	//void BindComputePipelines(ShaderManager* sm);
	SDL_GPUDepthStencilTargetInfo depthTargetInfo{};
	~PipeManager();

private:
	SDL_GPUGraphicsPipeline* GetOrCreatePipeline(ShaderProgram* sp);
	SDL_GPUComputePipeline* GetOrCreateComputePipeline(ComputeShaderProgram* sp);

	std::unordered_map<ShaderProgram*, SDL_GPUGraphicsPipeline*> graphics_pipelines;
	std::unordered_map<ComputeShaderProgram*, SDL_GPUComputePipeline*> compute_pipelines;

	SDL_Window* win;
	SDL_GPUDevice* dev;

};

