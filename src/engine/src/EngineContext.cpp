#include "PCH.h"
#include "EngineContext.h"

EngineContext::EngineContext(BufferManager* bm, TextureManager* tm, PassManager* pm, MaterialManager* mm, ObjectManager* om, ShaderManager* sm, ModelManager* md, CameraManager* cm, BatchBuilder* bb)
{
	this->buffer_manager = bm;
	this->texture_manager = tm;
	this->pass_manager = pm;
	this->material_manager = mm;
	this->object_manager = om;
	this->shader_manager = sm;
	this->model_manager = md;
	this->camera_manager = cm;

	this->batch_builder = bb;
}

TextureAtlas* EngineContext::CreateTextureAtlas(const AtlasName& name, SDL_GPUTextureCreateInfo tci, const std::string& sampler_name)
{
	auto sampler = texture_manager->GetSampler(sampler_name);
	return texture_manager->CreateTextureAtlas(name, tci, sampler);
}

TextureAtlas* EngineContext::CreateTextureAtlas(const AtlasName& name, const AtlasName& existing_atlas_name, const std::string& sampler_name)
{
	auto sampler = texture_manager->GetSampler(sampler_name);
	TextureAtlas* existing_atlas = texture_manager->GetTextureAtlas(existing_atlas_name);
	return texture_manager->CreateTextureAtlas(name, existing_atlas, sampler);
}

TextureHandle* EngineContext::CreateTextureFromFile(const TextureName& name, const AtlasName& atlas_name, const char* path) {
	return texture_manager->CreateTextureFromFile(name, atlas_name, path);
}


Material* EngineContext::CreateMaterial(std::string name, std::initializer_list<std::pair<TextureSlotRole, TextureName>> textures, std::initializer_list<ShaderName> shaders)
{
	std::vector<ShaderProgram*> shader_programs;
	for (const auto& shader_name : shaders) {
		ShaderProgram* sp = shader_manager->GetShaderProgram(shader_name);
		if (!sp) {
			SDL_Log("EngineContext::Creating material with non existing shader program");
			continue;
		}
		shader_programs.push_back(sp);
	}
	std::vector<std::pair<TextureSlotRole, TextureHandle*>> texture_handles;
	for (const auto& [role, texture_name] : textures) {
		TextureHandle* handle = texture_manager->GetTextureHandle(texture_name);
		if (!handle) {
			SDL_Log("EngineContext::Creating material with non existing texture '%s'", texture_name.c_str());
			continue;
		}
		texture_handles.emplace_back(role, handle);
	}
	return material_manager->CreateMaterial(name, texture_handles, shader_programs);
}

ModelData* EngineContext::CreateModel(const ModelName& name, const char* model_path, const char* index_path)
{
	return model_manager->CreateModel(name, model_path, index_path);
}
