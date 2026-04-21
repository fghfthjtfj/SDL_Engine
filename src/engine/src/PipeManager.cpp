#include "PCH.h"
#include "PipeManager.h"
#include "RenderCommandData.h"
#include "ShaderManager.h"


PipeManager::PipeManager(SDL_GPUDevice* dev, SDL_Window* win) {
    this->win = win;
    this->dev = dev;
}

void PipeManager::CreateGraphicsPiplenes(ShaderManager* sm)
{
    if (!sm) {
		SDL_Log("PipeManager::ShaderManager pointer is null!");
        return;
	}
    if (!sm->IsDirtyGraphicsPipelines()) {
        return;
	}
    for (auto& pair : sm->GetShaderPrograms()) {
        ShaderProgram* sp = pair.second.get();
        SDL_GPUGraphicsPipeline* pipe = GetOrCreatePipeline(sp);
        if (!pipe) {
			SDL_Log("Failed to create pipeline for shader program: %s", pair.first.c_str());
        }
	}
	sm->SetDirtyGraphicsPipelines(false);
}

void PipeManager::CreateComputePipelines(ShaderManager* sm)
{
    if (!sm) {
        SDL_Log("PipeManager::ShaderManager pointer is null!");
        return;
    }
    if (!sm->IsDirtyComputePipelines()) {
        return;
    }
    for (auto& pair : sm->GetComputeShaderPrograms()) {
        ComputeShaderProgram* sp = pair.second.get();
		SDL_GPUComputePipeline* pipe = GetOrCreateComputePipeline(sp);
        if (!pipe) {
            SDL_Log("Failed to create compute pipeline for shader program: %s", pair.first.c_str());
		}
    }
}

SDL_GPUGraphicsPipeline* PipeManager::GetOrCreatePipeline(ShaderProgram* sp)
{
    auto it = graphics_pipelines.find(sp);
    if (it != graphics_pipelines.end()) {
		if (it->second == nullptr) {
			SDL_Log("Pipeline for given ShaderProgram is nullptr!");
        }
        return it->second;
    }

    SDL_GPUGraphicsPipelineCreateInfo pci;
    SDL_zero(pci);

    pci.vertex_shader = sp->vs.shader_data.shader;
    pci.fragment_shader = sp->fs.shader_data.shader;

    pci.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;
    pci.rasterizer_state.fill_mode = SDL_GPU_FILLMODE_FILL;
    pci.rasterizer_state.cull_mode = sp->spd->cull_mode;
    pci.rasterizer_state.front_face = SDL_GPU_FRONTFACE_COUNTER_CLOCKWISE;

    // оепемеярх RSPB б ьеидеп!
    pci.rasterizer_state.enable_depth_bias = sp->spd->associated_render_pass->rsb_params.enable_depth_bias;
    pci.rasterizer_state.depth_bias_constant_factor = sp->spd->associated_render_pass->rsb_params.depth_bias_constant_factor;
    pci.rasterizer_state.depth_bias_slope_factor = sp->spd->associated_render_pass->rsb_params.depth_bias_slope_factor;
    pci.rasterizer_state.depth_bias_clamp = sp->spd->associated_render_pass->rsb_params.depth_bias_clamp;



    pci.vertex_input_state.num_vertex_buffers = 1;
    pci.vertex_input_state.vertex_buffer_descriptions = &sp->vs.vb;
    pci.vertex_input_state.num_vertex_attributes = safe_u32(sp->vs.attributes.size());
    pci.vertex_input_state.vertex_attributes = sp->vs.attributes.data();

    pci.depth_stencil_state.enable_depth_test = sp->spd->enable_depth_test;
    pci.depth_stencil_state.enable_depth_write = sp->spd->enable_depth_write;
    pci.depth_stencil_state.enable_stencil_test = sp->spd->enable_stencil_test;
    pci.depth_stencil_state.compare_op = SDL_GPU_COMPAREOP_LESS;


    SDL_GPUColorTargetDescription ctd = MakeDefaultColorTarget();

    if (sp->spd->has_color_target) {
        pci.target_info.num_color_targets = 1;
        pci.target_info.color_target_descriptions = &ctd;
    }
    else {
        pci.target_info.num_color_targets = 0;
        pci.target_info.color_target_descriptions = nullptr;
    }

    pci.target_info.has_depth_stencil_target = true;
    pci.target_info.depth_stencil_format = SDL_GPU_TEXTUREFORMAT_D32_FLOAT;

	SDL_GPUGraphicsPipeline* pipe = nullptr;
    pipe = SDL_CreateGPUGraphicsPipeline(dev, &pci);

    if (!pipe) {
        SDL_LogError(
            SDL_LOG_CATEGORY_APPLICATION,
            "Pipeline creation failed: %s",
            SDL_GetError()
        );
        return nullptr;
    }

    graphics_pipelines.emplace(sp, pipe);
    return pipe;
}

