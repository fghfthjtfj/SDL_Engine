#include "PCH.h"
#include "DefaultShaderSet.h"
#include "ShaderManager.h"
#include "BufferManager.h"
#include "RenderManager.h"
#include "ObjectManager.h"
#include "LightDataModule.h"
#include "TransformDataModule.h"
#include "BatchBuilder.h"
#include "DefaultRenderPassSet.h"
#include "TextureManager.h"

namespace DefaultShaderProgramSet
{
    // render
    bool render_main_inited = false;
    bool render_shadow_inited = false;
    bool render_transparent_inited = false;
    
    // compute
    bool culling_zeros_inited = false;
    bool culling_count_inited = false;
    bool culling_offset_inited = false;
    bool culling_out_indirect_inited = false;
    bool culling_write_inited = false;

    bool shadow_blur_inited = false;
}

void DefaultShaderProgramSet::SetMainShaderProgram(BufferManager* bm, ShaderManager* sm, PassManager* pm)
{
    using namespace DefaultBuffersNames;
    if (render_main_inited) {
        SDL_Log("Main render shader programs already initialized.");
        return;
    }
    VertexShaderData vs = sm->CreateVertexShader("../engine/shaders_code/main_pass/main_pass.vert.hlsl");
    FragmentShaderData fs = sm->CreateFragmentShader("../engine/shaders_code/main_pass/main_pass.frag.hlsl");
	FragmentShaderData fs_debug = sm->CreateFragmentShader("../engine/shaders_code/main_pass/debug_pass.frag.hlsl");
    ShaderProgramDescription* spd_main =
        sm->CreateShaderProgramDescription("spd")
        ->UsedInRenderPass(pm->GetRenderPassStep(DefaultRenderPassNamespace::MAIN_PASS))
        ->BehavesAsOpaqueGeometry()->DoesNotCull()
        ;

    sm->CreateShaderProgram("sp", spd_main, bm,
        vs, { DEFAULT_TRANSFORM_BUFFER, DEFAULT_POSITION_INDEX_BUFFER, DEFAULT_CAMERA_BUFFER, DEFAULT_LIGHT_CAMERA_BUFFER },
        fs, { DEFAULT_LIGHT_BUFFER, DEFAULT_LIGHT_CAMERA_BUFFER },
        { TextureSlotRole::Albedo, TextureSlotRole::Normal }
    );

    render_main_inited = true;
 //   ShaderProgramDescription* spd_debug =
 //       sm->CreateShaderProgramDescription("spd_debug")
 //       ->UsedInRenderPass(pm->GetRenderPassStep("DEBUG_PASS"))
 //       ->BehavesAsOpaqueGeometry()->DoesNotCull()
 //       ;
 //
	//VertexShaderData vs_old = sm->CreateVertexShaderFromSPV("../engine/shaders_spv/main_pass.vert.spv");
	//FragmentShaderData fs_old = sm->CreateFragmentShaderFromSPV("../engine/shaders_spv/main_pass.frag.spv");
 //   sm->CreateShaderProgram("sp_debug", spd_debug, bm,
 //       vs, { DEFAULT_TRANSFORM_BUFFER, DEFAULT_POSITION_INDEX_BUFFER, DEFAULT_CAMERA_BUFFER, DEFAULT_LIGHT_CAMERA_BUFFER },
 //       fs_debug, { DEFAULT_LIGHT_BUFFER, DEFAULT_LIGHT_CAMERA_BUFFER },
 //       { TextureSlotRole::Albedo, TextureSlotRole::Normal }
 //   );

}

