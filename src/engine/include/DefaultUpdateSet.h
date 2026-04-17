#pragma once

class ObjectManager;
class BufferManager;
class CameraManager;
class Camera;
class PIB_DataModule;
class TransformDataModule;
class LightDataModule;
class ModelManager;
class PassManager;
class InderectDataModule;
class BoundSphereDataModule;
class BatchBuilder;
class CountBufferDataModule;

namespace DefaultUpdateSet
{
	void SetDefaultCameraUpdater(BufferManager* buffer_manager, CameraManager* camera);
	void SetDefaultPositionUpdater(BufferManager* buffer_manager, ObjectManager* om, TransformDataModule* tdm);
	void SetDefaultLightUpdater(BufferManager* buffer_manager, ObjectManager* om, CameraManager* cm, LightDataModule* ldm);
	void SetDefaultPositionIndexUpdater(BufferManager* buffer_manager, PassManager* rm, ObjectManager* om, PIB_DataModule* pib_dm, BatchBuilder* bb);
	void SetDefaultVertexUpdater(BufferManager* buffer_manager, ModelManager* mm);
	void SetDefaultIndexUpdater(BufferManager* buffer_manager, ModelManager* mm);
	void SetDefaultLightCamerasUpdater(BufferManager* buffer_manager, ObjectManager* om, LightDataModule* ldm);
	void SetDefaultIndirectUpdater(BufferManager* buffer_manager, PassManager* pm, InderectDataModule* idm);
	void SetDefaultBoundSphereUpdater(BufferManager* buffer_manager, PassManager* pm, ModelManager* mm, BoundSphereDataModule* bdm);
	void SetDefaultCountBufferUpdater(BufferManager* bm, ObjectManager* om, CountBufferDataModule* cdm, LightDataModule* ldm, BatchBuilder* bb);
	void SetDefaultOffsetBufferUpdater(BufferManager* bm, ObjectManager* om, CountBufferDataModule* cdm, LightDataModule* ldm, BatchBuilder* bb);
	void SetDefaultEntityToBatchUpdater(BufferManager* bm, ObjectManager* om, PassManager* pm, BatchBuilder* bb, PIB_DataModule* pdm);
	void SetDefaultOutTransformUpdater(BufferManager* bm, TransformDataModule* tdm);
	void SetDefaultOutIndirectUpldater(BufferManager* bm, ObjectManager* om, BatchBuilder* bb, LightDataModule* ldm);

	void SetDefaultCountReader(BufferManager* bm, TransformDataModule* tdm);
	
};