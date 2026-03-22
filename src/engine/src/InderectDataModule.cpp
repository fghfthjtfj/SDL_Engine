#include "PCH.h"
#include "InderectDataModule.h"
#include "ObjectManager.h"
#include "BufferManager.h"
#include "RenderManager.h"
#include "ModelData.h"

InderectDataModule::InderectDataModule()
{
}

uint32_t InderectDataModule::CalculateIndirectSize(PassManager* pm)
{
	if (!dirty) {
		return 0;
	}
	total_size = 0;
	const std::vector<RenderPassStep*>& render_passes = pm->GetOrderedRenderPasses();

	for (const RenderPassStep* rp : render_passes) {
		for (const auto& [_, shader_batch] : rp->shader_batches) {
			for (const auto& [_, atlas_batch] : shader_batch.atlases_batches) {
				for (const auto& [_, texture_batch] : atlas_batch.texture_batches) {
					for (const auto& [_, model_batch] : texture_batch.model_batches) {
						total_size += sizeof(SDL_GPUIndexedIndirectDrawCommand);
					}
				}
			}
		}
	}

	return total_size;
}

void InderectDataModule::StoreIndirect(BufferManager* bm, PassManager* pm, UploadTask* task)
{
	const std::vector<RenderPassStep*>& render_passes = pm->GetOrderedRenderPasses();

	for (const RenderPassStep* rp : render_passes) {
		for (const auto& [_, shader_batch] : rp->shader_batches) {
			for (const auto& [_, atlas_batch] : shader_batch.atlases_batches) {
				for (const auto& [_, texture_batch] : atlas_batch.texture_batches) {
					for (const auto& [_, model_batch] : texture_batch.model_batches) {
						SDL_GPUIndexedIndirectDrawCommand data;
						data.num_indices = model_batch.submesh->indexCount;
						data.num_instances = model_batch.instanceCount;
						data.first_index = model_batch.submesh->indexOffset;
						data.vertex_offset = model_batch.submesh->vertexOffset;
						data.first_instance = model_batch.firstInstance;

						bm->UploadToTransferBuffer(task, sizeof(data), &data);
					}
				}
			}
		}
	}
	dirty = false;

}

uint32_t InderectDataModule::AskNumCommands(PassManager* pm)
{
	uint32_t num_model_batches = 0;
	const std::vector<RenderPassStep*>& render_passes = pm->GetOrderedRenderPasses();

	for (const RenderPassStep* rp : render_passes) {
		for (const auto& [_, shader_batch] : rp->shader_batches) {
			for (const auto& [_, atlas_batch] : shader_batch.atlases_batches) {
				for (const auto& [_, texture_batch] : atlas_batch.texture_batches) {
					num_model_batches += safe_u32(texture_batch.model_batches.size());
				}
			}
		}
	}

	return num_model_batches;
}
