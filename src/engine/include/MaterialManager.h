#pragma once
#include <unordered_map>
#include <vector>
#include "ShaderData.h"
#include "MaterialData.h"


class MaterialManager {
public:
	MaterialManager();
	// Число пар TextureSlotRole, TextureHandle* должно совпадать с числом required_slots в каждом ShaderProgram* из shader_programs. Порядок не важен, но все роли из required_slots должны быть представлены в textures.
	// TextureSlotRole, TextureHandle* count must match the number of required_slots in each ShaderProgram* in shader_programs. The order does not matter, but all roles from required_slots must be represented in textures.
	Material* CreateMaterial(std::string name, std::initializer_list<std::pair<TextureSlotRole, TextureHandle*>> textures, std::vector<ShaderProgram*> shader_programs);
	std::vector<Material*> GetAllMaterials();
	Material* GetMaterial(const std::string& name);
	~MaterialManager();
private:
	std::unordered_map<std::string, std::unique_ptr<Material>> materials;
};