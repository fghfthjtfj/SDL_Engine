#include "PCH.h"
#include "EngineContext.h"

EngineContext::EngineContext(BufferManager* bm, TextureManager* tm, PassManager* rm, MaterialManager* mm, ObjectManager* om, ShaderManager* sm, ModelManager* md, CameraManager* cm, PipeManager* pm, BatchBuilder* bb)
{
	this->buffer_manager = bm;
	this->texture_manager = tm;
	this->pass_manager = rm;
	this->material_manager = mm;
	this->object_manager = om;
	this->shader_manager = sm;
	this->model_manager = md;
	this->camera_manager = cm;
	this->pipe_manager = pm;

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
	shader_programs.reserve(shaders.size());
	for (const auto& shader_name : shaders) {
		ShaderProgram* sp = shader_manager->GetShaderProgram(shader_name);
		if (!sp) {
			SDL_Log("EngineContext::Creating material with non existing shader program");
			continue;
		}
		shader_programs.push_back(sp);
	}
	std::vector<std::pair<TextureSlotRole, TextureHandle*>> texture_handles;
	texture_handles.reserve(textures.size());
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

void EngineContext::DeleteEntity(SceneData* scene, Entity e)
{
	const bool needs_pib = object_manager->Has<ModelComponent>(scene, e)
		&& object_manager->Has<Positions>(scene, e);

	if (needs_pib && scene == object_manager->GetActiveScene()) {
		batch_builder->SetPIBNeedUpload(true);
		batch_builder->SetDirtyBatches(true);
	}

	object_manager->DeleteEntity(scene, e);
}

void EngineContext::CreateGraphicsPipelines()
{
	if (!shader_manager->IsDirtyGraphicsPipelines()) {
		return;
	}
	
	auto& shader_programs = shader_manager->GetShaderPrograms();
	pipe_manager->CreateGraphicsPiplenes(shader_programs);
	shader_manager->SetDirtyGraphicsPipelines(false);
}

void EngineContext::CreateComputePipelines()
{
	if (!shader_manager->IsDirtyComputePipelines()) {
		return;
	}
	auto& compute_shader_programs = shader_manager->GetComputeShaderPrograms();
	pipe_manager->CreateComputePipelines(compute_shader_programs);
	shader_manager->SetDirtyComputePipelines(false);
}