SDL_GPUComputePipeline* PipeManager::GetOrCreateComputePipeline(ComputeShaderProgram* sp)
{
    auto it = compute_pipelines.find(sp);
    if (it != compute_pipelines.end()) {
        if (it->second == nullptr)
            SDL_Log("Pipeline for given ComputeShaderProgram is nullptr!");
        return it->second;
    }

    SDL_GPUComputePipelineCreateInfo ci;
    SDL_zero(ci);
    ci.code = sp->cs.spv_code;
	ci.code_size = sp->cs.spv_size;
    ci.entrypoint = "main";

    const SDL_GPUShaderFormat fmt = SDL_GetGPUShaderFormats(dev);
    if (fmt & SDL_GPU_SHADERFORMAT_SPIRV) ci.format = SDL_GPU_SHADERFORMAT_SPIRV;
    else if (fmt & SDL_GPU_SHADERFORMAT_DXIL)  ci.format = SDL_GPU_SHADERFORMAT_DXIL;
    else if (fmt & SDL_GPU_SHADERFORMAT_MSL)   ci.format = SDL_GPU_SHADERFORMAT_MSL;

    ci.num_samplers = sp->cs.num_samplers;
    ci.num_readonly_storage_textures = sp->cs.num_readonly_storage_textures;
    ci.num_readonly_storage_buffers = sp->cs.num_readonly_storage_buffers;
    ci.num_readwrite_storage_textures = sp->cs.num_readwrite_storage_textures;
    ci.num_readwrite_storage_buffers = sp->cs.num_readwrite_storage_buffers;
    ci.num_uniform_buffers = sp->cs.num_uniform_buffers;
    ci.threadcount_x = sp->cs.threadcount_x;
    ci.threadcount_y = sp->cs.threadcount_y;
    ci.threadcount_z = sp->cs.threadcount_z;

    SDL_GPUComputePipeline* pipeline = SDL_CreateGPUComputePipeline(dev, &ci);
    if (!pipeline)
        SDL_Log("Failed to create compute pipeline: %s", SDL_GetError());

    compute_pipelines.emplace(sp, pipeline);
    return pipeline;
}

SDL_GPUColorTargetDescription PipeManager::MakeDefaultColorTarget()
{
    SDL_GPUColorTargetDescription ctd{};
    SDL_zero(ctd);
    ctd.blend_state.enable_blend = true;
    ctd.blend_state.src_color_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA;
    ctd.blend_state.dst_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
    ctd.blend_state.color_blend_op = SDL_GPU_BLENDOP_ADD;
    ctd.blend_state.src_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE;
    ctd.blend_state.dst_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
    ctd.blend_state.alpha_blend_op = SDL_GPU_BLENDOP_ADD;
    ctd.format = SDL_GetGPUSwapchainTextureFormat(dev, win);
    return ctd;
}

SDL_GPUColorTargetDescription PipeManager::MakeNoColorTarget() {
	SDL_GPUColorTargetDescription ctd{};
	SDL_zero(ctd);
	return ctd;
}

SDL_GPUGraphicsPipeline* PipeManager::GetGraphicPipeline(ShaderProgram* sp)
{
    auto it = graphics_pipelines.find(sp);
    if (it != graphics_pipelines.end()) {
        if (it->second == nullptr) {
            SDL_Log("Pipeline for given ShaderProgram is nullptr!");
        }
        return it->second;
	}
	SDL_Log("PipeManager::Graphic pipeline not found for given ShaderProgram!");
	return nullptr;
}

SDL_GPUComputePipeline* PipeManager::GetComputePipeline(ComputeShaderProgram* sp)
{
    auto it = compute_pipelines.find(sp);
    if (it != compute_pipelines.end()) {
        if (it->second == nullptr) {
            SDL_Log("Pipeline for given ComputeShaderProgram is nullptr!");
			assert(it->second && "Compute pipeline should not be null here!");
		}
        return it->second;
    }
    SDL_Log("PipeManager::Compute pipeline not found for given ComputeShaderProgram!");
	return nullptr;
}

PipeManager::~PipeManager()
{
    for (auto& pair : graphics_pipelines) {
        if (pair.second) {
            SDL_ReleaseGPUGraphicsPipeline(dev, pair.second);
        }
        graphics_pipelines.clear();
    }
}

