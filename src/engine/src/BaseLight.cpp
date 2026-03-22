#include <iostream>
#include "BaseLight.h"
#include "Utils.h"
#include "BufferManager.h"
#include "BufferData.h"
#include "CameraStruct.h"

void Light::CalculateLight(const Camera* camera, LightData* light) {
    glm::vec3 pos = camera->GetPosition();
    light->position_radius = glm::vec4(pos, light->position_radius.w);
    glm::vec3 dir = glm::normalize(camera->GetTarget() - camera->GetPosition());
    light->direction_angle = glm::vec4(dir, light->direction_angle.w);
}

void Light::StoreLightData(BufferManager* bm, BufferData* buffer_data, const Camera* camera, std::vector<LightData>& lights) {

    CalculateLight(camera, &lights[0]);
    bm->uploadToGPUBuffer(buffer_data, lights.data(), safe_u32(lights.size() * sizeof(LightData)));
}


