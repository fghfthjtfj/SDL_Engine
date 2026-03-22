#pragma once
#include <vector>
#include <glm/glm.hpp>
#include "LightStruct.h"
#include "BaseComponents.h"

struct UploadTask;
struct SceneData;
class BufferManager;
class ObjectManager;
class Camera;

struct LightCamera {
    glm::mat4 view;
    glm::mat4 proj;
};
// нопедекхрэ яерреп!
class LightDataModule {
public:
    LightDataModule();
    uint32_t CalculateLightSize(ObjectManager* om, SceneData* scene);
    void StoreLightData(BufferManager* bm, UploadTask* task, ObjectManager* om, SceneData* scene);

	uint32_t CalculateLightCamerasSize(ObjectManager* om, SceneData* scene);
	void StoreLightCameras(BufferManager* bm, UploadTask* task, ObjectManager* om, SceneData* scene);

    uint32_t AskNumLightCameras(ObjectManager* om, SceneData* scene);
private:
    uint32_t total_size = 0;
};