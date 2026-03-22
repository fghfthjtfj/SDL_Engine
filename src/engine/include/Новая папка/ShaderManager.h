#pragma once
#include "ShaderData.h"
#include <unordered_map>

class BufferManager;

class ShaderManager
{
public:
	ShaderManager(SDL_GPUDevice* device);
	FragmentShaderData CreateFragmentShader(const char* path);
	VertexShaderData CreateVertexShader(const char* path);

	ShaderProgram* CreateShaderProgram(const std::string& name, BufferManager* bm,
		VertexShaderData vs, std::initializer_list<const char*> vertex_shader_buffers, 
		FragmentShaderData fs, std::initializer_list<const char*> fragment_shader_buffers,
		SDL_GPUColorTargetDescription ctd, bool has_color_target);
	ShaderProgram* operator[](const std::string& name);
	
	~ShaderManager();

private:
	ShaderData CreateShader(const char* path);
	void ReadVertexAttributes(const Uint8* shader_code, size_t shader_size,
		SDL_GPUVertexBufferDescription& vb,
		std::vector<SDL_GPUVertexAttribute>& attributes);
	std::unordered_map<std::string, std::unique_ptr<ShaderProgram>> shader_programs;
	SDL_GPUDevice* dev;
};

