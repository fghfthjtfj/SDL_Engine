#include "PCH.h"
#include "ObjectManager.h"
#include "RenderManager.h"
#include "PipeManager.h"
#include "TextureData.h"
#include "ModelData.h"

SceneData* ObjectManager::CreateScene(const SceneName& name) {
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

void ObjectManager::SetSceneState(const SceneName& scene_name, bool is_active)
{
    auto scene = (*this)[scene_name];
    if (scene) {
        scene->is_active = is_active;
    }
    else {
        SDL_Log("Scene '%s' not found!", scene_name.c_str());
	}
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
