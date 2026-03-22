#pragma once
#include <SDL3/SDL_gpu.h>
#include <SDL3/SDL.h>
#include <string_view>
#include <vector>
//#include "ShaderManager.h"
#include "TextureData.h"
#include "BufferManager.h"
#include "RenderCommandData.h"

struct Material;

class MaterialManager;
class PipeManager;
class ObjectManager;

class RenderManager
{
public:
    RenderManager();
	//RenderPass* CreateRenderPass(const std::string& name, std::vector<RenderCommandData>&& rcd, RenderPassTexturesData&& rptd);
	RenderPass* CreateRenderPass(const std::string& name, RenderPassTexturesData&& rptd, int pass_index);

	void FillRenderPasses(MaterialManager* mm, PipeManager* pm, ObjectManager* om);
	void ExecuteRenderPasses(SDL_GPUCommandBuffer* cb, BufferManager* buffer_manager, uint8_t render_frame);
	RenderPass* operator[](const std::string& name);
	~RenderManager();


private:
	void ExecuteRenderCommands(SDL_GPURenderPass* SDL_rp, const RenderPass& rp, BufferManager* bm, std::vector<RenderCommandData>& rcd, uint8_t render_frame);
	std::unordered_map<std::string, std::unique_ptr<RenderPass>> render_passes;
	std::vector<RenderPass*> ordered_passes;

};