#include "PCH.h"
#include "PipeManager.h"
#include "ShaderData.h"
#include "TextureManager.h"


PipeManager::PipeManager(SDL_GPUDevice* dev, SDL_Window* win) {
    this->win = win;
    this->dev = dev;
}

SDL_GPUGraphicsPipeline* PipeManager::GetOrCreatePipeline(ShaderProgram* sp)
{
    auto it = pipelines.find(sp);
    if (it != pipelines.end()) {
        return it->second;
    }

    SDL_GPUGraphicsPipelineCreateInfo pci;
    SDL_zero(pci);

    // ===== shaders =====
    pci.vertex_shader = sp->vs.shader_data.shader;
    pci.fragment_shader = sp->fs.shader_data.shader;

    // ===== primitive / rasterizer =====
    pci.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;
    pci.rasterizer_state.fill_mode = SDL_GPU_FILLMODE_FILL;
    pci.rasterizer_state.cull_mode = sp->cull_mode;
    pci.rasterizer_state.front_face = SDL_GPU_FRONTFACE_COUNTER_CLOCKWISE;

    // ===== vertex input =====
    pci.vertex_input_state.num_vertex_buffers = 1;
    pci.vertex_input_state.vertex_buffer_descriptions = &sp->vs.vb;
    pci.vertex_input_state.num_vertex_attributes =
        static_cast<Uint32>(sp->vs.attributes.size());
    pci.vertex_input_state.vertex_attributes =
        sp->vs.attributes.data();

    // ===== depth / stencil =====
    pci.depth_stencil_state.enable_depth_test = sp->enable_depth_test;
    pci.depth_stencil_state.enable_depth_write = sp->enable_depth_write;
    pci.depth_stencil_state.enable_stencil_test = sp->enable_stencil_test;
    pci.depth_stencil_state.compare_op = SDL_GPU_COMPAREOP_LESS;


    // ===== color / depth targets =====
    if (sp->has_color_target) {
        pci.target_info.num_color_targets = 1;
        pci.target_info.color_target_descriptions = &sp->ctd;
    }
    else {
        pci.target_info.num_color_targets = 0;
        pci.target_info.color_target_descriptions = nullptr;
    }

    pci.target_info.has_depth_stencil_target = true;
    pci.target_info.depth_stencil_format = SDL_GPU_TEXTUREFORMAT_D32_FLOAT;

    SDL_GPUGraphicsPipeline* pipe =
        SDL_CreateGPUGraphicsPipeline(dev, &pci);

    if (!pipe) {
        SDL_LogError(
            SDL_LOG_CATEGORY_APPLICATION,
            "Pipeline creation failed: %s",
            SDL_GetError()
        );
        return nullptr;
    }

    pipelines.emplace(sp, pipe);
    return pipe;
}


SDL_GPUColorTargetDescription PipeManager::MakeDefaultColorTarget()
{
    SDL_GPUColorTargetDescription ctd{};
    SDL_zero(ctd);
    ctd.blend_state.enable_blend = false;
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

PipeManager::~PipeManager()
{
    for (auto& pair : pipelines) {
        if (pair.second) {
            SDL_ReleaseGPUGraphicsPipeline(dev, pair.second);
        }
        pipelines.clear();
    }
}