void DefaultShaderProgramSet::SetDefaultShadowShaderProgram(BufferManager* bm, ShaderManager* sm, PassManager* pm)
{
    using namespace DefaultBuffersNames;
    if (render_shadow_inited) {
        SDL_Log("Shadow render shader programs already initialized.");
        return;
    }
    VertexShaderData vs_2 = sm->CreateVertexShader("../engine/shaders_code/shadow_pass/shadow_pass.vert.hlsl");
    FragmentShaderData fs_2 = sm->CreateFragmentShader("../engine/shaders_code/shadow_pass/shadow_pass.frag.hlsl");
    RasterizerStateBiasParams shadow_rsbp = {};
    shadow_rsbp.enable_depth_bias = true;
    shadow_rsbp.depth_bias_constant_factor = 1.0f;   // подбираешь
    shadow_rsbp.depth_bias_slope_factor = 2.0f;   // подбираешь
    shadow_rsbp.depth_bias_clamp = 0.0f;
    ShaderProgramDescription* spd_shadow =
        sm->CreateShaderProgramDescription("spd_shadow")
        ->UsedInRenderPass(pm->GetRenderPassStep(DefaultRenderPassNamespace::SHADOW_PASS))
        ->BehavesAsShadowCaster()->DoesNotCull()
        //->WithDepthBias({ shadow_rsbp })
        ;

    ShaderProgram* sp_shadow = sm->CreateShaderProgram("sp_shadow", spd_shadow, bm,
        vs_2, { DEFAULT_TRANSFORM_BUFFER, DEFAULT_POSITION_INDEX_BUFFER, DEFAULT_LIGHT_CAMERA_BUFFER },
        fs_2, {},
        {}
    );

 //   ShaderProgramDescription* spd_shadow_old =
 //       sm->CreateShaderProgramDescription("spd_shadow_old")
 //       ->UsedInRenderPass(pm->GetRenderPassStep("DEBUG_SHADOW_PASS"))
 //       ->BehavesAsShadowCaster()->DoesNotCull()
 //       //->WithDepthBias({ shadow_rsbp })
 //       ;
	//VertexShaderData vs_shadow_old = sm->CreateVertexShaderFromSPV("../engine/shaders_spv/shadow_pass.vert.spv");
	//FragmentShaderData fs_shadow_old = sm->CreateFragmentShaderFromSPV("../engine/shaders_spv/shadow_pass.frag.spv");
 //   sm->CreateShaderProgram("sp_shadow_old", spd_shadow_old, bm,
 //       vs_shadow_old, { DEFAULT_TRANSFORM_BUFFER, DEFAULT_POSITION_INDEX_BUFFER, DEFAULT_LIGHT_CAMERA_BUFFER },
 //       fs_shadow_old, {},
 //       {}
	//);
    render_shadow_inited = true;
}

void DefaultShaderProgramSet::SetTransparentShaderProgram(BufferManager* bm, ShaderManager* sm, PassManager* pm)
{
    using namespace DefaultBuffersNames;
    if (render_transparent_inited) {
        SDL_Log("Transparent render shader programs already initialized.");
        return;
    }

}

void DefaultShaderProgramSet::SetCullingZerosPrograms(BufferManager* bm, ShaderManager* sm, PassManager* pm)
{
    using namespace DefaultBuffersNames;
    if (culling_zeros_inited) {
        SDL_Log("Culling zeros programs already initialized.");
        return;
    }

    ComputeShaderData csd_zeros = sm->CreateComputeShader(
        "../engine/shaders/culling_clear.comp.spv");

    sm->CreateComputeShaderProgram("csp_zeros", csd_zeros,
        { bm->GetBufferData(DEFAULT_COUNT_BUFFER) },  // rw_storage_buffers
        {},                        // ro_storage_buffers
        {},                        // ro_storage_textures
        {},                        // rw_storage_textures
        {},                        // texture_samplers
        pm->GetComputePrepassStep(DefaultRenderPassNamespace::CULLING_ZEROS_PREPASS));

    culling_zeros_inited = true;
}

