#include "PCH.h"
#include "MaterialManager.h"

MaterialManager::MaterialManager()
{
}

Material* MaterialManager::RegisterMaterial(std::string name, const RenderPassName& rp_name, ShaderProgram* shader_program){
	auto it = materials.find(name);
	if (it == materials.end()) {
		auto data = std::make_unique<Material>();
		data->variants[rp_name] = shader_program;
		materials[name] = std::move(data);
		return materials[name].get();
	}
	else {
		it->second->variants[rp_name] = shader_program;
		SDL_Log("Material '%s' already exists, added/updated variant for render pass '%s'.", name.c_str(), rp_name.c_str());
		return it->second.get();
	}
}

std::vector<Material*> MaterialManager::GetAllMaterials()
{
    std::vector<Material*> result;
	result.reserve(materials.size());
    for (auto& [name, material] : materials) {
        result.push_back(material.get());
	}
	return result;
}

Material* MaterialManager::operator[](const std::string& name)
{
	return nullptr;
}

MaterialManager::~MaterialManager()
{
}
