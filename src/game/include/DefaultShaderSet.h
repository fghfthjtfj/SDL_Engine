#pragma once

class ShaderManager;
class PassManager;
class BufferManager;
class TextureManager;
class CameraManager;
class ObjectManager;
class BatchBuilder;
class LightDataModule;

namespace DefaultShaderProgramSet
{
    // Render shader programs
    void SetMainShaderProgram(BufferManager* bm, ShaderManager* sm, PassManager* pm);
    void SetDefaultShadowShaderProgram(BufferManager* bm, ShaderManager* sm, PassManager* pm);

    // Compute shader programs
    void SetCullingZerosPrograms(BufferManager* bm, ShaderManager* sm, PassManager* pm);
    void SetCullingCountPrograms(BufferManager* bm, ShaderManager* sm, PassManager* pm, CameraManager* cm, ObjectManager* om, BatchBuilder* bb, LightDataModule* ldm);
    void SetCullingOffsetPrograms(BufferManager* bm, ShaderManager* sm, PassManager* pm);
    void SetCullingOutIndirectPrograms(BufferManager* bm, ShaderManager* sm, PassManager* pm, ObjectManager* om, BatchBuilder* bb, LightDataModule* ldm);
    void SetCullingWritePrograms(BufferManager* bm, ShaderManager* sm, PassManager* pm, CameraManager* cm, ObjectManager* om, BatchBuilder* bb, LightDataModule* ldm);

    void SetShadowBlurPrograms(BufferManager* bm, ShaderManager* sm, PassManager* pm, TextureManager* tm, ObjectManager* om, LightDataModule* ldm);
}