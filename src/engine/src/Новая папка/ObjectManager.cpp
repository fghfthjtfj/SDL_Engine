#include "PCH.h"
#include "ObjectManager.h"
#include "RenderCommandData.h"
#include "TextureData.h"

SceneData* ObjectManager::CreateScene(const std::string& name) {
    auto [it, inserted] = scenes_data.emplace(name, std::make_unique<SceneData>());
    return it->second.get();
}

SceneData* ObjectManager::operator[](const std::string& name) {
    auto it = scenes_data.find(name);
    if (it != scenes_data.end()){
		return it->second.get();
	}
    SDL_Log("Scene '%s' not found!", name.c_str());

    return nullptr;
}

void ObjectManager::SetSceneState(const std::string& scene_name, bool is_active)
{
    auto scene = (*this)[scene_name];
    if (scene) {
        scene->is_active = is_active;
    }
    else {
        SDL_Log("Scene '%s' not found!", scene_name.c_str());
	}
	dirty = true;
}

SceneData* ObjectManager::GetActiveScene()
{
    for (auto& [name, scene] : scenes_data) {
        if (scene->is_active)
            return scene.get();
    }
    SDL_Log("No active scene found!");
    return nullptr; // не найдено
}

//PassBatchesResult ObjectManager::BuildPassBatches(SceneData* scene, RenderPassType pass)
//{
//    using BatchKey = std::tuple<ModelData*, TextureData*, TextureData*>;
//    std::unordered_map<BatchKey, PassBatchData> batches;
//
//    // Это "линейный" индекс экземпляра в порядке обхода сцены.
//    // Он должен совпадать с тем, как FillPIB_ForPass заполняет cache:
//    // cache[0] = total_positions;
//    // cache[1 + instanceIndex] — индекс позиции.
//    uint32_t instanceIndex = 0;
//
//    switch (pass)
//    {
//    case RenderPassType::Main:
//    {
//        ForEachArchetype<Positions, TextureComponent, ModelComponent>(
//            scene,
//            [&](ComponentArray<Positions, void>* posArr,
//                ComponentArray<TextureComponent, void>* texArr,
//                ComponentArray<ModelComponent, void>* modelArr)
//        {
//            const size_t count = posArr->size();
//
//            for (size_t i = 0; i < count; ++i)
//            {
//                ModelData* model = modelArr->data[i].model;
//                TextureData* albedo = texArr->data[i].texture;
//                TextureData* normal = texArr->data[i].normal;
//
//                BatchKey key{ model, albedo, normal };
//                auto& batch = batches[key];
//
//                if (batch.instanceCount == 0)
//                {
//                    batch.model = model;
//                    batch.firstInstance = instanceIndex; // базовый индекс в PIB
//                }
//
//                batch.instanceCount++;
//                instanceIndex++;
//            }
//        }
//        );
//        break;
//    }
//
//    case RenderPassType::Shadow:
//    {
//        // Набор компонентов совпадает с FillPIB_ForPass для Shadow
//        ForEachArchetype<Positions, ModelComponent, TextureComponent, ShadowComponent>(
//            scene,
//            [&](ComponentArray<Positions, void>* posArr,
//                ComponentArray<ModelComponent, void>* modelArr,
//                ComponentArray<TextureComponent, void>* texArr,
//                ComponentArray<ShadowComponent, void>*  /*shadowArr*/)
//        {
//            const size_t count = posArr->size();
//
//            for (size_t i = 0; i < count; ++i)
//            {
//                ModelData* model = modelArr->data[i].model;
//                TextureData* albedo = texArr->data[i].texture;
//                TextureData* normal = texArr->data[i].normal; // если в shadow-проходе нормаль не нужна — можно поставить nullptr
//
//                BatchKey key{ model, albedo, normal };
//                auto& batch = batches[key];
//
//                if (batch.instanceCount == 0)
//                {
//                    batch.model = model;
//                    batch.firstInstance = instanceIndex;
//                }
//
//                batch.instanceCount++;
//                instanceIndex++;
//            }
//        }
//        );
//        break;
//    }
//
//    default:
//        SDL_Log("ObjectManager::BuildPassBatches: unsupported RenderPassType");
//        break;
//    }
//
//    // Раскладываем в результат
//    PassBatchesResult result;
//    result.pass_batches_data.reserve(batches.size());
//    result.texture_bindings.reserve(batches.size());
//
//    for (auto& [key, batch] : batches)
//    {
//        result.pass_batches_data.push_back(batch);
//
//        auto [model, albedo, normal] = key;
//
//        std::vector<SDL_GPUTextureSamplerBinding> bindings;
//        if (albedo)
//            bindings.push_back({ albedo->texture, albedo->sampler });
//        if (normal)
//            bindings.push_back({ normal->texture, normal->sampler });
//
//        result.texture_bindings.push_back(std::move(bindings));
//    }
//
//    return result;
//}

