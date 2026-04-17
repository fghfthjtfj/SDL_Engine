#include "DefaultUpdateSet.h"
#include "ObjectManager.h"
#include "BufferManager.h"
#include "LightDataModule.h"
#include "CameraManager.h"
#include "PIB_DataModule.h"
#include "TransformDataModule.h"
#include "ModelManager.h"
#include "InderectDataModule.h"
#include "BatchBuilder.h"
#include "BoundSphereDataModule.h"
#include "CountBufferDataModule.h"

namespace DefaultUpdateSet
{
    bool camera_update_inited = false;
    bool position_update_inited = false;
    bool light_update_inited = false;
    bool position_index_update_inited = false;
    bool vertex_update_inited = false;
    bool index_update_inited = false;
    bool light_cameras_update_inited = false;
	bool indirect_update_inited = false;
    bool bound_sphere_update_intited = false;
}

void DefaultUpdateSet::SetDefaultCameraUpdater(BufferManager* bm, CameraManager* cm)
{
    if (camera_update_inited) {
        SDL_Log("Default camera updater is already initialized.");
        return;
	}
    bm->CreateUpdateInstruction(DEFAULT_CAMERA_BUFFER,
        [cm](SDL_GPUCopyPass* cp, BufferManager* bm, UploadTask& task)
    {
        cm->StoreActiveCamera(bm, &task);
    },
        [cm]() -> uint32_t
    {
        return cm->CalculateCameraSize();
    }
    );
    
	camera_update_inited = true;
}


void DefaultUpdateSet::SetDefaultPositionUpdater(BufferManager* buffer_manager, ObjectManager* om, TransformDataModule* tdm)
{
    if (position_update_inited) {
        SDL_Log("Default position updater is already initialized.");
		return;
	}
    buffer_manager->CreateUpdateInstruction(DEFAULT_TRANSFORM_BUFFER,

        [om, tdm](SDL_GPUCopyPass* cp, BufferManager* buffer_manager, UploadTask& task)
    {
        SceneData* scene = om->GetActiveScene();
        if (!scene)
            return;

        tdm->StoreTransforms(buffer_manager, &task, om, scene);
    },

        [om, tdm]() -> uint32_t
    {
        SceneData* scene = om->GetActiveScene();
        if (!scene)
            return 0;

        return tdm->CalculateTransformSize(om, scene);
    }
    );

	position_update_inited = true;
}


void DefaultUpdateSet::SetDefaultLightUpdater(BufferManager* buffer_manager, ObjectManager* om, CameraManager* cm, LightDataModule* ldm)
{
    if (light_update_inited) {
		SDL_Log("Default light updater is already initialized.");
		return;
	}
    buffer_manager->CreateUpdateInstruction(DEFAULT_LIGHT_BUFFER, 
        [om, cm, ldm](SDL_GPUCopyPass* cp, BufferManager* buffer_manager, UploadTask& task)
    {
        SceneData* scene = om->GetActiveScene();
        if (!scene)
            return;
        ldm->StoreLightData(buffer_manager, &task, om, scene);
    },
        [om, cm, ldm]() -> uint32_t
    {
        SceneData* scene = om->GetActiveScene();
        if (!scene)
            return 0;
        return ldm->CalculateLightSize(om, scene);
	}
    );
	light_update_inited = true;
}

void DefaultUpdateSet::SetDefaultPositionIndexUpdater(BufferManager* buffer_manager, PassManager* rm, ObjectManager* om, PIB_DataModule* pib_dm, BatchBuilder* bb)
{
	if (position_index_update_inited) {
		SDL_Log("Default position index updater is already initialized.");
		return;
	}
    buffer_manager->CreateUpdateInstruction(DEFAULT_POSITION_INDEX_BUFFER,
        [rm, pib_dm](SDL_GPUCopyPass* cp, BufferManager* buffer_manager, UploadTask& task)
    {
        pib_dm->StorePIB(buffer_manager, rm, &task);
    },
		[om, rm, pib_dm, bb]() -> uint32_t
        {
        return pib_dm->CalculatePIBSizes(bb, om, rm);
	}
    );

	position_index_update_inited = true;
}

void DefaultUpdateSet::SetDefaultVertexUpdater(BufferManager* buffer_manager, ModelManager* mm)
{
	if (vertex_update_inited) {
		SDL_Log("Default vertex updater is already initialized.");
		return;
	}
    buffer_manager->CreateUpdateInstruction(DEFAULT_VERTEX_BUFFER,
        [mm](SDL_GPUCopyPass* cp, BufferManager* buffer_manager, UploadTask& task)
    {
        mm->UploadModelVertexBuffer(buffer_manager, &task);
    },
        [mm]() -> uint32_t
    {
        return mm->CalculateModelsVerticesSize();
    }
	);

	vertex_update_inited = true;
}

void DefaultUpdateSet::SetDefaultIndexUpdater(BufferManager* buffer_manager, ModelManager* mm)
{
	if (index_update_inited) {
		SDL_Log("Default index updater is already initialized.");
		return;
	}
    buffer_manager->CreateUpdateInstruction(DEFAULT_INDEX_BUFFER,
        [mm](SDL_GPUCopyPass* cp, BufferManager* buffer_manager, UploadTask& task)
    {
        mm->UploadModelIndexBuffer(buffer_manager, &task);
    },
        [mm]() -> uint32_t
    {
        return mm->CalculateModelsIndicesSize();
    }
	);
}

