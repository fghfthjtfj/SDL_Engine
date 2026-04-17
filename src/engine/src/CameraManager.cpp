#include "PCH.h"
#include "CameraManager.h"
#include "BufferManager.h"

Camera* CameraManager::CreateCamera(float width, float height, float fov_y, float near_plane, float far_plane)
{
    cameras.emplace_back(width, height, fov_y, near_plane, far_plane);

    return &cameras.back();
}

void CameraManager::SetActiveCamera(int index)
{
    for (size_t i = 0; i < cameras.size(); ++i) {
        cameras[i].active = false;
    }
	cameras[index].active = true;
}

Camera* CameraManager::GetActiveCamera()
{
    for (size_t i = 0; i < cameras.size(); ++i) {
        if (cameras[i].active) {
            return &cameras[i];
		}
	}
    SDL_Log("No active camera");
	return nullptr;
}

uint32_t CameraManager::CalculateCameraSize()
{
	return sizeof(CameraData);
}

void CameraManager::StoreActiveCamera(BufferManager* bm, UploadTask* task)
{
    Camera* cam = GetActiveCamera();

    CameraData data;
    data.view = cam->GetView();
    data.proj = cam->GetProj();

	bm->UploadToPrePassTransferBuffer(task, sizeof(CameraData), &data);
}

//void CameraManager::StoreActiveCamera(BufferManager* bm, UploadTask* task, SDL_GPUCommandBuffer* cb)
//{
//    BufferData* light_buffer = (*bm)[DEFAULT_LIGHT_CAMERA_BUFFER];
//    BufferData* camera_buffer = (*bm)[DEFAULT_CAMERA_BUFFER];
//    SDL_GPUCopyPass* cp = SDL_BeginGPUCopyPass(cb);
//    SDL_GPUBufferLocation src{
//        bm->_GetGPUBufferForFrame(light_buffer, bm->logic_index),
//        0
//    };
//    SDL_GPUBufferLocation dst{
//        bm->_GetGPUBufferForFrame(camera_buffer, bm->logic_index),
//        0
//    };
//    SDL_CopyGPUBufferToBuffer(cp, &src, &dst, 128, false);
//    SDL_EndGPUCopyPass(cp);
//}


//void CameraManager::StoreActiveCamera(BufferManager* bm, SDL_GPUCopyPass* cp)
//{
//    BufferData* light_buffer = (*bm)[DEFAULT_LIGHT_CAMERA_BUFFER];
//    BufferData* camera_buffer = (*bm)[DEFAULT_CAMERA_BUFFER];
//
//    SDL_GPUBufferLocation src{
//        bm->_GetGPUBufferForFrame(light_buffer, bm->logic_index),
//        0
//    };
//    SDL_GPUBufferLocation dst{
//        bm->_GetGPUBufferForFrame(camera_buffer, bm->logic_index),
//        0
//    };
//    SDL_CopyGPUBufferToBuffer(cp, &src, &dst, 128, false);
//    SDL_EndGPUCopyPass(cp);
//}
