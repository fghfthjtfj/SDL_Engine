#include "DefaultRenderPassSet.h"
#include "TextureManager.h"
#include "RenderManager.h"
#include "TexturesPresets.h"
#include "TextureSamplerPresets.h"
#include "ObjectManager.h"
#include "TransformDataModule.h"
#include "LightDataModule.h"
#include "InderectDataModule.h"
#include "BatchBuilder.h"

namespace DefaultRenderPassNamespace
{
    static TextureAtlas* shadow_moments_array = nullptr;
    static TextureAtlas* shadow_depth_flat_array = nullptr;


    bool shadow_pass_inited = false;
	bool main_pass_inited = false;
}

void DefaultRenderPassNamespace::SetDefaultShadowPCFRenderPass(PassManager* rm, TextureManager* tm, BufferManager* bm, ObjectManager* om, BatchBuilder* bb)
{
    if (shadow_pass_inited) {
        SDL_Log("Default shadow render pass is already initialized.");
        return;
    }
    auto shadow_sampler = tm->GetSampler(DEFAULT_SHADOW_SAMPLER);

    shadow_depth_flat_array = tm->CreateTextureAtlas(SHADOW_DEPTH_FLAT_ARRAY, TexturePresets::GetCreateInfo(TexturePreset::Depth_FlatArray2048_8Layers), shadow_sampler);
    TextureAtlas* shadow_temp = tm->CreateTextureAtlas("shadow_depth_single_temp", TexturePresets::GetCreateInfo(TexturePreset::TempDepth2048), shadow_sampler);

    RenderPassTexturesInfo shadow_rptd{};
    shadow_rptd.CreateDepthTextureInfo(SDL_GPU_LOADOP_CLEAR, SDL_GPU_STOREOP_STORE, shadow_depth_flat_array->format);

    auto shadowPass = rm->CreateRenderPass(
        SHADOW_PASS,
        [rm, bm, om, tm](SDL_GPUCommandBuffer* cb, PassManager* pm, RenderPassStep& rp)
    {
        Uint32 cameraIndex = 0;      // îáùèé äëÿ âñåõ òèïîâ ñâåòà
        Uint32 sphereLayer = 0;      // òîëüêî äëÿ sphere

        auto flat_array = tm->GetTextureAtlas(SHADOW_DEPTH_FLAT_ARRAY);

        om->ForEach<Positions, SpotLightComponent, ShadowCasterComponent>(om->GetActiveScene(),
            [&](Positions& pos_el, SpotLightComponent& light, ShadowCasterComponent& sc) {
            //SDL_Log("Shadow pass: cameraIndex=%u, spotLayer=%u", cameraIndex, spotLayer);

            if (light.needsUpdate) {
                SDL_PushGPUVertexUniformData(cb, 0, &cameraIndex, sizeof(Uint32));
                rm->RenderPassStandardBody(cb, &rp, bm, 0);

                auto cp = SDL_BeginGPUCopyPass(cb);
                SDL_GPUTextureLocation src = {
                    .texture = rp.renderPassTexsData.depthTargetInfo.texture,
                    .layer = 0
                };
                SDL_GPUTextureLocation dst = {
                    .texture = flat_array->texture_binding.texture,
                    .layer = cameraIndex
                };
                SDL_CopyGPUTextureToTexture(cp, &src, &dst, flat_array->width, flat_array->height, 1, false);
                SDL_EndGPUCopyPass(cp);
            };
            cameraIndex++;
        }
        );

        om->ForEach<Positions, SphereLightComponent, ShadowCasterComponent>(om->GetActiveScene(),
            [&](Positions& pos_el, SphereLightComponent& light, ShadowCasterComponent& sc) {
            for (int face = 0; face < 6; ++face) {
                if (light.needsUpdate) {

                    SDL_PushGPUVertexUniformData(cb, 0, &cameraIndex, sizeof(Uint32));
                    rm->RenderPassStandardBody(cb, &rp, bm, 0);

                    auto cp = SDL_BeginGPUCopyPass(cb);
                    SDL_GPUTextureLocation src = {
                        .texture = rp.renderPassTexsData.depthTargetInfo.texture
                    };
                    SDL_GPUTextureLocation dst = {
                        .texture = flat_array->texture_binding.texture,
                        .layer = cameraIndex
                    };
                    SDL_CopyGPUTextureToTexture(cp, &src, &dst, flat_array->width, flat_array->height, 1, false);
                    SDL_EndGPUCopyPass(cp);
                }
                cameraIndex++;
            }

            sphereLayer++;
        });
    },
        std::move(shadow_rptd),
        10
    );
    shadowPass->renderPassTexsData.SetDepthTexture(
        shadow_temp->texture_binding.texture
    );

    shadow_pass_inited = true;
}

