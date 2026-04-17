#pragma once
#include <SDL3/SDL_gpu.h>
#include <SDL3/SDL.h>
#include <string_view>
#include <vector>
//#include "ShaderManager.h"
#include "TextureData.h"
#include "BufferManager.h"
#include "RenderCommandData.h"
#include "Aliases.h"

struct Material;

class MaterialManager;
class PipeManager;
class ObjectManager;

inline constexpr const char* MAIN_PASS = "_DefaultMainRenderPass";
inline constexpr const char* SHADOW_PASS = "_DefaultShadowRenderPass";
inline constexpr const char* CULLING_PREPASS = "_DefaultCullingComputePass";
inline constexpr const char* CULLING_ZEROS_PREPASS = "_DefaultCullingZerosComputePass";
inline constexpr const char* CULLING_OFFSET_PREPASS = "_DefaultCullingOffsetComputePass";
inline constexpr const char* CULLING_OUT_INDIRECT_PREPASS = "_DefaultCullingOutIndirectComputePass";
inline constexpr const char* CULLING_WRITE_PASS = "_DefaultCullingWritePass";


class PassManager
{
public:
    PassManager();
	RenderPassStep* CreateRenderPass(const ComputePassName& name, std::function<void(SDL_GPUCommandBuffer*, PassManager*, RenderPassStep&)> render_function, RenderPassTexturesInfo&& rptd, int pass_index, RasterizerStateBiasParams& RSB_Params);
	ComputePassStep* CreateComputePass(const ComputePassName& name, std::function<void(SDL_GPUCommandBuffer*, PassManager*, ComputePassStep&, uint8_t)> compute_function, int pass_index);
	ComputePassStep* CreateComputePrepass(const ComputePrepassName& name, std::function<void(SDL_GPUCommandBuffer*, PassManager*, ComputePassStep&, uint8_t)> compute_function, int pass_index);

	void FillRenderPasses();
	void ExecutePassesSteps(SDL_GPUCommandBuffer* cb, uint8_t pass_frame);
	void ExecutePrepassesSteps(SDL_GPUCommandBuffer* cb, uint8_t pass_frame);
	// Íà÷èíàåò è çàâåğøàåò SDL_GPURenderPass
	// Starts and end SDL_GPURenderPass
	void RenderPassStandardBody(SDL_GPUCommandBuffer* cb, RenderPassStep* render_pass, BufferManager* bm, uint32_t additional_offset); // ÄÎÁÀÂÈÒÜ ÀÍÀËÎÃÈ×ÍÓŞ COMPUTE STEP ËÎÃÈÊÓ ÊÎÍÑÒÀÍÒ!
	void WaitComputePrepass(SDL_GPUDevice* dev);
	// Íà÷èíàåò è çàâåğøàåò SDL_GPUComputePass
	// Starts and end SDL_GPUComputePass
	void ComputePassStandardBody(SDL_GPUCommandBuffer* cb, ComputePassStep* compute_pass, BufferManager* bm, const void* raw, uint8_t pass_frame);

	void SetRenderFrame(uint8_t frame) { render_frame = frame; }
	RenderPassStep* GetRenderPassStep(const RenderPassName& name);
	ComputePassStep* GetComputePassStep(const ComputePassName& name);
	ComputePassStep* GetComputePrepassStep(const ComputePrepassName& name);

	const std::vector<RenderPassStep*>& GetOrderedRenderPasses() { return ordered_passes; }
	const std::vector<ComputePassStep*>& GetOrderedComputePasses() { return ordered_compute_steps; }
	const std::vector<ComputePassStep*>& GetOrderedComputePrepasses() { return ordered_compute_prepass_steps; }

	~PassManager();

private:
	inline void ExecuteRenderBatches(SDL_GPUCommandBuffer* cb, SDL_GPURenderPass* SDL_rp, const RenderPassStep& rp, BufferManager* bm, uint32_t additional_offset);
	std::unordered_map<RenderPassName, std::unique_ptr<RenderPassStep>> render_steps;
	std::unordered_map<ComputePassName, std::unique_ptr<ComputePassStep>> compute_steps;
	std::unordered_map<ComputePrepassName, std::unique_ptr<ComputePassStep>> compute_prepass_steps;

	std::vector<RenderPassStep*> ordered_passes;
	std::vector<ComputePassStep*> ordered_compute_steps;
	std::vector<ComputePassStep*> ordered_compute_prepass_steps;

	uint8_t render_frame = 0;
};