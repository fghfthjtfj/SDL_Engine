#pragma once
#include <unordered_set>
#include "ShaderData.h"

struct TextureHandle;

struct Material {
    std::unordered_map<TextureSlotRole, TextureHandle*> textures;
    std::unordered_set<ShaderProgram*> shader_programs;
};
