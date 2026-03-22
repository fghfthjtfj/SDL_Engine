#pragma once

class ObjectManager;
class PipeManager;
class PassManager;
class TextureManager;
class ShaderManager;
struct SceneData;

class BatchBuilder {
public:
	BatchBuilder();
	void BuildRenderBatches(PipeManager* pm, PassManager* pass_manager, ObjectManager* om, SceneData* scene);
	void BuildComputeBatches(PipeManager* pm, ShaderManager* sm);
	void BuildComputePrepassBatches(PipeManager* pm, ShaderManager* sm);
	bool CheckPIBNeedUpload() { return need_PIB_upload; };
	void SetPIBNeedUpload(bool state) { need_PIB_upload = state; };
	uint32_t AskNumCommands();
private:
	void FinilizeRenderBatches(PassManager* pass_manager);
	bool dirty_batches = true; // Определить сеттер!
	bool need_PIB_upload = true;

	uint32_t total_commands = 0;
};