#include "PCH.h"
#include "RenderManager.h"
#include "BufferManager.h"
#include "ModelData.h"
#include "MaterialManager.h"
#include "PipeManager.h"
#include "ObjectManager.h"

PassManager::PassManager() {}

RenderPassStep* PassManager::CreateRenderPass(const ComputePassName& name, std::function<void(SDL_GPUCommandBuffer*, PassManager*, RenderPassStep&)> render_function, RenderPassTexturesInfo&& rptd, int pass_index, RasterizerStateBiasParams& RSB_Params)
{
	if (pass_index == -1) {
		SDL_Log("PassManager::CreateRenderPass: Invalid RenderPassStep order index.");
		return nullptr;
	}

	auto data = std::make_unique<RenderPassStep>();
	data->renderPassTexsData = std::move(rptd);
	data->render_function = render_function;
	data->pass_index = pass_index;
	data->rsb_params = RSB_Params;

	RenderPassStep* ptr = data.get();
	render_steps[name] = std::move(data);

	return ptr;
}

ComputePassStep* PassManager::CreateComputePass(const ComputePassName& name, std::function<void(SDL_GPUCommandBuffer*, PassManager*, ComputePassStep&, uint8_t)> compute_function, int pass_index)
 {
	if (pass_index == -1) {
		SDL_Log("PassManager::CreateComputePass: Invalid ComputePassStep order index.");
		return nullptr;
	}
	auto data = std::make_unique<ComputePassStep>();
	data->compute_function = compute_function;
	data->pass_index = pass_index;

	ComputePassStep* ptr = data.get();
	compute_steps[name] = std::move(data);
	return ptr;
}

ComputePassStep* PassManager::CreateComputePrepass(const ComputePrepassName& name, std::function<void(SDL_GPUCommandBuffer*, PassManager*, ComputePassStep&, uint8_t)> compute_function, int pass_index)
{
	if (pass_index == -1) {
		SDL_Log("PassManager::CreateComputePass: Invalid ComputePassStep order index.");
		return nullptr;
	}
	auto data = std::make_unique<ComputePassStep>();
	data->compute_function = compute_function;
	data->pass_index = pass_index;

	ComputePassStep* ptr = data.get();
	compute_prepass_steps[name] = std::move(data);
	return ptr;
}

void PassManager::FillRenderPasses()
{
	ordered_passes.clear();
	ordered_passes.reserve(render_steps.size());
	for (auto& [_, rp] : render_steps)
		ordered_passes.push_back(rp.get());

	ordered_compute_steps.clear();
	ordered_compute_steps.reserve(compute_steps.size());
	for (auto& [_, cs] : compute_steps)
		ordered_compute_steps.push_back(cs.get());

	ordered_compute_prepass_steps.clear();
	ordered_compute_prepass_steps.reserve(compute_prepass_steps.size());
	for (auto& [_, pcs] : compute_prepass_steps)
		ordered_compute_prepass_steps.push_back(pcs.get());

	std::sort(ordered_passes.begin(), ordered_passes.end(),
		[](const RenderPassStep* a, const RenderPassStep* b) {
		return a->pass_index < b->pass_index;
	});

	std::sort(ordered_compute_steps.begin(), ordered_compute_steps.end(),
		[](const ComputePassStep* a, const ComputePassStep* b) {
		return a->pass_index < b->pass_index;
	});

	std::sort(ordered_compute_prepass_steps.begin(), ordered_compute_prepass_steps.end(),
		[](const ComputePassStep* a, const ComputePassStep* b) {
		return a->pass_index < b->pass_index;
	});
}

void PassManager::ExecutePassesSteps(SDL_GPUCommandBuffer* cb, uint8_t pass_frame)
{
	int i = 0, j = 0;
	while (i < ordered_passes.size() || j < ordered_compute_steps.size()) {
		if (i < ordered_passes.size() && (j >= ordered_compute_steps.size() || ordered_passes[i]->pass_index <= ordered_compute_steps[j]->pass_index)) {
			ordered_passes[i]->render_function(cb, this, *ordered_passes[i]);
			i++;
		}
		else if (j < ordered_compute_steps.size()) {
			ordered_compute_steps[j]->compute_function(cb, this, *ordered_compute_steps[j], pass_frame);
			j++;
		}
	}
}

void PassManager::ExecutePrepassesSteps(SDL_GPUCommandBuffer* cb, uint8_t pass_frame)
{
	for (auto& step : ordered_compute_prepass_steps) {
		step->compute_function(cb, this, *step, pass_frame);
	}
}

void PassManager::RenderPassStandardBody(SDL_GPUCommandBuffer* cb, RenderPassStep* render_pass_step, BufferManager* bm)
{
	SDL_GPURenderPass* rp = nullptr;
		rp = SDL_BeginGPURenderPass(cb,
			&render_pass_step->renderPassTexsData.colorTargetInfo,
			render_pass_step->renderPassTexsData.numColorTargets,
			&render_pass_step->renderPassTexsData.depthTargetInfo);
		if (!rp) {
			SDL_Log("PassManager::ExecutePassesSteps: Failed to begin render pass!");
			return;
		}
		ExecuteRenderBatches(cb, rp, *render_pass_step, bm);
		SDL_EndGPURenderPass(rp);
	
}

