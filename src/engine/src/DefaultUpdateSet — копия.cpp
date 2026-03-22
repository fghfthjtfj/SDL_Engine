#include "DefaultUpdateSet.h"
#include "ObjectManager.h"
#include "BufferManager.h"
#include "LightDataModule.h"
#include "SceneData.h"
#include "CameraManager.h"
#include "PIB_DataModule.h"
#include "TransformDataModule.h"

void SetDefaultCameraUpdater(BufferManager* buffer_manager, CameraManager* cm)
{
    buffer_manager->CreateUpdateInstruction(DEFAULT_CAMERA_BUFFER,
        [buffer_manager, cm](BufferData& bd)
    {
        cm->StoreActiveCamera(buffer_manager, &bd);
    });
}

void SetDefaultPositionUpdater(BufferManager* buffer_manager, ObjectManager* om, TransformDataModule* tdm)
{
    buffer_manager->CreateUpdateInstruction(DEFAULT_TRANSFORM_BUFFER,
        [buffer_manager, om, tdm](BufferData& bd) {
        SceneData* scene = om->GetActiveScene();
        if (scene)
			tdm->StoreTransforms(buffer_manager, om, scene);
    });
}

void SetDefaultLightUpdater(BufferManager* buffer_manager, ObjectManager* om, CameraManager* cm, LightDataModule* ldm)
{
    buffer_manager->CreateUpdateInstruction(DEFAULT_LIGHT_BUFFER, [buffer_manager, om, cm, ldm](BufferData& bd) {
        Camera* camera = cm->GetActiveCamera();
        SceneData* scene = om->GetActiveScene();

        ldm->StoreLightData(buffer_manager, om, scene, camera);
    });
}

void SetDefaultPositionIndexUpdater(BufferManager* buffer_manager, ObjectManager* om, PIB_DataModule* pib_dm)
{
    buffer_manager->CreateUpdateInstruction(DEFAULT_POSITION_INDEX_BUFFER_MP, [buffer_manager, om, pib_dm](BufferData& bd) {
        pib_dm->StorePIB_MainPass(buffer_manager, om, om->GetActiveScene());
    });
}