void DefaultRenderPassNamespace::SetDefaultShadowVSMRenderPass(PassManager* rm, TextureManager* tm, BufferManager* bm, ObjectManager* om, BatchBuilder* bb)
{
    if (shadow_pass_inited) {
        SDL_Log("Default shadow render pass is already initialized.");
        return;
	}
	auto shadow_sampler = tm->GetSampler(VSM_SAMPLER);
    auto vsm_sampler = tm->GetSampler(VSM_SAMPLER);

	shadow_moments_array = tm->CreateTextureAtlas(SHADOW_MOMENTS_ARRAY, TexturePresets::GetCreateInfo(TexturePreset::ShadowRG32_FlatArray1024_8Layers), vsm_sampler);
	TextureAtlas* shadow_depth_tex = tm->CreateTextureAtlas("shadow_depth_single_temp", TexturePresets::GetCreateInfo(TexturePreset::TempDepth1024), shadow_sampler);
	TextureAtlas* shadow_moments_temp = tm->CreateTextureAtlas(SHADOW_MOMENTS_BLUR_TEMP, TexturePresets::GetCreateInfo(TexturePreset::TempShadowRG32_1024), vsm_sampler);

    RenderPassTexturesInfo shadow_rptd{};
    shadow_rptd.CreateDepthTextureInfo(SDL_GPU_LOADOP_CLEAR, SDL_GPU_STOREOP_DONT_CARE, shadow_depth_tex->format);
    shadow_rptd.CreateColorTextureInfo(SDL_GPU_LOADOP_CLEAR, SDL_GPU_STOREOP_STORE, {1.0, 1.0, 1.0, 1.0}, shadow_moments_array->format);

    auto shadowPass = rm->CreateRenderPass(
        SHADOW_PASS,
        [rm, bm, om, tm, bb](SDL_GPUCommandBuffer* cb, PassManager* pm, RenderPassStep& rp)
        {
            Uint32 cameraIndex = 0;      // общий для всех типов света
            Uint32 sphereLayer = 0;      // только для sphere

			auto flat_array = tm->GetTextureAtlas(SHADOW_MOMENTS_ARRAY);
            uint32_t commands_byte_offset = bb->AskNumCommands() * sizeof(SDL_GPUIndexedIndirectDrawCommand);
            om->ForEach<Positions, SpotLightComponent, ShadowCasterComponent>(om->GetActiveScene(),
                [&](Positions& pos_el, SpotLightComponent& light, ShadowCasterComponent& sc) {
                //SDL_Log("Shadow pass: cameraIndex=%u, spotLayer=%u", cameraIndex, spotLayer);
                uint32_t byte_offset = (1 + cameraIndex) * commands_byte_offset;
                if (light.needsUpdate) {
                    SDL_PushGPUVertexUniformData(cb, 0, &cameraIndex, sizeof(Uint32));
                    rp.renderPassTexsData.colorTargetInfo.layer_or_depth_plane = cameraIndex;
                    rm->RenderPassStandardBody(cb, &rp, bm, 0);
                };
                cameraIndex++;
            }
            );

            om->ForEach<Positions, SphereLightComponent, ShadowCasterComponent>(om->GetActiveScene(),
                [&](Positions& pos_el, SphereLightComponent& light, ShadowCasterComponent& sc) {
                for (int face = 0; face < 6; ++face) {
                    uint32_t byte_offset = (1 + cameraIndex) * commands_byte_offset;
                    if (light.needsUpdate) {
                        SDL_PushGPUVertexUniformData(cb, 0, &cameraIndex, sizeof(Uint32));

                        rp.renderPassTexsData.colorTargetInfo.layer_or_depth_plane = cameraIndex;
                        rm->RenderPassStandardBody(cb, &rp, bm, 0);
                    }
                    cameraIndex++;
                }

                sphereLayer++; 
            });
        },
        std::move(shadow_rptd),
        10
    );
    shadowPass->renderPassTexsData.SetColorTexture(
        shadow_moments_array->texture_binding.texture
	);
	shadowPass->renderPassTexsData.SetDepthTexture(
        shadow_depth_tex->texture_binding.texture
	);

	shadow_pass_inited = true;
}

void DefaultRenderPassNamespace::SetDefaultShadowBlurPass(PassManager* pm, BufferManager* bm)
{
    pm->CreateComputePass(
        SHADOW_BLUR_PASS,
        [pm, bm](SDL_GPUCommandBuffer* cb, PassManager* pm, ComputePassStep& cp, uint8_t pass_frame)
    {
        ShadowBlurUniform push_data = {};
        DummyDispatchData dispatch_data = {};
        pm->ComputePassStandardBody(cb, &cp, bm, &push_data, &dispatch_data, pass_frame);
    },
        11
    );
}

