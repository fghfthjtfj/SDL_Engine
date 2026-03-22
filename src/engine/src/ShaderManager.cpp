#include "PCH.h"
#include "spirv_reflect.h"
#include "ShaderManager.h"
#include "BufferManager.h"

ShaderManager::ShaderManager(SDL_GPUDevice* device) {
    dev = device;
};


ShaderData ShaderManager::CreateShader(const char* path) {
    size_t shader_size = 0;
    Uint8* shader_code = (Uint8*)SDL_LoadFile(path, &shader_size);

    if (!shader_code) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to load shader file: %s", path);
        return {};
    }

    SDL_GPUShaderCreateInfo sci; SDL_zero(sci);
    const SDL_GPUShaderFormat shader_format = SDL_GetGPUShaderFormats(dev);
    if (shader_format & SDL_GPU_SHADERFORMAT_DXIL) {
        sci.format = SDL_GPU_SHADERFORMAT_DXIL;
    }
    else if (shader_format & SDL_GPU_SHADERFORMAT_MSL) {
        sci.format = SDL_GPU_SHADERFORMAT_MSL;
    }
    else if (shader_format & SDL_GPU_SHADERFORMAT_SPIRV) {
        sci.format = SDL_GPU_SHADERFORMAT_SPIRV;
    }
    else {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "No supported shader format found.");
        SDL_free(shader_code);
        return {};
    }
    sci.entrypoint = "main";
    sci.code = shader_code;
    sci.code_size = shader_size;

    // Автоматическое определение ресурсов для SPIR-V
    Uint32 num_samplers = 0;
    Uint32 num_storage_textures = 0;
    Uint32 num_storage_buffers = 0;
    Uint32 num_uniform_buffers = 0;

    if (sci.format == SDL_GPU_SHADERFORMAT_SPIRV) {
        SpvReflectShaderModule module;
        SpvReflectResult result = spvReflectCreateShaderModule(shader_size, shader_code, &module);
        if (result != SPV_REFLECT_RESULT_SUCCESS) {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "SPIRV-Reflect: Failed to parse SPIR-V for file: %s", path);
            SDL_free(shader_code);
            return {};
        }

        uint32_t count = 0;
        spvReflectEnumerateDescriptorBindings(&module, &count, NULL);
        std::vector<SpvReflectDescriptorBinding*> bindings(count);
        if (count > 0) {
            spvReflectEnumerateDescriptorBindings(&module, &count, (SpvReflectDescriptorBinding**)bindings.data());
        }
        for (uint32_t i = 0; i < count; ++i) {
            auto* b = bindings[i];
            switch (b->descriptor_type) {
            case SPV_REFLECT_DESCRIPTOR_TYPE_SAMPLER:
            case SPV_REFLECT_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
                ++num_samplers;
                break;
            case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_IMAGE:
                ++num_storage_textures;
                break;
            case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_BUFFER:
                ++num_storage_buffers;
                break;
            case SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
                ++num_uniform_buffers;
                break;
            default:
                break;
            }
        }
        spvReflectDestroyShaderModule(&module);
    }
    // Для других форматов — оставить по 0, либо расширить по необходимости.

    sci.num_samplers = num_samplers;
    sci.num_storage_textures = num_storage_textures;
    sci.num_storage_buffers = num_storage_buffers;
    sci.num_uniform_buffers = num_uniform_buffers;

    if (SDL_strstr(path, ".vert")) {
        sci.stage = SDL_GPU_SHADERSTAGE_VERTEX;
    }
    else if (SDL_strstr(path, ".frag")) {
        sci.stage = SDL_GPU_SHADERSTAGE_FRAGMENT;
    }
    else {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Unknown shader stage for file: %s", path);
        SDL_free(shader_code);
        return {};
    }
    SDL_GPUShader* shader = SDL_CreateGPUShader(dev, &sci);
	ShaderData shader_data = { shader, shader_size, shader_code };
    return shader_data;
}



FragmentShaderData ShaderManager::CreateFragmentShader(const char* path)
{
    ShaderData shader_data = CreateShader(path);

	FragmentShaderData fs_data;
    fs_data.shader_data = shader_data;

    if (!shader_data.shader || !shader_data.shader_code) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to create fragment shader or load code.");
        return fs_data;
	}

    SDL_free(shader_data.shader_code);
    fs_data.shader_data.shader_code = nullptr;
    return fs_data;
}

VertexShaderData ShaderManager::CreateVertexShader(const char* path)
{
    ShaderData shader_data = CreateShader(path);

    VertexShaderData vs_data;
    vs_data.shader_data = shader_data;

    if (!shader_data.shader || !shader_data.shader_code) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to create vertex shader or load code.");
        return vs_data;
    }

    ReadVertexAttributes(shader_data.shader_code, shader_data.shader_size, vs_data.vb, vs_data.attributes);

    SDL_free(shader_data.shader_code);
    vs_data.shader_data.shader_code = nullptr;

    return vs_data;
}

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

