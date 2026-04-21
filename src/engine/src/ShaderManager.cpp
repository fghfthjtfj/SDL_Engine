#include "PCH.h"
#include "ShaderManager.h"
#include "BufferManager.h"
#include <filesystem>

ShaderManager::ShaderManager(SDL_GPUDevice* device) {
    dev = device;

    SDL_ShaderCross_Init();

    // === НАДЁЖНЫЙ путь к папке кэша ===
    const char* base = SDL_GetBasePath();                    // папка, где лежит .exe

    m_cacheBasePath = std::string(base) + "shaders/shader_cache";

    std::filesystem::create_directories(m_cacheBasePath);
    SDL_Log("[Shader] Shader cache directory: %s", m_cacheBasePath.c_str());
    SDL_free((void*)base);
};

ShaderProgramDescription* ShaderManager::CreateShaderProgramDescription(const std::string& name, bool enable_depth_test, bool enable_depth_write, bool enable_stencil_test, bool has_color_target, RenderPassStep* rp)
{
    auto it = shader_program_descriptions.find(name);
    if (it != shader_program_descriptions.end()) {
        SDL_Log("Shader program description '%s' already exists, returning existing description.", name.c_str());
        return it->second.get();
    }

	auto description = std::make_unique<ShaderProgramDescription>();
	description->enable_depth_test = enable_depth_test;
	description->enable_depth_write = enable_depth_write;
	description->enable_stencil_test = enable_stencil_test;
	description->has_color_target = has_color_target;
    description->associated_render_pass = rp;
    ShaderProgramDescription* ptr = description.get();
    shader_program_descriptions.emplace(name, std::move(description));
	return ptr;
}

ShaderProgram* ShaderManager::CreateShaderProgram(const std::string& name, ShaderProgramDescription* spd, BufferManager* bm,
    VertexShaderData vs, std::initializer_list<const char*> vertex_shader_buffers, 
    FragmentShaderData fs, std::initializer_list<const char*> fragment_shader_buffers, 
    std::initializer_list<TextureSlotRole> texture_slots)
{
    auto it = shader_programs.find(name);
    if (it != shader_programs.end()) {
        SDL_Log("Shader program '%s' already exists, returning existing program.", name.c_str());
        return it->second.get();
    }

    auto program = std::make_unique<ShaderProgram>();
    program->vs = vs;
    program->fs = fs;
	program->vertex_shader_buffers.reserve(vertex_shader_buffers.size());
	for (const char* buffer_name : vertex_shader_buffers) {
		program->vertex_shader_buffers.push_back(bm->GetBufferData(buffer_name));
	}
	program->fragment_shader_buffers.reserve(fragment_shader_buffers.size());
	for (const char* buffer_name : fragment_shader_buffers) {
		program->fragment_shader_buffers.push_back(bm->GetBufferData(buffer_name));
	}
    for (TextureSlotRole role : texture_slots) {
        program->required_slots.push_back(role);
	}
	program->spd = spd;
    ShaderProgram* ptr = program.get();

    shader_programs.emplace(name, std::move(program));

	dirty_graphics_pipelines = true;
    return ptr;
}

ComputeShaderProgram* ShaderManager::CreateComputeShaderProgram(const std::string& name, BufferManager* bm,
    ComputeShaderData cs, std::initializer_list<BufferDataName> rw_storage_buffers, std::initializer_list<BufferDataName> ro_storage_buffers,
    ComputePassStep* associated_compute_pass)
{
    auto it = compute_shader_programs.find(name);
    if (it != compute_shader_programs.end()) {
        SDL_Log("Compute shader program '%s' already exists, returning existing.", name.c_str());
        return it->second.get();
    }

    auto result = std::make_unique<ComputeShaderProgram>();
    result->cs = cs;
    result->ro_storage_buffers.reserve(ro_storage_buffers.size());
    for (const char* buffer_name : ro_storage_buffers) {
        result->ro_storage_buffers.push_back(bm->GetBufferData(buffer_name));
    }

    result->rw_storage_buffers.reserve(rw_storage_buffers.size());
    for (const char* buffer_name : rw_storage_buffers) {
        result->rw_storage_buffers.push_back(bm->GetBufferData(buffer_name));
    }
    result->associated_compute_pass = associated_compute_pass;

    ComputeShaderProgram* ptr = result.get();
    compute_shader_programs.emplace(name, std::move(result));

    dirty_compute_pipelines = true;
    return ptr;
}

ShaderProgramDescription* ShaderManager::GetShaderProgramDescription(const std::string& name)
{
    auto it = shader_program_descriptions.find(name);
    if (it != shader_program_descriptions.end())
        return it->second.get();
    SDL_Log("Shader program description '%s' not found", name.c_str());
	return nullptr;
}

ShaderProgram* ShaderManager::GetShaderProgram(const std::string& name)
{
    auto it = shader_programs.find(name);
    if (it != shader_programs.end())
        return it->second.get();
    SDL_Log("Shader program '%s' not found", name.c_str());
    return nullptr;
}

ComputeShaderProgram* ShaderManager::GetComputeShaderProgram(const std::string& name)
{
    auto it = compute_shader_programs.find(name);
    if (it != compute_shader_programs.end())
        return it->second.get();
    SDL_Log("Compute shader data '%s' not found", name.c_str());
	return nullptr;
}

ShaderManager::~ShaderManager()
{
	for (auto& pair : shader_programs) {
		auto& prog = pair.second;

		if (prog->vs.shader_data.shader)
			SDL_ReleaseGPUShader(dev, prog->vs.shader_data.shader);
		if (prog->fs.shader_data.shader)
			SDL_ReleaseGPUShader(dev, prog->fs.shader_data.shader);

		if (!prog->vs.attributes.empty())
			prog->vs.attributes.clear();
	}
    for (auto& pair : compute_shader_programs) {
        auto& prog = pair.second;
        if (prog->cs.spv_code) {
            SDL_free(prog->cs.spv_code);
        }
	}
	shader_programs.clear();
	SDL_ShaderCross_Quit();
}
