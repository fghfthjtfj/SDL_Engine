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
	FragmentShaderData CreateFragmentShader(const char* path);
	VertexShaderData CreateVertexShader(const char* path);
	ShaderProgramDescription* CreateShaderProgramDescription(const std::string& name, bool enable_depth_test, bool enable_depth_write, bool enable_stencil_test, bool has_color_target, RenderPassStep* rp);
	
	ShaderProgram* CreateShaderProgram(const std::string& name, ShaderProgramDescription* spd, BufferManager* bm,
		VertexShaderData vs, std::initializer_list<BufferDataName> vertex_shader_buffers,
		FragmentShaderData fs, std::initializer_list<BufferDataName> fragment_shader_buffers,
		std::initializer_list<TextureSlotRole> texture_slots
	);
	ComputeShaderData CreateComputeShader(const char* path);
	// Порядок создание ComputeShaderProgram не определяет порядок их выполнения в проходе!
	// The order in which ComputeShaderPrograms are created does not determine the order in which they are executed in a pass!
	ComputeShaderProgram* CreateComputeShaderProgram(const std::string& name, BufferManager* bm,
		ComputeShaderData cs, 
		std::initializer_list<BufferDataName> rw_storage_buffers,
		std::initializer_list<BufferDataName> ro_storage_buffers,

		ComputePassStep* associated_compute_pass);

	ShaderProgramDescription* GetShaderProgramDescription(const std::string& name);
	ShaderProgram* GetShaderProgram(const std::string& name);

	ComputeShaderProgram* GetComputeShaderProgram(const std::string& name);

	std::unordered_map<std::string, std::unique_ptr<ShaderProgram>>& GetShaderPrograms() { return shader_programs; }
	std::unordered_map<std::string, std::unique_ptr<ComputeShaderProgram>>& GetComputeShaderPrograms() { return compute_shader_programs; }

	bool IsDirtyGraphicsPipelines() const { return dirty_graphics_pipelines; }
	void SetDirtyGraphicsPipelines(bool dirty) { dirty_graphics_pipelines = dirty; }
	bool IsDirtyComputePipelines() const { return dirty_compute_pipelines; }
	void SetDirtyComputePipelines(bool dirty) { dirty_compute_pipelines = dirty; }

	~ShaderManager();

private:
	void ReadVertexAttributes(const Uint8* shader_code, size_t shader_size, SDL_GPUVertexBufferDescription& vb,std::vector<SDL_GPUVertexAttribute>& attributes);
	void ReadComputeMetadata(const Uint8* code, size_t size, ComputeShaderData& out);
	ShaderData CreateShaderInternal(const Uint8* code, size_t size, SDL_GPUShaderStage stage);

	std::unordered_map<std::string, std::unique_ptr<ShaderProgramDescription>> shader_program_descriptions;
	std::unordered_map<std::string, std::unique_ptr<ShaderProgram>> shader_programs;
	
	std::unordered_map<std::string, std::unique_ptr<ComputeShaderProgram>> compute_shader_programs;
	SDL_GPUDevice* dev;

	bool dirty_graphics_pipelines = true;
	bool dirty_compute_pipelines = true;
};