PassBatchesResult ObjectManager::BuildPassBatches(
    SceneData* scene,
    const RenderPassName& rp_name,
    ShaderProgram* sp
)
{
    using BatchMap = std::unordered_map<BatchKey, PassBatchData, size_t(*)(const BatchKey&)>;
    BatchMap batches(0, &HashBatchKey);

    // Сцена -> группировка по ключу (model + albedo + normal) только для объектов,
    // у которых MaterialComponent указывает на материал с variants[rp_name] == sp
    ForEachArchetype<MaterialComponent, ModelComponent, TextureComponent>(
        scene,
        [&](ComponentArray<MaterialComponent, void>* matArr,
            ComponentArray<ModelComponent, void>* modelArr,
            ComponentArray<TextureComponent, void>* texArr)
    {
        const size_t count = matArr->size();
        for (size_t i = 0; i < count; ++i)
        {
            Material* mat = matArr->data[i].material;
            if (!mat) continue;

            auto it = mat->variants.find(rp_name);
            if (it == mat->variants.end()) continue;
            if (it->second != sp) continue;

            ModelData* model = modelArr->data[i].model;
            TextureData* albedo = texArr->data[i].texture;
            TextureData* normal = texArr->data[i].normal; // может быть nullptr

            BatchKey key{ model, albedo, normal };
            auto& batch = batches[key];

            if (batch.instanceCount == 0) {
                batch.model = model;
                // firstInstance назначим после сортировки/упаковки в вектор
            }

            batch.instanceCount++;
        }
    });

    // Переносим в вектор и делаем детерминированный порядок батчей,
    // чтобы ты мог в другом месте заполнить PIB в точно таком же порядке.
    std::vector<std::pair<BatchKey, PassBatchData>> ordered;
    ordered.reserve(batches.size());
    for (const auto& kv : batches) {
        ordered.emplace_back(kv.first, kv.second);
    }

    std::sort(ordered.begin(), ordered.end(),
        [](const auto& a, const auto& b)
    {
        const BatchKey& ka = a.first;
        const BatchKey& kb = b.first;
        if (ka.model != kb.model)   return ka.model < kb.model;
        if (ka.albedo != kb.albedo) return ka.albedo < kb.albedo;
        return ka.normal < kb.normal;
    }
    );

    PassBatchesResult result;
    result.pass_batches_data.reserve(ordered.size());
    result.texture_bindings.reserve(ordered.size());

    uint32_t cursor = 0; // диапазоны внутри этой RCD (если тебе нужно смещение по pass — делай это выше уровнем)

    for (auto& item : ordered)
    {
        const BatchKey& key = item.first;
        PassBatchData batch = item.second;

        batch.firstInstance = cursor;
        cursor += batch.instanceCount;

        result.pass_batches_data.push_back(batch);

        std::vector<SDL_GPUTextureSamplerBinding> bindings;
        if (key.albedo) bindings.push_back({ key.albedo->texture, key.albedo->sampler });
        if (key.normal) bindings.push_back({ key.normal->texture, key.normal->sampler });
        result.texture_bindings.push_back(std::move(bindings));
    }

    return result;
}