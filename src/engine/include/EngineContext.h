#pragma once
#include "BufferManager.h"
#include "TextureManager.h"
#include "RenderManager.h"
#include "MaterialManager.h"
#include "ObjectManager.h"
#include "ShaderManager.h"
#include "ModelManager.h"
#include "CameraManager.h"
#include "BatchBuilder.h"
#include "Aliases.h"


class EngineContext {
public:
	EngineContext(BufferManager* bm, TextureManager* tm, PassManager* pm, MaterialManager* mm, ObjectManager* om, ShaderManager* sm, ModelManager* md, CameraManager* cm, BatchBuilder* bb);

	TextureAtlas* CreateTextureAtlas(const AtlasName& name, SDL_GPUTextureCreateInfo tci, const std::string& sampler_name);
	TextureAtlas* CreateTextureAtlas(const AtlasName& name, const AtlasName& existing_atlas_name, const std::string& sampler_name);
	TextureHandle* CreateTextureFromFile(const TextureName& name, const AtlasName& atlas_name, const char* path);

	Material* CreateMaterial(std::string name, std::initializer_list<std::pair<TextureSlotRole, TextureName>> textures, std::initializer_list<ShaderName> shaders);

	ModelData* CreateModel(const ModelName& name, const char* model_path, const char* index_path);

	template<typename... Components>
	Entity CreateEntity(const std::string& scene_name, Components&&... comps) {
		constexpr bool needs_pib = contains_type_v<ModelComponent, Components...>
			&& contains_type_v<PositionProxy16, Components...>;

		if constexpr (needs_pib) {
			SceneData* active_scene = object_manager->GetActiveScene();
			SceneData* target_scene = object_manager->GetScene(scene_name);
			if (target_scene != nullptr && active_scene == target_scene) {
				batch_builder->SetPIBNeedUpload(true);
				batch_builder->SetDirtyBatches(true);
			}
		}

		return object_manager->CreateEntity(scene_name, std::forward<Components>(comps)...);
	}
	BufferManager* GetBufferManager() const { return buffer_manager; }
	TextureManager* GetTextureManager() const { return texture_manager; }
	ShaderManager* GetShaderManager() const { return shader_manager; }
	ModelManager* GetModelManager() const { return model_manager; }
	PassManager* GetRenderManager() const { return pass_manager; }
	ObjectManager* GetObjectManager() const { return object_manager; }
	CameraManager* GetCameraManager() const { return camera_manager; }
	MaterialManager* GetMaterialManager() const { return material_manager; }

	BatchBuilder* GetBatchBuilder() const { return batch_builder; }

private:
	BufferManager* buffer_manager = nullptr;
	TextureManager* texture_manager = nullptr;
	PassManager* pass_manager = nullptr;
	MaterialManager* material_manager = nullptr;
	ObjectManager* object_manager = nullptr;
	ShaderManager* shader_manager = nullptr;
	ModelManager* model_manager = nullptr;
	CameraManager* camera_manager = nullptr;

	BatchBuilder* batch_builder = nullptr;
};
