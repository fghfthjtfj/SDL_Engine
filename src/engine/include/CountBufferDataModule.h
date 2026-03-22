#pragma once
#include <cstdint>

class BatchBuilder;
class LightDataModule;
class ObjectManager;
struct SceneData;

class CountBufferDataModule {
public:
	CountBufferDataModule();
	uint32_t CountBufferSize(ObjectManager* om, SceneData* scene, BatchBuilder* bb, LightDataModule* ldm);
private:
	uint32_t total_size = 0;
};