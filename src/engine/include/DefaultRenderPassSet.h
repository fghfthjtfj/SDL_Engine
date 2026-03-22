#pragma once
#include "TextureData.h"

class PassManager;
class BufferManager;
class TextureManager;
class ObjectManager;
class ShaderManager;
class PipeManager;

class TransformDataModule;
class LightDataModule;
class InderectDataModule;

namespace DefaultRenderPassSet
{
    void SetDefaultShadowRenderPass(PassManager* rm, TextureManager* tm, BufferManager* bm, ObjectManager* om);
    void SetDefaultMainRenderPass(PassManager* rm, TextureManager* tm, BufferManager* bm);

    void SetDefaultCullingComputeZerosPass(PassManager* pm, BufferManager* bm);
	
    struct ComputeCullingCountUniform {
        uint32_t num_instances;
        uint32_t num_commands;
        uint32_t num_cameras;
        uint32_t cmd_offset;
    }; 
    void SetDefaultCullingComputeCountPass(PassManager* rm, BufferManager* bm, ObjectManager* om, TransformDataModule* tdm, LightDataModule* ldm, InderectDataModule* idm);


}