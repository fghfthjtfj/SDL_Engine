#include "PCH.h"
#include "spirv_reflect.h"
#include "ShaderManager.h"

// Прямое создание GPU-шейдера из .spv БЕЗ shadercross-кросс-компиляции —
// точная копия старого загрузчика под новые структуры.
// ВНИМАНИЕ: сырой SPIR-V уходит прямо в SDL_CreateGPUShader, поэтому путь
// рабочий только когда бэкенд SDL_GPU реально потребляет SPIRV (Vulkan).
// На DXIL/MSL он упадёт — там нужен shadercross-путь.
ShaderData ShaderManager::CreateShaderFromSPV(const char* path, SDL_GPUShaderStage stage)
{
    size_t shader_size = 0;
    Uint8* shader_code = (Uint8*)SDL_LoadFile(path, &shader_size);
    if (!shader_code) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to load shader file: %s", path);
        return {};
    }

    const SDL_GPUShaderFormat shader_format = SDL_GetGPUShaderFormats(dev);
    if (!(shader_format & SDL_GPU_SHADERFORMAT_SPIRV)) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
            "CreateShaderFromSPV requires a SPIRV-capable backend (Vulkan): %s", path);
        SDL_free(shader_code);
        return {};
    }

    SDL_GPUShaderCreateInfo sci; SDL_zero(sci);
    sci.format = SDL_GPU_SHADERFORMAT_SPIRV;
    sci.entrypoint = "main";
    sci.code = shader_code;
    sci.code_size = shader_size;
    sci.stage = stage;

    // Автоопределение ресурсов через spirv_reflect (как в старом CreateShader)
    Uint32 num_samplers = 0;
    Uint32 num_storage_textures = 0;
    Uint32 num_storage_buffers = 0;
    Uint32 num_uniform_buffers = 0;

    SpvReflectShaderModule module;
    if (spvReflectCreateShaderModule(shader_size, shader_code, &module) != SPV_REFLECT_RESULT_SUCCESS) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "SPIRV-Reflect: Failed to parse SPIR-V: %s", path);
        SDL_free(shader_code);
        return {};
    }

    uint32_t count = 0;
    spvReflectEnumerateDescriptorBindings(&module, &count, nullptr);
    std::vector<SpvReflectDescriptorBinding*> bindings(count);
    if (count)
        spvReflectEnumerateDescriptorBindings(&module, &count, bindings.data());

    for (uint32_t i = 0; i < count; ++i) {
        switch (bindings[i]->descriptor_type) {
        case SPV_REFLECT_DESCRIPTOR_TYPE_SAMPLER:
        case SPV_REFLECT_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
            ++num_samplers; break;
        case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_IMAGE:
            ++num_storage_textures; break;
        case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_BUFFER:
            ++num_storage_buffers; break;
        case SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
            ++num_uniform_buffers; break;
        default: break;
        }
    }
    spvReflectDestroyShaderModule(&module);

    sci.num_samplers = num_samplers;
    sci.num_storage_textures = num_storage_textures;
    sci.num_storage_buffers = num_storage_buffers;
    sci.num_uniform_buffers = num_uniform_buffers;

    SDL_GPUShader* shader = SDL_CreateGPUShader(dev, &sci);
    if (!shader) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
            "SDL_CreateGPUShader failed: %s\n%s", path, SDL_GetError());
    }

    ShaderData shader_data = { shader, shader_size, shader_code };
    return shader_data;
}

VertexShaderData ShaderManager::CreateVertexShaderFromSPV(const char* path)
{
    ShaderData shader_data = CreateShaderFromSPV(path, SDL_GPU_SHADERSTAGE_VERTEX);

    VertexShaderData vs_data;
    vs_data.shader_data = shader_data;

    if (!shader_data.shader || !shader_data.shader_code) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to create vertex shader from SPV: %s", path);
        return vs_data;
    }

    ReadVertexAttributes(shader_data.shader_code, shader_data.shader_size, vs_data.vb, vs_data.attributes);

    SDL_free(shader_data.shader_code);
    vs_data.shader_data.shader_code = nullptr;
    return vs_data;
}

FragmentShaderData ShaderManager::CreateFragmentShaderFromSPV(const char* path)
{
    ShaderData shader_data = CreateShaderFromSPV(path, SDL_GPU_SHADERSTAGE_FRAGMENT);

    FragmentShaderData fs_data;
    fs_data.shader_data = shader_data;

    if (!shader_data.shader || !shader_data.shader_code) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to create fragment shader from SPV: %s", path);
        return fs_data;
    }

    SDL_free(shader_data.shader_code);
    fs_data.shader_data.shader_code = nullptr;
    return fs_data;
}

ComputeShaderData ShaderManager::CreateComputeShaderFromSPV(const char* path)
{
    size_t shader_size = 0;
    Uint8* shader_code = (Uint8*)SDL_LoadFile(path, &shader_size);
    if (!shader_code) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to load compute shader: %s", path);
        return {};
    }

    ComputeShaderData result;
    result.spv_code = shader_code;   // владение остаётся в структуре, пайплайн строится позже
    result.spv_size = shader_size;

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

// 4-аргументная перегрузка ReadVertexAttributes (spirv_reflect) — оставь её здесь же,
// как добавляли в прошлый раз.
void ShaderManager::ReadVertexAttributes(const Uint8* shader_code, size_t shader_size,
    SDL_GPUVertexBufferDescription& vb, std::vector<SDL_GPUVertexAttribute>& attributes)
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