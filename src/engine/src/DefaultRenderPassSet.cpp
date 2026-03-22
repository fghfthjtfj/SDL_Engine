#include "DefaultRenderPassSet.h"
#include "TextureManager.h"
#include "RenderManager.h"
#include "TexturesPresets.h"
#include "TextureSamplerPresets.h"
#include "ObjectManager.h"
#include "TransformDataModule.h"
#include "LightDataModule.h"
#include "InderectDataModule.h"

namespace DefaultRenderPassSet
{
    static TextureAtlas* shadow_depth_flat_array = nullptr;

    std::string SHADOW_DEPTH_FLAT_ARRAY = "shadow_depth_flat_array";

    bool shadow_pass_inited = false;
	bool main_pass_inited = false;
}

void DefaultRenderPassSet::SetDefaultShadowRenderPass(PassManager* rm, TextureManager* tm, BufferManager* bm, ObjectManager* om)
{
    if (shadow_pass_inited) {
        SDL_Log("Default shadow render pass is already initialized.");
        return;
	}
	auto shadow_sampler = tm->GetSampler(DEFAULT_SHADOW_SAMPLER);
	auto default_sampler = tm->GetSampler(DEFAULT_SAMPLER);

	shadow_depth_flat_array = tm->CreateTextureAtlas(SHADOW_DEPTH_FLAT_ARRAY, TexturePresets::GetCreateInfo(TexturePreset::Depth_FlatArray1024_8Layers), shadow_sampler);

	TextureAtlas* shadow_temp = tm->CreateTextureAtlas("shadow_depth_single_temp", TexturePresets::GetCreateInfo(TexturePreset::TempDepth1024), shadow_sampler);

    RenderPassTexturesInfo shadow_rptd{};
    shadow_rptd.CreateDepthTextureInfo(SDL_GPU_LOADOP_CLEAR, SDL_GPU_STOREOP_STORE);

    RasterizerStateBiasParams shadow_rsbp;
	shadow_rsbp.enable_depth_bias = true;
	shadow_rsbp.depth_bias_constant_factor = 0.003f;
	shadow_rsbp.depth_bias_slope_factor = 1.0f;
	shadow_rsbp.depth_bias_clamp = 0.01f;

    auto shadowPass = rm->CreateRenderPass(
        SHADOW_PASS,
        [rm, bm, om, tm](SDL_GPUCommandBuffer* cb, PassManager* pm, RenderPassStep& rp)
        {
            Uint32 cameraIndex = 0;      // общий для всех типов света
            Uint32 sphereLayer = 0;      // только для sphere

			auto flat_array = tm->GetTextureAtlas(SHADOW_DEPTH_FLAT_ARRAY);

            om->ForEach<Positions, SpotLightComponent, ShadowCasterComponent>(om->GetActiveScene(),
                [&](Positions& pos_el, SpotLightComponent& light, ShadowCasterComponent& sc) {
                //SDL_Log("Shadow pass: cameraIndex=%u, spotLayer=%u", cameraIndex, spotLayer);

                if (light.needsUpdate) {
                    SDL_PushGPUVertexUniformData(cb, 0, &cameraIndex, sizeof(Uint32));
                    rm->RenderPassStandardBody(cb, &rp, bm);

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
                        rm->RenderPassStandardBody(cb, &rp, bm);

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
        10,
        shadow_rsbp
    );
	shadowPass->renderPassTexsData.SetDepthTexture(
        shadow_temp->texture_binding.texture
	);

	shadow_pass_inited = true;
}

void DefaultRenderPassSet::SetDefaultMainRenderPass(PassManager* rm, TextureManager* tm, BufferManager* bm)
{
    if (main_pass_inited) {
        SDL_Log("Default main render pass is already initialized.");
        return;
	}
    if (!shadow_pass_inited) {
        SDL_Log("Default shadow render pass must be initialized before the default main render pass.");
        return;
	}
	RenderPassTexturesInfo main_rptd{};
	main_rptd.CreateColorTextureInfo(SDL_GPU_LOADOP_CLEAR, SDL_GPU_STOREOP_STORE, { 0.1f,0.1f,0.14f,1.0f });
    main_rptd.CreateDepthTextureInfo(SDL_GPU_LOADOP_CLEAR, SDL_GPU_STOREOP_DONT_CARE);

	RasterizerStateBiasParams main_rsbp;
    auto mainPass = rm->CreateRenderPass(
        MAIN_PASS,
        [rm, bm](SDL_GPUCommandBuffer* cb, PassManager* pm, RenderPassStep& rp)
        {
            rm->RenderPassStandardBody(cb, &rp, bm);
        },
        std::move(main_rptd),
        20 ,
        main_rsbp
	);

    auto tci = TexturePresets::GetCreateInfo(TexturePreset::SingleDepth2048);
    tci.width = 800;
    tci.height = 600;
    mainPass->global_texture_bindings = { shadow_depth_flat_array->texture_binding };
    tm->main_pass_depth_texture = tm->CreateGPU_Texture(tci);
    mainPass->renderPassTexsData.SetDepthTexture(tm->main_pass_depth_texture);

	main_pass_inited = true;
}

void DefaultRenderPassSet::SetDefaultCullingComputeZerosPass(PassManager* pm, BufferManager* bm)
{
    auto compute_zeros_pass = pm->CreateComputePrepass(
        CULLING_ZEROS_PREPASS,
        [pm, bm](SDL_GPUCommandBuffer* cb, PassManager* pm, ComputePassStep& cp, uint8_t pass_frame) 
        {
            pm->ComputePassStandardBody(cb, &cp, bm, nullptr, pass_frame);
        },
    0
    );
}

void DefaultRenderPassSet::SetDefaultCullingComputeCountPass(PassManager* pm, BufferManager* bm, ObjectManager* om, TransformDataModule* tdm, LightDataModule* ldm, InderectDataModule* idm)
{
    auto compute_pass = pm->CreateComputePrepass(
        CULLING_PREPASS,
        [pm, bm, om, tdm, ldm, idm](SDL_GPUCommandBuffer* cb, PassManager* pm, ComputePassStep& cp, uint8_t pass_frame)
        {
            ComputeCullingCountUniform data;
            data.num_instances = tdm->AskNumTransform(om, om->GetActiveScene());
            data.num_commands = idm->AskNumCommands(pm);

		    pm->ComputePassStandardBody(cb, &cp, bm, &data, pass_frame);
        },
        10
		);
}

