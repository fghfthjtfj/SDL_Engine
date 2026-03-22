#pragma once
#include <cstdint>

class PassManager;
class BufferManager;
class ModelManager;
struct UploadTask;

class BoundSphereDataModule {
public:
	BoundSphereDataModule();
	uint32_t CalculateSphereSize(PassManager* pm, ModelManager* mm);
	void StoreSpheres(BufferManager* bm, UploadTask* task, PassManager* pm);
private:
	uint32_t total_size;
};