void DefaultRenderPassNamespace::SetDefaultMainRenderPass(PassManager* rm, TextureManager* tm, BufferManager* bm)
{
    if (main_pass_inited) {
        SDL_Log("Default main render pass is already initialized.");
        return;
	}
    if (!shadow_pass_inited) {
        SDL_Log("Default shadow render pass must be initialized before the default main render pass.");
        return;
	}

    auto tci = TexturePresets::GetCreateInfo(TexturePreset::SingleDepth2048);
    tci.width = 800;
    tci.height = 600;
    tm->main_pass_depth_texture = tm->CreateGPU_Texture(tci);

	RenderPassTexturesInfo main_rptd{};
	main_rptd.CreateColorTextureInfo(SDL_GPU_LOADOP_CLEAR, SDL_GPU_STOREOP_STORE, { 0.1f,0.1f,0.14f,1.0f }, SDL_GPU_TEXTUREFORMAT_INVALID);
    main_rptd.CreateDepthTextureInfo(SDL_GPU_LOADOP_CLEAR, SDL_GPU_STOREOP_DONT_CARE, tci.format);

    auto mainPass = rm->CreateRenderPass(
        MAIN_PASS,
        [rm, bm](SDL_GPUCommandBuffer* cb, PassManager* pm, RenderPassStep& rp)
        {
            rm->RenderPassStandardBody(cb, &rp, bm, 0);
        },
        std::move(main_rptd),
        20
	);


    mainPass->global_texture_bindings = { shadow_depth_flat_array->texture_binding };
    mainPass->renderPassTexsData.SetDepthTexture(tm->main_pass_depth_texture);

	main_pass_inited = true;
}

void DefaultRenderPassNamespace::SetDefaultCullingComputeZerosPass(PassManager* pm, BufferManager* bm)
{
    auto compute_zeros_pass = pm->CreateComputePrepass(
        CULLING_ZEROS_PREPASS,
        [pm, bm](SDL_GPUCommandBuffer* cb, PassManager* pm, ComputePassStep& cp, uint8_t pass_frame) 
        {
            pm->ComputePassStandardBody(cb, &cp, bm, nullptr, nullptr, pass_frame);
        },
    0
    );
}

void DefaultRenderPassNamespace::SetDefaultCullingComputeCountPass(PassManager* pm, BufferManager* bm, ObjectManager* om, TransformDataModule* tdm, LightDataModule* ldm, InderectDataModule* idm)
{
    auto compute_pass = pm->CreateComputePrepass(
        CULLING_PREPASS,
        [pm, bm, om, tdm, ldm, idm](SDL_GPUCommandBuffer* cb, PassManager* pm, ComputePassStep& cp, uint8_t pass_frame)
        {
            ComputeCullingCountUniform data;
            data.num_instances = tdm->AskNumTransform(om, om->GetActiveScene());
            data.num_commands = idm->AskNumCommands(pm);

		    pm->ComputePassStandardBody(cb, &cp, bm, &data, nullptr, pass_frame);
        },
        10
		);
}

void DefaultRenderPassNamespace::SetDefaultCullingOffstPass(PassManager* pm, BufferManager* bm)
{
    auto compute_pass = pm->CreateComputePrepass(CULLING_OFFSET_PREPASS,
        [bm](SDL_GPUCommandBuffer* cb, PassManager* pm, ComputePassStep& cp, uint8_t pass_frame) {
            pm->ComputePassStandardBody(cb, &cp, bm, nullptr, nullptr, pass_frame);
        },
        20
    );
}

void DefaultRenderPassNamespace::SetDefaultCullingOutIndirectPass(PassManager* pm, BufferManager* bm)
{
    pm->CreateComputePrepass(CULLING_OUT_INDIRECT_PREPASS,
        [bm](SDL_GPUCommandBuffer* cb, PassManager* pm, ComputePassStep& cp, uint8_t pass_frame) {
            ComputeCullingOutIndirectUniform data;
            pm->ComputePassStandardBody(cb, &cp, bm, &data, nullptr, pass_frame);
        },
        30
    );
}

void DefaultRenderPassNamespace::SetDefaultCullingOutTransformPass(PassManager* pm, BufferManager* bm, ObjectManager* om, TransformDataModule* tdm, LightDataModule* ldm, InderectDataModule* idm)
{
    pm->CreateComputePass(
        CULLING_WRITE_PASS,
        [pm, bm, om, tdm, ldm, idm](SDL_GPUCommandBuffer* cb, PassManager* pm, ComputePassStep& cp, uint8_t pass_frame)
        {
            ComputeCullingCountUniform data;
            data.num_instances = tdm->AskNumTransform(om, om->GetActiveScene());
            data.num_commands = idm->AskNumCommands(pm);

            pm->ComputePassStandardBody(cb, &cp, bm, &data, nullptr, pass_frame);
        },
        0
    );
}