void PassManager::WaitComputePrepass(SDL_GPUDevice* dev)
{
	SDL_WaitForGPUIdle(dev);
}

void PassManager::ComputePassStandardBody(SDL_GPUCommandBuffer* cb, ComputePassStep* compute_step, BufferManager* bm, const void* raw, uint8_t pass_frame)
{
	for (auto& [_, shader_batch] : compute_step->shader_batches) {
		if (shader_batch.push_func) {
			PushConstantBinder binder{ cb };
			shader_batch.push_func(binder, raw);
		}

		SDL_GPUComputePass* cmp = nullptr;
		std::vector<SDL_GPUStorageBufferReadWriteBinding> storage_buffer_bindings = bm->BuildBindGPUComputeRWBuffers(shader_batch.rw_storage_buffers, pass_frame);

		cmp = SDL_BeginGPUComputePass(cb, nullptr, 0, storage_buffer_bindings.data(), safe_u32(storage_buffer_bindings.size()));
		SDL_BindGPUComputePipeline(cmp, shader_batch.pipeline);
		bm->BindGPUComputeRO_Buffers(cmp, 0, shader_batch.ro_storage_buffers, pass_frame);
		SDL_DispatchGPUCompute(cmp, 1, 1, 1);
		SDL_EndGPUComputePass(cmp);
	};
}

RenderPassStep* PassManager::GetRenderPassStep(const RenderPassName& name)
{
	auto it = render_steps.find(name);
	if (it != render_steps.end()) {
		return it->second.get();
	}
	SDL_Log("Render pass '%s' not found", name.c_str());
	return nullptr;
}

ComputePassStep* PassManager::GetComputePassStep(const ComputePassName& name)
{
	auto it = compute_steps.find(name);
	if (it != compute_steps.end()) {
		return it->second.get();
	}
	SDL_Log("Compute pass '%s' not found", name.c_str());
	return nullptr;
}

ComputePassStep* PassManager::GetComputePrepassStep(const ComputePrepassName& name)
{
	auto it = compute_prepass_steps.find(name);
	if (it != compute_prepass_steps.end()) {
		return it->second.get();
	}
	SDL_Log("Compute prepass '%s' not found", name.c_str());
	return nullptr;
}

PassManager::~PassManager()
{
	render_steps.clear();
}

inline void PassManager::ExecuteRenderBatches(SDL_GPUCommandBuffer* cb, SDL_GPURenderPass* rp, const RenderPassStep& render_pass_step, BufferManager* bm)
{
	int draw_calls = 0;
	for (auto& [_, shader_batch] : render_pass_step.shader_batches)
	{
		SDL_BindGPUGraphicsPipeline(rp, shader_batch.pipeline);
		SDL_BindGPUFragmentSamplers(rp, 0, render_pass_step.global_texture_bindings.data(), safe_u32(render_pass_step.global_texture_bindings.size()));

		bm->BindGPUVertexBuffer(rp, 0, 0);
		bm->BindGPUIndexBuffer(rp, 0);

		if (!shader_batch.vertexStorageBuffers.empty()) {
			bm->BindGPUVertexStorageBuffers(rp, 0, shader_batch.vertexStorageBuffers, render_frame);
		}
		else {
			//SDL_Log("No vertex storage buffers found for render command");
		}
		if (!shader_batch.fragmentStorageBuffers.empty()) {
			bm->BindGPUFragmentStorageBuffers(rp, 0, shader_batch.fragmentStorageBuffers, render_frame);
		}
		else {
			//SDL_Log("No fragment storage buffers found for render command");
		}

		for (auto& [_, atlas_batch] : shader_batch.atlases_batches) {
			if (!atlas_batch.texture_binding.empty()) {
				SDL_BindGPUFragmentSamplers(rp, safe_u32(render_pass_step.global_texture_bindings.size()), atlas_batch.texture_binding.data(), safe_u32(atlas_batch.texture_binding.size()));
			}
			for (auto& [_, texture_batch] : atlas_batch.texture_batches) {
				if (!texture_batch.texture_uvl.empty()) {
					TextureData padded[4] = {};
					size_t count = std::min(texture_batch.texture_uvl.size(), size_t(4));
					for (size_t i = 0; i < count; i++) {
						if (texture_batch.texture_uvl[i])
							padded[i] = *texture_batch.texture_uvl[i];
					}
					SDL_PushGPUFragmentUniformData(cb, 0, padded, sizeof(padded));
				}
				else {
					//SDL_Log("Texture batch UVL data does not exist or is empty");
				}

				SDL_DrawGPUIndexedPrimitivesIndirect(rp,
					bm->_GetGPUBufferForFrame(bm->GetBufferData(DEFAULT_INDIRECT_BUFFER), render_frame),
					safe_u32(texture_batch.indirect_command_index * sizeof(SDL_GPUIndexedIndirectDrawCommand)),
					safe_u32(texture_batch.model_batches.size())
				);
				draw_calls++;

				
			}
		}
	}
	//std::cout << "Draw calls: " << draw_calls << " for pass" << render_pass_step.pass_index << std::endl;
}
