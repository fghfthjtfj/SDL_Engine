#pragma once
#include <cstdint>

class ObjectManager;
class BufferManager;
class PassManager;
struct UploadTask;

class InderectDataModule
{
public:
	InderectDataModule();
	uint32_t CalculateIndirectSize(PassManager* pm);
	void StoreIndirect(BufferManager* bm, PassManager* pm, UploadTask* task);
	uint32_t AskNumCommands(PassManager* pm);
private:
	uint32_t total_size = 0;
	bool dirty = true;
};