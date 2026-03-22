#pragma once
#include <unordered_map>
#include "ShaderData.h"
#include "MaterialData.h"


class MaterialManager {
public:
	MaterialManager();
	Material* RegisterMaterial(std::string name, const RenderPassName& rp_name, ShaderProgram* shader_program);
	std::vector<Material*> GetAllMaterials();
	Material* operator[](const std::string& name);
	~MaterialManager();
private:
	std::unordered_map<std::string, std::unique_ptr<Material>> materials;
};