#include "BaseScene.h"
#include "Utils.h"

BaseScene::BaseScene() {}

// ЧУШЬ!!!
void BaseScene::AddSprites(std::vector<BaseSprite*> sprites)
{
    for (auto sprite : sprites) {
        sprites.push_back(sprite);
    };
}

//void BaseScene::BuildBatches(
//    std::unordered_map<std::string, ModelData>* models_data,
//    std::vector<glm::mat4>& allTransforms,
//    std::vector<BaseSprite*>* objects,
//    std::vector<RenderCommandData*>& all_rcd,
//    std::unordered_map<RenderCommandData*, std::vector<ModelData*>>& rcd_batches
//)
//{
//    allTransforms.clear();
//    for (auto& [name, md] : *models_data) {
//        md.firstInstance = 0;
//        md.instanceCount = 0;
//    }
//    rcd_batches.clear();
//
//    // 1. Сначала сгруппируй все объекты по ModelData*
//    std::unordered_map<ModelData*, std::vector<BaseSprite*>> model_batches;
//    for (auto* obj : *objects) {
//        auto it = models_data->find(obj->GetModelPath());
//        if (it == models_data->end()) continue;
//        ModelData* md = &it->second;
//        model_batches[md].push_back(obj);
//    }
//
//    // 2. Для каждого RenderCommandData
//    for (auto* rcd : all_rcd) {
//        std::vector<ModelData*>& models = rcd->models_data;
//        for (auto* md : models) {
//            auto group_it = model_batches.find(md);
//            if (group_it == model_batches.end()) continue; // Нет ни одного объекта такого типа в сцене
//
//            auto& group = group_it->second;
//            // Обновляем instance данные
//            md->firstInstance = safe_u32(allTransforms.size());
//            md->instanceCount = safe_u32(group.size());
//            for (auto* obj : group) {
//                obj->matrix_index = allTransforms.size();
//                allTransforms.push_back(obj->GetSpritePositon());
//            }
//            // Собираем батч для данного рендер-команда
//            rcd_batches[rcd].push_back(md);
//        }
//    }
//}


std::vector<glm::mat4> BaseScene::GetObjectsPosition()
{
	std::vector<glm::mat4> positions;
	for (const auto& sprite : sprites) {
		positions.push_back(sprite->GetSpritePositon());
	}
	return positions;
}

void BaseScene::UpdateTransforms(std::vector<glm::mat4>& allTransforms)
{
    for (auto* obj : sprites) {
        if (obj->dirty && obj->matrix_index >= 0 && obj->matrix_index < allTransforms.size()) {
            allTransforms[obj->matrix_index] = obj->position;
            obj->dirty = false;
        }
    }
}

//void BaseScene::StoreTransforms(std::vector<uint8_t>& scratch_buffer, std::vector<glm::mat4>& allTransforms)
//{
//    UpdateTransforms(allTransforms);
//    size_t sz = sizeof(glm::mat4) * allTransforms.size();
//    scratch_buffer.resize(sz);
//    memcpy(scratch_buffer.data(), allTransforms.data(), sz);
//}
void BaseScene::StoreTransforms(BufferManager* bm, BufferData* buffer_data, std::vector<glm::mat4>& allTransforms)
{
    UpdateTransforms(allTransforms);
    size_t sz = sizeof(glm::mat4) * allTransforms.size();
	bm->uploadToGPUBuffer(buffer_data, allTransforms.data(), safe_u32(sz));
}


BaseScene::~BaseScene()
{
	sprites.clear();
}