void DefaultUpdateSet::SetDefaultLightCamerasUpdater(BufferManager* buffer_manager, ObjectManager* om, LightDataModule* ldm)
{
	if (light_cameras_update_inited) {
		SDL_Log("Default light cameras updater is already initialized.");
		return;
	}
    buffer_manager->CreateUpdateInstruction(DEFAULT_LIGHT_CAMERA_BUFFER,
        [om, ldm](SDL_GPUCopyPass* cp, BufferManager* buffer_manager, UploadTask& task)
    {
        SceneData* scene = om->GetActiveScene();
        if (!scene)
            return;
        ldm->StoreLightCameras(buffer_manager, &task, om, scene);
    },
        [om, ldm]() -> uint32_t
    {
        SceneData* scene = om->GetActiveScene();
        if (!scene)
            return 0;
        return ldm->CalculateLightCamerasSize(om, scene);
    }
	);

	light_cameras_update_inited = true;
}

void DefaultUpdateSet::SetDefaultIndirectUpdater(BufferManager* buffer_manager, PassManager* pm, InderectDataModule* idm)
{
    if (indirect_update_inited) {
        SDL_Log("Default indirect updater is already initialized.");
        return;
    }
    buffer_manager->CreateUpdateInstruction(DEFAULT_INDIRECT_BUFFER,
        [pm, idm](SDL_GPUCopyPass* cp, BufferManager* buffer_manager, UploadTask& task)
    {
        idm->StoreIndirect(buffer_manager, pm, &task);
    },
        [pm, idm]() -> uint32_t
    {
        return idm->CalculateIndirectSize(pm);
    }
    );
    indirect_update_inited = true;
}

void DefaultUpdateSet::SetDefaultBoundSphereUpdater(BufferManager* buffer_manager, PassManager* pm, ModelManager* mm, BoundSphereDataModule* bdm)
{
    if (bound_sphere_update_intited) {
        SDL_Log("Default bound sphere updater is already initialized.");
    }

    buffer_manager->CreatePrePassUpdateInstruction(DEFAULT_BOUND_SPHERE_BUFFER,
        [pm, bdm](SDL_GPUCopyPass* cp, BufferManager* buffer_manager, UploadTask& task)
    {
        bdm->StoreSpheres(buffer_manager, &task, pm);
    },
        [pm, mm, bdm]() -> uint32_t
    {
        return bdm->CalculateSphereSize(pm, mm);
    }
    );
    bound_sphere_update_intited = true;
}

void DefaultUpdateSet::SetDefaultCountBufferUpdater(BufferManager* bm, ObjectManager* om, CountBufferDataModule* cdm, LightDataModule* ldm, BatchBuilder* bb)
{
    bm->CreatePrePassUpdateInstruction(DEFAULT_COUNT_BUFFER,
        nullptr,

        [om, cdm, ldm, bb]() -> uint32_t {
        return cdm->CountBufferSize(om, om->GetActiveScene(), bb, ldm);
    }
    );
}

void DefaultUpdateSet::SetDefaultOffsetBufferUpdater(BufferManager* bm, ObjectManager* om, CountBufferDataModule* cdm, LightDataModule* ldm, BatchBuilder* bb)
{
    bm->CreatePrePassUpdateInstruction(DEFAULT_OFFSET_BUFFER,
        nullptr,

        [om, cdm, ldm, bb]() -> uint32_t {
            return cdm->CountBufferSize(om, om->GetActiveScene(), bb, ldm) - 1;
        }
    );
}

void DefaultUpdateSet::SetDefaultEntityToBatchUpdater(BufferManager* bm, ObjectManager* om, PassManager* pm, BatchBuilder* bb, PIB_DataModule* pdm)
{
    bm->CreatePrePassUpdateInstruction(DEFAULT_ENTITY_TO_BATCH_BUFFER,
        [pdm, pm](SDL_GPUCopyPass* cp, BufferManager* bm, UploadTask& task) {
            pdm->StoreEntityToBatch(bm, pm, &task);
        },
        [om, pm, bb, pdm]() -> uint32_t {
            return pdm->CalcuteEntityToBatch(bb, om, pm);
        }
    );
}

void DefaultUpdateSet::SetDefaultOutTransformUpdater(BufferManager* bm, TransformDataModule* tdm)
{
    bm->CreatePostReadbackUpdateInstruction(DEFAULT_OUT_TRANSFORM_BUFFER,
        nullptr,

        [tdm]() -> uint32_t {
            return tdm->CalculateOutTransformSize();
        }
    );
}

void DefaultUpdateSet::SetDefaultOutIndirectUpldater(BufferManager* bm, ObjectManager* om, BatchBuilder* bb, LightDataModule* ldm)
{
    bm->CreatePrePassUpdateInstruction(DEFAULT_OUT_INDIRECT_BUFFER,
        nullptr,
        [om, bb, ldm]() -> uint32_t {
            uint32_t num_cameras_total = ldm->AskNumLightCameras(om, om->GetActiveScene()) + 1; // +1 main
            return num_cameras_total * bb->AskNumCommands() * sizeof(SDL_GPUIndexedIndirectDrawCommand);

        }
    );
}

//void DefaultUpdateSet::SetDefaultOutTransformUpdater(BufferManager* bm, TransformDataModule* tdm)
//{
//    bm->CreateUpdateInstruction(
//
//    )
//}

void DefaultUpdateSet::SetDefaultCountReader(BufferManager* bm, TransformDataModule* tdm)
{
    bm->CreateReadBackInstruction(DEFAULT_COUNT_BUFFER,
        [tdm](BufferManager* buffer_manager, ReadBackTask& task) {
        tdm->ReadBackCullingCountReader(buffer_manager, &task);
    },
        [tdm]() -> uint32_t {
        return tdm->ReadBackCullingCountSize();
    }
    );
}

//void DefaultUpdateSet::SetDefaultOutTransformReader(BufferManager* bm, TransformDataModule* tdm)
//{
//    bm->CreateReadBackInstruction(DEFAULT_OUT_TRA)
//}