ComputeShaderData ShaderManager::CreateComputeShader(const char* path)
{
    size_t shader_size = 0;
    Uint8* shader_code = (Uint8*)SDL_LoadFile(path, &shader_size);
    if (!shader_code) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to load compute shader: %s", path);
        return {};
    }

    ComputeShaderData result;
    result.shader_data.shader_code = shader_code;
    result.shader_data.shader_size = shader_size;

    const SDL_GPUShaderFormat fmt = SDL_GetGPUShaderFormats(dev);
    if (!(fmt & SDL_GPU_SHADERFORMAT_SPIRV))
        return result;

    SpvReflectShaderModule module;
    if (spvReflectCreateShaderModule(shader_size, shader_code, &module) != SPV_REFLECT_RESULT_SUCCESS) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "SPIRV-Reflect failed for: %s", path);
        SDL_free(shader_code);
        return {};
    }

    if (module.entry_point_count > 0) {
        result.threadcount_x = module.entry_points[0].local_size.x;
        result.threadcount_y = module.entry_points[0].local_size.y;
        result.threadcount_z = module.entry_points[0].local_size.z;
    }

    uint32_t count = 0;
    spvReflectEnumerateDescriptorBindings(&module, &count, nullptr);
    std::vector<SpvReflectDescriptorBinding*> bindings(count);
    if (count)
        spvReflectEnumerateDescriptorBindings(&module, &count, bindings.data());

    for (auto* b : bindings) {
        switch (b->descriptor_type) {
        case SPV_REFLECT_DESCRIPTOR_TYPE_SAMPLER:
        case SPV_REFLECT_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
            ++result.num_samplers; break;
        case SPV_REFLECT_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
            ++result.num_readonly_storage_textures; break;
        case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_IMAGE:
            (b->decoration_flags & SPV_REFLECT_DECORATION_NON_WRITABLE)
                ? ++result.num_readonly_storage_textures
                : ++result.num_readwrite_storage_textures;
            break;
        case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_BUFFER:
            (b->decoration_flags & SPV_REFLECT_DECORATION_NON_WRITABLE)
                ? ++result.num_readonly_storage_buffers
                : ++result.num_readwrite_storage_buffers;
            break;
        case SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
            ++result.num_uniform_buffers; break;
        default: break;
        }
    }
    spvReflectDestroyShaderModule(&module);
    return result;
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

ComputeShaderProgram* ShaderManager::GetComputeShaderData(const std::string& name)
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
	shader_programs.clear();
}

void ShaderManager::ReadVertexAttributes(const Uint8* shader_code, size_t shader_size, SDL_GPUVertexBufferDescription& vb, std::vector<SDL_GPUVertexAttribute>& attributes)
{
    SpvReflectShaderModule module;
    if (spvReflectCreateShaderModule(shader_size, shader_code, &module) != SPV_REFLECT_RESULT_SUCCESS) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "SPIRV-Reflect: Failed to parse SPIR-V for vertex attribute reflection.");
        return;
    }

    uint32_t count = 0;
    spvReflectEnumerateInputVariables(&module, &count, nullptr);
    std::vector<SpvReflectInterfaceVariable*> vars(count);
    if (count)
        spvReflectEnumerateInputVariables(&module, &count, (SpvReflectInterfaceVariable**)vars.data());

    attributes.clear();
    std::sort(vars.begin(), vars.end(),
        [](const SpvReflectInterfaceVariable* a, const SpvReflectInterfaceVariable* b) {
            return a->location < b->location;
        });

    size_t offset = 0;
    for (uint32_t i = 0; i < count; ++i) {
        const auto* var = vars[i];
        if (var->decoration_flags & SPV_REFLECT_DECORATION_BUILT_IN) continue;

        SDL_GPUVertexAttribute attr = {};
        attr.buffer_slot = 0;
        attr.location = var->location;
        switch (var->format) {
        case SPV_REFLECT_FORMAT_R32G32B32_SFLOAT:
            attr.format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3;
            attr.offset = safe_u32(offset);offset += 12; break;
        case SPV_REFLECT_FORMAT_R32G32_SFLOAT:
            attr.format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2;
            attr.offset = safe_u32(offset); offset += 8; break;
        case SPV_REFLECT_FORMAT_R32G32B32A32_SFLOAT:
            attr.format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4;
            attr.offset = safe_u32(offset); offset += 16; break;
        default: break;
        }
        attributes.push_back(attr);
    }
    spvReflectDestroyShaderModule(&module);

    SDL_zero(vb);
    vb.slot = 0;
    vb.input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX;
    vb.pitch = (Uint32)offset;
    vb.instance_step_rate = 0;
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
