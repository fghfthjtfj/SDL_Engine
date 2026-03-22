#pragma once

class ObjectManager;
class BufferManager;
class PassManager;
class BatchBuilder;
struct UploadTask;


class PIB_DataModule
{
public:
    PIB_DataModule();
    uint32_t CalculatePIBSizes(BatchBuilder* bb, ObjectManager* om, PassManager* rm);
    void StorePIB(BufferManager* bm, PassManager* rm, UploadTask* task);

private:
    bool dirty = true;
	uint32_t total_elements = 0;
};
