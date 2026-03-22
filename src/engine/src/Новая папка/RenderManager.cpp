#include "PCH.h"
#include "RenderManager.h"
#include "BufferManager.h"
#include "ModelData.h"
#include "MaterialManager.h"
#include "PipeManager.h"
#include "ObjectManager.h"

RenderManager::RenderManager() {}

RenderPass* RenderManager::CreateRenderPass(const std::string& name, RenderPassTexturesData&& rptd, int pass_index)
{
	auto data = std::make_unique<RenderPass>();
	data->renderPassTexsData = std::move(rptd);

	// ВАЖНО: размер берём ДО вставки в map
	if (pass_index == -1) {
		data->pass_index = static_cast<int>(render_passes.size());
	}
	else {
		data->pass_index = pass_index;
	}

	RenderPass* ptr = data.get();
	render_passes[name] = std::move(data);

	return ptr;
}

void RenderManager::FillRenderPasses(MaterialManager* mm, PipeManager* pm, ObjectManager* om)
{
	std::vector<Material*> materials = mm->GetAllMaterials();
	for (auto& material : materials) {
		for (auto& [rp_name, shader_program] : material->variants) {
			auto it = render_passes.find(rp_name);
			if (it != render_passes.end()) {
				RenderCommandData rcd{};
				rcd.pipeline = pm->GetOrCreatePipeline(shader_program);
				rcd.vertexStorageBuffers = shader_program->vertex_shader_buffers;
				rcd.fragmentStorageBuffers = shader_program->fragment_shader_buffers;
				// тут om собирает батчи для рендера
				rcd.batches = om->BuildPassBatches(om->GetActiveScene(), rp_name, shader_program);

				it->second->commands.push_back(std::move(rcd));

			}
			else{
				SDL_Log("Render pass '%s' not found for material variant", rp_name.c_str());
				continue;
			}
		}
	}
	// пересобираем упорядоченный список проходов
	ordered_passes.clear();
	ordered_passes.reserve(render_passes.size());

	for (auto& [_, rp] : render_passes)
		ordered_passes.push_back(rp.get());

	std::sort(
		ordered_passes.begin(),
		ordered_passes.end(),
		[](const RenderPass* a, const RenderPass* b) {
		return a->pass_index < b->pass_index;
	}
	);

}

void RenderManager::ExecuteRenderPasses(SDL_GPUCommandBuffer* cb, BufferManager* bm, uint8_t render_frame)
{
    for (auto& render_pass : ordered_passes){
		SDL_GPURenderPass* rp = SDL_BeginGPURenderPass(cb,
			&render_pass->renderPassTexsData.colorTargetInfo,
			render_pass->renderPassTexsData.numColorTargets,
			&render_pass->renderPassTexsData.depthTargetInfo);
		
		ExecuteRenderCommands(rp, *render_pass, bm, render_pass->commands, render_frame);
		SDL_EndGPURenderPass(rp);
	}
}

RenderPass* RenderManager::operator[](const std::string& name)
{
	auto it = render_passes.find(name);
	if (it != render_passes.end()) {
		return it->second.get();
	}
	SDL_Log("Render pass '%s' not found", name.c_str());
	return nullptr;
}

RenderManager::~RenderManager()
{
	render_passes.clear();
}

void RenderManager::ExecuteRenderCommands(SDL_GPURenderPass* SDL_rp, const RenderPass& rp, BufferManager* bm, std::vector<RenderCommandData>& rcd, uint8_t render_frame)
{
	for (auto& cmd : rcd)
	{

		SDL_BindGPUGraphicsPipeline(SDL_rp, cmd.pipeline);
		SDL_BindGPUFragmentSamplers(SDL_rp, 0, rp.global_texture_bindings.data(), safe_u32(rp.global_texture_bindings.size()));

		bm->BindGPUVertexBuffer(SDL_rp, 0, 0);
		bm->BindGPUIndexBuffer(SDL_rp, 0);

		if (!cmd.vertexStorageBuffers.empty()) {
			bm->BindGPUVertexStorageBuffers(SDL_rp, 0, cmd.vertexStorageBuffers, render_frame);
		}
		else {
			//SDL_Log("No vertex storage buffers found for render command");
		}
		if (!cmd.fragmentStorageBuffers.empty()) {
			bm->BindGPUFragmentStorageBuffers(SDL_rp, 0, cmd.fragmentStorageBuffers, render_frame);
		}
		else {
			//SDL_Log("No fragment storage buffers found for render command");
		}
		
		for (int i = 0; i < cmd.batches.pass_batches_data.size(); i++) {
			const auto& batch = cmd.batches.pass_batches_data[i];
			if (i < cmd.batches.texture_bindings.size() && !cmd.batches.texture_bindings[i].empty()) {
				const auto& bindings = cmd.batches.texture_bindings[i];
				SDL_BindGPUFragmentSamplers(SDL_rp, safe_u32(rp.global_texture_bindings.size()), bindings.data(), static_cast<Uint32>(bindings.size()));
			}
			else {
				SDL_Log("no tex binds");
			}
			SDL_DrawGPUIndexedPrimitives(SDL_rp, batch.model->indexCount, batch.instanceCount, batch.model->indexOffset, batch.model->vertexOffset, batch.firstInstance);
		}

	}
}
