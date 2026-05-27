#pragma once
#include "ShaderData.h"
#include <unordered_map>
#include <string>
#include "SDL3/SDL_gpu.h"
#include "Aliases.h"
#include <memory>


class BufferManager;
struct RenderPassStep;

class ShaderManager
{
public:
	ShaderManager(SDL_GPUDevice* device);
	ShaderProgramDescription* CreateShaderProgramDescription(const std::string& name);
	VertexShaderData CreateVertexShader(const char* hlsl_path, std::initializer_list<VertexBufferBinding> bindings);
	FragmentShaderData CreateFragmentShader(const char* path);
	
	ShaderProgram* CreateShaderProgram(const std::string& name, ShaderProgramDescription* spd, BufferManager* bm,
		VertexShaderData vs, std::initializer_list<BufferDataName> vertex_shader_buffers,
		FragmentShaderData fs, std::initializer_list<BufferDataName> fragment_shader_buffers,
		std::initializer_list<TextureSlotRole> texture_slots
	);
	ComputeShaderData CreateComputeShader(const char* path);
	// Порядок создание ComputeShaderProgram не определяет порядок их выполнения в проходе!
	// The order in which ComputeShaderPrograms are created does not determine the order in which they are executed in a pass!
	ComputeShaderProgram* CreateComputeShaderProgram(const std::string& name,
		ComputeShaderData cs, 
		std::initializer_list<BufferData*> rw_storage_buffers,
		std::initializer_list<BufferData*> ro_storage_buffers,
		std::initializer_list<ComputeShaderProgram::ComputeRWTextureBinding> rw_storage_textures,
		std::initializer_list<TextureAtlas*> ro_storage_textures,
		std::initializer_list<TextureAtlas*> texture_samplers,
		ComputePassStep* associated_compute_pass);


	VertexShaderData CreateVertexShaderFromSPV(const char* path, std::initializer_list<VertexBufferBinding> bindings);
	FragmentShaderData CreateFragmentShaderFromSPV(const char* spv_path);
	ComputeShaderData CreateComputeShaderFromSPV(const char* spv_path);

	ShaderProgramDescription* GetShaderProgramDescription(const std::string& name);
	ShaderProgram* GetShaderProgram(const std::string& name);

	ComputeShaderProgram* GetComputeShaderProgram(const std::string& name);

	std::unordered_map<std::string, std::unique_ptr<ShaderProgram>>& GetShaderPrograms() { return shader_programs; }
	std::vector<std::unique_ptr<ComputeShaderProgram>>& GetComputeShaderPrograms() { return compute_shader_programs; };

	bool IsDirtyGraphicsPipelines() const { return dirty_graphics_pipelines; }
	void SetDirtyGraphicsPipelines(bool dirty) { dirty_graphics_pipelines = dirty; }
	bool IsDirtyComputePipelines() const { return dirty_compute_pipelines; }
	void SetDirtyComputePipelines(bool dirty) { dirty_compute_pipelines = dirty; }

	~ShaderManager();

private:
	VertexShaderData BuildVertexShader(const Uint8* spv, size_t spv_size, const char* dbg_name, std::initializer_list<VertexBufferBinding> bindings);

	FragmentShaderData BuildFragmentShader(const Uint8* spv, size_t spv_size, const char* dbg_name);

	ComputeShaderData BuildComputeShader(Uint8* spv, size_t spv_size, const char* dbg_name);

	std::string BuildCachePath(const char* source_path, uint64_t hash) const;
	void ReadVertexAttributes(std::initializer_list<ShaderBase::VertexBufferBinding> bindings, VertexShaderData& vs);

	Uint8* LoadOrCompileSPIRV(const char* hlsl_path, SDL_ShaderCross_ShaderStage stage, size_t& out_size);

	std::string m_cacheBasePath;

	std::unordered_map<std::string, std::unique_ptr<ShaderProgramDescription>> shader_program_descriptions;
	std::unordered_map<std::string, std::unique_ptr<ShaderProgram>> shader_programs;
	
	std::vector<std::unique_ptr<ComputeShaderProgram>> compute_shader_programs;
	std::unordered_map<std::string, ComputeShaderProgram*>  compute_shader_programs_by_name;

	SDL_GPUDevice* dev;

	bool dirty_graphics_pipelines = true;
	bool dirty_compute_pipelines = true;
};