void DefaultShaderProgramSet::SetCullingCountPrograms(
    BufferManager* bm, ShaderManager* sm, PassManager* pm,
    CameraManager* cm, ObjectManager* om, BatchBuilder* bb,
    LightDataModule* ldm)
{
    using namespace DefaultBuffersNames;
    if (culling_count_inited) {
        SDL_Log("Culling count programs already initialized.");
        return;
    }

    ComputeShaderData csd = sm->CreateComputeShader(
        "../engine/shaders/culling_count.comp.spv");

    // Main camera
    ComputeShaderProgram* cs_main = sm->CreateComputeShaderProgram(
        "compute_sp_main", csd,
        { bm->GetBufferData(DEFAULT_COUNT_BUFFER) },                    // rw
        {
            bm->GetBufferData(DEFAULT_POSITION_INDEX_BUFFER),
            bm->GetBufferData(DEFAULT_ENTITY_TO_BATCH_BUFFER),
            bm->GetBufferData(DEFAULT_BOUND_SPHERE_BUFFER),
            bm->GetBufferData(DEFAULT_CAMERA_BUFFER),
            bm->GetBufferData(DEFAULT_TRANSFORM_BUFFER)
        },                                                              // ro
        {}, {}, {},
        pm->GetComputePrepassStep(DefaultRenderPassNamespace::CULLING_PREPASS));

    cs_main->BindPushConstants<DefaultRenderPassNamespace::ComputeCullingCountUniform>(
        [cm](const PushConstantBinder& binder, DefaultRenderPassNamespace::ComputeCullingCountUniform data) {
        data.num_cameras = 1;
        data.cmd_offset = 0;
        binder.Push(0, data);
    });

    // Light cameras
    ComputeShaderProgram* cs_light = sm->CreateComputeShaderProgram(
        "compute_sp_light", csd,
        { bm->GetBufferData(DEFAULT_COUNT_BUFFER) },                    // rw
        {
            bm->GetBufferData(DEFAULT_POSITION_INDEX_BUFFER),
            bm->GetBufferData(DEFAULT_ENTITY_TO_BATCH_BUFFER),
            bm->GetBufferData(DEFAULT_BOUND_SPHERE_BUFFER),
            bm->GetBufferData(DEFAULT_LIGHT_CAMERA_BUFFER),
            bm->GetBufferData(DEFAULT_TRANSFORM_BUFFER)
        },                                                              // ro
        {}, {}, {},
        pm->GetComputePrepassStep(DefaultRenderPassNamespace::CULLING_PREPASS));

    cs_light->BindPushConstants<DefaultRenderPassNamespace::ComputeCullingCountUniform>(
        [ldm, om, bb](const PushConstantBinder& binder, DefaultRenderPassNamespace::ComputeCullingCountUniform data) {
        data.num_cameras = ldm->AskNumLightCameras(om, om->GetActiveScene());
        data.cmd_offset = bb->AskNumCommands() * 1;
        binder.Push(0, data);
    });

    culling_count_inited = true;
}

void DefaultShaderProgramSet::SetCullingOffsetPrograms(
    BufferManager* bm, ShaderManager* sm, PassManager* pm)
{
    using namespace DefaultBuffersNames;
    if (culling_offset_inited) {
        SDL_Log("Culling offset programs already initialized.");
        return;
    }

    ComputeShaderData csd_offset = sm->CreateComputeShader(
        "../engine/shaders/culling_offsets.comp.spv");

    sm->CreateComputeShaderProgram("csp_offsets", csd_offset,
        { bm->GetBufferData(DEFAULT_OFFSET_BUFFER) },      // rw
        { bm->GetBufferData(DEFAULT_COUNT_BUFFER) },       // ro
        {}, {}, {},
        pm->GetComputePrepassStep(DefaultRenderPassNamespace::CULLING_OFFSET_PREPASS));

    culling_offset_inited = true;
}

