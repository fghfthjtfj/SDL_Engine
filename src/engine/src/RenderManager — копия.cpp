#include "PCH.h"
#include "RenderManager.h"
#include "BufferManager.h"
#include "ModelData.h"
#include "TextureManager.h"

RenderManager::RenderManager(BufferManager* buffer_manager, Fcolor color) {
	colorTargetInfo = {};
	colorTargetInfo.load_op = SDL_GPU_LOADOP_CLEAR;
	colorTargetInfo.store_op = SDL_GPU_STOREOP_STORE;
	colorTargetInfo.clear_color = SDL_FColor({ color.r, color.g, color.b, color.a });

}

RenderCommandData* RenderManager::CreateRenderCommand(const std::string& name, RenderCommandData&& params)
{
	// Проверяем — если уже существует, вернуть существующий
	auto it = render_commands.find(name);
	if (it != render_commands.end()) {
		SDL_Log("Render command '%s' already exists, returning existing command.", name.c_str());
		return it->second.get();
	}

	auto cmd = std::make_unique<RenderCommandData>(std::move(params));
	RenderCommandData* ptr = cmd.get();

	render_commands.emplace(name, std::move(cmd));

	return ptr;
}


void RenderManager::ExecuteRenderCommand(SDL_GPURenderPass* rp, BufferManager* buffer_manager, uint8_t render_frame)
{
	//auto t0 = std::chrono::high_resolution_clock::now();
	for (auto& [name, cmd] : render_commands)
	{
		SDL_BindGPUGraphicsPipeline(rp, cmd->pipeline);

		buffer_manager->BindGPUVertexBuffer(rp, 0, 0);
		buffer_manager->BindGPUIndexBuffer(rp, 0);

		if (!cmd->vertexStorageBuffers.empty()) {
			buffer_manager->BindGPUVertexStorageBuffers(rp, 0, cmd->vertexStorageBuffers, render_frame);
		}
		else {
			SDL_Log("No vertex storage buffers found for render command");
		}
		if (!cmd->fragmentStorageBuffers.empty()) {
			buffer_manager->BindGPUFragmentStorageBuffers(rp, 0, cmd->fragmentStorageBuffers, render_frame);
		}
		else {
			SDL_Log("No fragment storage buffers found for render command");
		}

		for (int i = 0; i < cmd->batches.pass_batches_data.size(); i++) {
			const auto& batch = cmd->batches.pass_batches_data[i];
			if (i < cmd->batches.texture_bindings.size() && !cmd->batches.texture_bindings[i].empty()) {
				const auto& bindings = cmd->batches.texture_bindings[i];
				SDL_BindGPUFragmentSamplers(rp, 0, bindings.data(), static_cast<Uint32>(bindings.size()));
			}
			SDL_DrawGPUIndexedPrimitives(rp, batch.model->indexCount, batch.instanceCount, batch.model->indexOffset, batch.model->vertexOffset, batch.firstInstance);
		}

	}
	//auto t1 = std::chrono::high_resolution_clock::now();
	//double seconds = std::chrono::duration<double>(t1 - t0).count();
	//std::cout << "RenderCommand execution taken " << seconds << " s" << std::endl;
}

void RenderManager::UpdateColorTargetInfoTex(SDL_GPUTexture* tex)
{
	colorTargetInfo.texture = tex;
}

void RenderManager::SetDepthTargetInfo(TextureManager* tm, Uint32 width, Uint32 height)
{

	SDL_GPUDepthStencilTargetInfo depthTargetInfo;
	SDL_zero(depthTargetInfo);
	depthTargetInfo.texture = tm->CreateGPU_Texture(width, height, SDL_GPU_TEXTUREFORMAT_D16_UNORM, SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET);

	depthTargetInfo.clear_depth = 1.0f;
	depthTargetInfo.load_op = SDL_GPU_LOADOP_CLEAR;
	depthTargetInfo.store_op = SDL_GPU_STOREOP_DONT_CARE;
	depthTargetInfo.cycle = true;
	this->depthTargetInfo = depthTargetInfo;
}

RenderCommandData* RenderManager::operator[](const std::string& name)
{
	auto it = render_commands.find(name);
	if (it != render_commands.end()) {
		return it->second.get();
	}
	SDL_Log("Render command '%s' not found", name.c_str());
	return nullptr;
}

RenderManager::~RenderManager()
{
	render_commands.clear();
}
