#pragma once
#include "TextureData.h"

class PassManager;
class BufferManager;
class TextureManager;
class ObjectManager;
class ShaderManager;
class PipeManager;

class BatchBuilder;

class TransformDataModule;
class LightDataModule;
class InderectDataModule;

namespace DefaultRenderPassNamespace
{
    inline const std::string SHADOW_DEPTH_FLAT_ARRAY = "shadow_depth_flat_array";

	void SetDefaultShadowPCFRenderPass(PassManager* rm, TextureManager* tm, BufferManager* bm, ObjectManager* om, BatchBuilder* bb);
    void SetDefaultShadowVSMRenderPass(PassManager* rm, TextureManager* tm, BufferManager* bm, ObjectManager* om, BatchBuilder* bb);

    struct ShadowBlurUniform {
        uint32_t layerIndex;
    };
    struct DummyDispatchData {};
    void SetDefaultShadowBlurPass(PassManager* pm, BufferManager* bm);
    void SetDefaultMainRenderPass(PassManager* rm, TextureManager* tm, BufferManager* bm);

    void SetDefaultCullingComputeZerosPass(PassManager* pm, BufferManager* bm);
	
    struct ComputeCullingCountUniform {
        uint32_t num_instances;
        uint32_t num_commands;
        uint32_t num_cameras;
        uint32_t cmd_offset;
    }; 
    void SetDefaultCullingComputeCountPass(PassManager* rm, BufferManager* bm, ObjectManager* om, TransformDataModule* tdm, LightDataModule* ldm, InderectDataModule* idm);
    void SetDefaultCullingOffstPass(PassManager* pm, BufferManager* bm);

    struct ComputeCullingOutIndirectUniform {
        uint32_t num_commands;
        uint32_t num_cameras;
        uint32_t cmd_offset;
    };
    void SetDefaultCullingOutIndirectPass(PassManager* pm, BufferManager* bm);
    void SetDefaultCullingOutTransformPass(PassManager* pm, BufferManager* bm, ObjectManager* om, TransformDataModule* tdm, LightDataModule* ldm, InderectDataModule* idm);

    inline const std::string SHADOW_MOMENTS_ARRAY = "shadow_moments_array";
    inline const std::string SHADOW_MOMENTS_BLUR_TEMP = "shadow_moments_single_temp";


}