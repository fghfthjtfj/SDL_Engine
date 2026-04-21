#include "PCH.h"
#include "ShaderManager.h"

inline uint64_t HashBytes(const uint8_t* data, size_t size) {
    uint64_t hash = 14695981039346656037ULL;
    for (size_t i = 0; i < size; i++) {
        hash ^= data[i];
        hash *= 1099511628211ULL;
    }
    return hash;
}

inline std::string BuildCachePath(const char* source_path, uint64_t hash) {
    // Вытаскиваем имя файла из пути
    const char* filename = source_path;
    for (const char* p = source_path; *p; ++p) {
        if (*p == '/' || *p == '\\') filename = p + 1;
    }

    char hash_str[17];
    SDL_snprintf(hash_str, sizeof(hash_str), "%016llx", (unsigned long long)hash);

    return std::string("../shaders/shader_cache/") + filename + "." + hash_str + ".spv";
}

ShaderData ShaderManager::CreateShaderInternal(const Uint8* code, size_t size, SDL_GPUShaderStage stage) {
    SDL_GPUShaderCreateInfo sci; SDL_zero(sci);

    const SDL_GPUShaderFormat supported = SDL_GetGPUShaderFormats(dev);
    if (supported & SDL_GPU_SHADERFORMAT_DXIL)  sci.format = SDL_GPU_SHADERFORMAT_DXIL;
    else if (supported & SDL_GPU_SHADERFORMAT_MSL)   sci.format = SDL_GPU_SHADERFORMAT_MSL;
    else if (supported & SDL_GPU_SHADERFORMAT_SPIRV) sci.format = SDL_GPU_SHADERFORMAT_SPIRV;
    else {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "No supported shader format found.");
        return {};
    }

    sci.entrypoint = "main";
    sci.code = code;
    sci.code_size = size;
    sci.stage = stage;

    if (sci.format == SDL_GPU_SHADERFORMAT_SPIRV) {
        SpvReflectShaderModule module;
        if (spvReflectCreateShaderModule(size, code, &module) != SPV_REFLECT_RESULT_SUCCESS) {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "SPIRV-Reflect: Failed to parse SPIR-V");
            return {};
        }

        uint32_t count = 0;
        spvReflectEnumerateDescriptorBindings(&module, &count, nullptr);
        std::vector<SpvReflectDescriptorBinding*> bindings(count);
        if (count)
            spvReflectEnumerateDescriptorBindings(&module, &count, bindings.data());

        for (auto* b : bindings) {
            switch (b->descriptor_type) {
            case SPV_REFLECT_DESCRIPTOR_TYPE_SAMPLER:
            case SPV_REFLECT_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER: ++sci.num_samplers;          break;
            case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_IMAGE:          ++sci.num_storage_textures;  break;
            case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_BUFFER:         ++sci.num_storage_buffers;   break;
            case SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER:         ++sci.num_uniform_buffers;   break;
            default: break;
            }
        }
        spvReflectDestroyShaderModule(&module);
    }

    ShaderData result;
    result.shader = SDL_CreateGPUShader(dev, &sci);
    return result;
}

VertexShaderData ShaderManager::CreateVertexShader(const char* path) {
    size_t size = 0;
    Uint8* code = (Uint8*)SDL_LoadFile(path, &size);
    if (!code) { SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to load: %s", path); return {}; }

    VertexShaderData vs;
    vs.shader_data = CreateShaderInternal(code, size, SDL_GPU_SHADERSTAGE_VERTEX);
    ReadVertexAttributes(code, size, vs.vb, vs.attributes);

    SDL_free(code);
    return vs;
}

FragmentShaderData ShaderManager::CreateFragmentShader(const char* path) {
    size_t size = 0;
    Uint8* code = (Uint8*)SDL_LoadFile(path, &size);
    if (!code) { SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to load: %s", path); return {}; }

    FragmentShaderData fs;
    fs.shader_data = CreateShaderInternal(code, size, SDL_GPU_SHADERSTAGE_FRAGMENT);

    SDL_free(code);
    return fs;
}

ComputeShaderData ShaderManager::CreateComputeShader(const char* path) {
    size_t size = 0;
    Uint8* code = (Uint8*)SDL_LoadFile(path, &size);
    if (!code) { SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to load: %s", path); return {}; }

    ComputeShaderData cs;
    ReadComputeMetadata(code, size, cs);
    cs.spv_code = code;
    cs.spv_size = size;
    return cs;
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
            attr.offset = safe_u32(offset); offset += 12; break;
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

void ShaderManager::ReadComputeMetadata(const Uint8* code, size_t size, ComputeShaderData& out) {
    SpvReflectShaderModule module;
    if (spvReflectCreateShaderModule(size, code, &module) != SPV_REFLECT_RESULT_SUCCESS) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "SPIRV-Reflect: failed to parse compute shader");
        return;
    }

    if (module.entry_point_count > 0) {
        out.threadcount_x = module.entry_points[0].local_size.x;
        out.threadcount_y = module.entry_points[0].local_size.y;
        out.threadcount_z = module.entry_points[0].local_size.z;
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
            ++out.num_samplers; break;
        case SPV_REFLECT_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
            ++out.num_readonly_storage_textures; break;
        case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_IMAGE:
            (b->decoration_flags & SPV_REFLECT_DECORATION_NON_WRITABLE)
                ? ++out.num_readonly_storage_textures
                : ++out.num_readwrite_storage_textures;
            break;
        case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_BUFFER:
            (b->decoration_flags & SPV_REFLECT_DECORATION_NON_WRITABLE)
                ? ++out.num_readonly_storage_buffers
                : ++out.num_readwrite_storage_buffers;
            break;
        case SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
            ++out.num_uniform_buffers; break;
        default: break;
        }
    }
    spvReflectDestroyShaderModule(&module);
}