void DefaultShaderProgramSet::SetCullingOutIndirectPrograms(
    BufferManager* bm, ShaderManager* sm, PassManager* pm,
    ObjectManager* om, BatchBuilder* bb, LightDataModule* ldm)
{
    using namespace DefaultBuffersNames;
    if (culling_out_indirect_inited) {
        SDL_Log("Culling out indirect programs already initialized.");
        return;
    }

    ComputeShaderData csd_out = sm->CreateComputeShader(
        "../engine/shaders/culling_out_indirect.comp.spv");

    // Main camera
    ComputeShaderProgram* csp_out_main = sm->CreateComputeShaderProgram(
        "csp_out_indirect", csd_out,
        { bm->GetBufferData(DEFAULT_OUT_INDIRECT_BUFFER) },   // rw
        {
            bm->GetBufferData(DEFAULT_INDIRECT_BUFFER),
            bm->GetBufferData(DEFAULT_COUNT_BUFFER),
            bm->GetBufferData(DEFAULT_OFFSET_BUFFER)
        },                                                    // ro
        {}, {}, {},
        pm->GetComputePrepassStep(DefaultRenderPassNamespace::CULLING_OUT_INDIRECT_PREPASS));

    csp_out_main->BindPushConstants<DefaultRenderPassNamespace::ComputeCullingOutIndirectUniform>(
        [bb](const PushConstantBinder& binder, DefaultRenderPassNamespace::ComputeCullingOutIndirectUniform data) {
        data.num_commands = bb->AskNumCommands();
        data.num_cameras = 1;
        data.cmd_offset = 0;
        binder.Push(0, data);
    });

    // Light cameras
    ComputeShaderProgram* csp_out_light = sm->CreateComputeShaderProgram(
        "csp_out_indirect_lights", csd_out,
        { bm->GetBufferData(DEFAULT_OUT_INDIRECT_BUFFER) },   // rw
        {
            bm->GetBufferData(DEFAULT_INDIRECT_BUFFER),
            bm->GetBufferData(DEFAULT_COUNT_BUFFER),
            bm->GetBufferData(DEFAULT_OFFSET_BUFFER)
        },                                                    // ro
        {}, {}, {},
        pm->GetComputePrepassStep(DefaultRenderPassNamespace::CULLING_OUT_INDIRECT_PREPASS));

    csp_out_light->BindPushConstants<DefaultRenderPassNamespace::ComputeCullingOutIndirectUniform>(
        [ldm, om, bb](const PushConstantBinder& binder, DefaultRenderPassNamespace::ComputeCullingOutIndirectUniform data) {
        data.num_commands = bb->AskNumCommands();
        data.num_cameras = ldm->AskNumLightCameras(om, om->GetActiveScene());
        data.cmd_offset = bb->AskNumCommands() * 1;
        binder.Push(0, data);
    });

    culling_out_indirect_inited = true;
}

void DefaultShaderProgramSet::SetCullingWritePrograms(
    BufferManager* bm, ShaderManager* sm, PassManager* pm,
    CameraManager* cm, ObjectManager* om, BatchBuilder* bb,
    LightDataModule* ldm)
{
    using namespace DefaultBuffersNames;
    if (culling_write_inited) {
        SDL_Log("Culling write programs already initialized.");
        return;
    }

    ComputeShaderData csd_write = sm->CreateComputeShader(
        "../engine/shaders/culling_write.comp.spv");

    // Main camera
    ComputeShaderProgram* csp_main = sm->CreateComputeShaderProgram(
        "csp_culling_write_main", csd_write,
        {
            bm->GetBufferData(DEFAULT_OFFSET_BUFFER),
            bm->GetBufferData(DEFAULT_OUT_TRANSFORM_BUFFER)
        },  // rw
        {
            bm->GetBufferData(DEFAULT_POSITION_INDEX_BUFFER),
            bm->GetBufferData(DEFAULT_ENTITY_TO_BATCH_BUFFER),
            bm->GetBufferData(DEFAULT_BOUND_SPHERE_BUFFER),
            bm->GetBufferData(DEFAULT_CAMERA_BUFFER),
            bm->GetBufferData(DEFAULT_TRANSFORM_BUFFER)
        },  // ro
        {}, {}, {},
        pm->GetComputePassStep(DefaultRenderPassNamespace::CULLING_WRITE_PASS));

    csp_main->BindPushConstants<DefaultRenderPassNamespace::ComputeCullingCountUniform>(
        [cm](const PushConstantBinder& binder, DefaultRenderPassNamespace::ComputeCullingCountUniform data) {
        data.num_cameras = 1;
        data.cmd_offset = 0;
        binder.Push(0, data);
    });

    // Light cameras
    ComputeShaderProgram* csp_light = sm->CreateComputeShaderProgram(
        "csp_culling_write_light", csd_write,
        {
            bm->GetBufferData(DEFAULT_OFFSET_BUFFER),
            bm->GetBufferData(DEFAULT_OUT_TRANSFORM_BUFFER)
        },  // rw
        {
            bm->GetBufferData(DEFAULT_POSITION_INDEX_BUFFER),
            bm->GetBufferData(DEFAULT_ENTITY_TO_BATCH_BUFFER),
            bm->GetBufferData(DEFAULT_BOUND_SPHERE_BUFFER),
            bm->GetBufferData(DEFAULT_LIGHT_CAMERA_BUFFER),
            bm->GetBufferData(DEFAULT_TRANSFORM_BUFFER)
        },  // ro
        {}, {}, {},
        pm->GetComputePassStep(DefaultRenderPassNamespace::CULLING_WRITE_PASS));

    csp_light->BindPushConstants<DefaultRenderPassNamespace::ComputeCullingCountUniform>(
        [ldm, om, bb](const PushConstantBinder& binder, DefaultRenderPassNamespace::ComputeCullingCountUniform data) {
        data.num_cameras = ldm->AskNumLightCameras(om, om->GetActiveScene());
        data.cmd_offset = bb->AskNumCommands() * 1;
        binder.Push(0, data);
    });

    culling_write_inited = true;
}

