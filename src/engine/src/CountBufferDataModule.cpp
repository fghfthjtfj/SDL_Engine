#include "PCH.h"
#include "CountBufferDataModule.h"
#include "LightDataModule.h"
#include "BatchBuilder.h"

CountBufferDataModule::CountBufferDataModule() {};

uint32_t CountBufferDataModule::CountBufferSize(ObjectManager* om, SceneData* scene, BatchBuilder* bb, LightDataModule* ldm)
{
	total_size = (ldm->AskNumLightCameras(om, scene) + 1) * bb->AskNumCommands() + 1;
	return total_size;
}