void DefaultShaderProgramSet::SetShadowBlurPrograms(
    BufferManager* bm, ShaderManager* sm, PassManager* pm,
    TextureManager* tm, ObjectManager* om, LightDataModule* ldm)
{
    using namespace DefaultRenderPassNamespace;
    if (shadow_blur_inited) {
        SDL_Log("Shadow blur programs already initialized.");
        return;
    }

    ComputeShaderData csd_h = sm->CreateComputeShader(
        "../engine/shaders_code/comp/shadow_blur_h.comp.hlsl");
    ComputeShaderData csd_v = sm->CreateComputeShader(
        "../engine/shaders_code/comp/shadow_blur_v.comp.hlsl");

    auto moments_atlas = tm->GetTextureAtlas(SHADOW_MOMENTS_ARRAY);
    auto blur_temp_atlas = tm->GetTextureAtlas(SHADOW_MOMENTS_BLUR_TEMP);

    uint32_t LAYER_COUNT = tm->GetTextureAtlas(SHADOW_MOMENTS_ARRAY)->layers;

    for (uint32_t L = 0; L < LAYER_COUNT; ++L) {
        // ───── H для слоя L: moments_array[L] (sampled) → blur_temp[0] (rw) ─────
        std::string name_h = "csp_shadow_blur_h_" + std::to_string(L);
        ComputeShaderProgram* csp_h = sm->CreateComputeShaderProgram(name_h, csd_h,
            {},                                                              // rw buffers
            {},                                                              // ro buffers
            { { blur_temp_atlas, /*mip*/0, /*layer*/0 } },                     // rw textures
            {},                                                              // ro storage textures
            { moments_atlas },                                               // sampled textures
            pm->GetComputePassStep(SHADOW_BLUR_PASS));

        csp_h->BindPushConstants<ShadowBlurUniform>(
            [L](const PushConstantBinder& binder, ShadowBlurUniform data) {
            data.layerIndex = L;
            binder.Push(0, data);
        });
        csp_h->BindDispatch<DummyDispatchData>(
            [L, om, blur_temp_atlas, ldm](DispatchSizeBinder& binder, DummyDispatchData) {
            if (ldm->IsShadowLayerDirty(om, L)) {
                binder.element_count = {
                    blur_temp_atlas->width,
                    blur_temp_atlas->height,
                    1
                };
            }
            else {
                binder.element_count = { 0, 0, 0 };
            }
        });

        // ───── V для слоя L: blur_temp[0] (sampled) → moments_array[L] (rw) ─────
        std::string name_v = "csp_shadow_blur_v_" + std::to_string(L);
        ComputeShaderProgram* csp_v = sm->CreateComputeShaderProgram(name_v, csd_v,
            {},
            {},
            { { moments_atlas, /*mip*/0, /*layer*/L } },
            {},
            { blur_temp_atlas },
            pm->GetComputePassStep(SHADOW_BLUR_PASS));

        csp_v->BindDispatch<DummyDispatchData>(
            [L, om, moments_atlas, ldm](DispatchSizeBinder& binder, DummyDispatchData) {
            if (ldm->IsShadowLayerDirty(om, L)) {
                binder.element_count = {
                    moments_atlas->width,
                    moments_atlas->height,
                    1
                };
            }
            else {
                binder.element_count = { 0, 0, 0 };
            }
        });
    }

    shadow_blur_inited = true;
}