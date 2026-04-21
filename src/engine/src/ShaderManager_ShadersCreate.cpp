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

std::string ShaderManager::BuildCachePath(const char* source_path, uint64_t hash) const
{
    // Имя файла без пути
    const char* filename = source_path;
    for (const char* p = source_path; *p; ++p) {
        if (*p == '/' || *p == '\\') filename = p + 1;
    }

    char hash_str[17];
    SDL_snprintf(hash_str, sizeof(hash_str), "%016llx", (unsigned long long)hash);

    // Всегда используем / — SDL и Windows это понимают
    return m_cacheBasePath + "/" + filename + "." + hash_str + ".spv";
}

void ShaderManager::ReadVertexAttributes(
    const SDL_ShaderCross_GraphicsShaderMetadata* metadata,
    SDL_GPUVertexBufferDescription& vb,
    std::vector<SDL_GPUVertexAttribute>& attributes)
{
    attributes.clear();
    if (!metadata || metadata->num_inputs == 0) {
        SDL_zero(vb);
        vb.slot = 0;
        vb.input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX;
        vb.pitch = 0;
        vb.instance_step_rate = 0;
        return;
    }

    // Собираем и сортируем по location (как было в старом коде)
    std::vector<const SDL_ShaderCross_IOVarMetadata*> input_vars;
    input_vars.reserve(metadata->num_inputs);
    for (Uint32 i = 0; i < metadata->num_inputs; ++i) {
        input_vars.push_back(&metadata->inputs[i]);
    }
    std::sort(input_vars.begin(), input_vars.end(),
        [](const SDL_ShaderCross_IOVarMetadata* a, const SDL_ShaderCross_IOVarMetadata* b) {
        return a->location < b->location;
    });

    size_t offset = 0;
    for (const auto* var : input_vars) {
        SDL_GPUVertexAttribute attr{};
        attr.buffer_slot = 0;
        attr.location = var->location;

        // Маппинг типов из SDL_ShaderCross_IOVarType → SDL_GPUVertexElementFormat
        // (расширенный по сравнению со старым кодом, поддерживает UINT/INT)
        SDL_GPUVertexElementFormat format = SDL_GPU_VERTEXELEMENTFORMAT_INVALID;
        uint32_t elem_size = 0;

        switch (var->vector_type) {
        case SDL_SHADERCROSS_IOVAR_TYPE_FLOAT32:
            switch (var->vector_size) {
            case 1: format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT;  elem_size = 4; break;
            case 2: format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2; elem_size = 8; break;
            case 3: format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3; elem_size = 12; break;
            case 4: format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4; elem_size = 16; break;
            }
            break;
        case SDL_SHADERCROSS_IOVAR_TYPE_UINT32:
            switch (var->vector_size) {
            case 1: format = SDL_GPU_VERTEXELEMENTFORMAT_UINT;  elem_size = 4; break;
            case 2: format = SDL_GPU_VERTEXELEMENTFORMAT_UINT2; elem_size = 8; break;
            case 3: format = SDL_GPU_VERTEXELEMENTFORMAT_UINT3; elem_size = 12; break;
            case 4: format = SDL_GPU_VERTEXELEMENTFORMAT_UINT4; elem_size = 16; break;
            }
            break;
        case SDL_SHADERCROSS_IOVAR_TYPE_INT32:
            switch (var->vector_size) {
            case 1: format = SDL_GPU_VERTEXELEMENTFORMAT_INT;  elem_size = 4; break;
            case 2: format = SDL_GPU_VERTEXELEMENTFORMAT_INT2; elem_size = 8; break;
            case 3: format = SDL_GPU_VERTEXELEMENTFORMAT_INT3; elem_size = 12; break;
            case 4: format = SDL_GPU_VERTEXELEMENTFORMAT_INT4; elem_size = 16; break;
            }
            break;
        default:
            SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION,
                "Unsupported vertex input type %d (vector_size %u) at location %u",
                var->vector_type, var->vector_size, var->location);
            continue;
        }

        if (format == SDL_GPU_VERTEXELEMENTFORMAT_INVALID) continue;

        attr.format = format;
        attr.offset = safe_u32(offset);
        offset += elem_size;
        attributes.push_back(attr);
    }

    SDL_zero(vb);
    vb.slot = 0;
    vb.input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX;
    vb.pitch = safe_u32(offset);
    vb.instance_step_rate = 0;
}

Uint8* ShaderManager::LoadOrCompileSPIRV(const char* hlsl_path,
    SDL_ShaderCross_ShaderStage stage,
    size_t& out_size)
{
    size_t src_size = 0;
    char* src = (char*)SDL_LoadFile(hlsl_path, &src_size);
    if (!src) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to load HLSL: %s", hlsl_path);
        return nullptr;
    }

    // === Улучшенный хэш: учитываем не только текст, но и целевую платформу/GPU ===
    const SDL_GPUShaderFormat supported = SDL_GetGPUShaderFormats(dev);
    uint64_t hash = HashBytes((const uint8_t*)src, src_size);

    // Добавляем информацию о GPU-формате в хэш (DXIL / MSL / SPIRV)
    // Это гарантирует, что при смене видеокарты/ОС кэш пересоберётся
    hash ^= (uint64_t)supported;
    hash *= 1099511628211ULL;   // чтобы изменение формата сильно меняло хэш

    std::string cache_path = BuildCachePath(hlsl_path, hash);

    // Пробуем кэш
    Uint8* spv = (Uint8*)SDL_LoadFile(cache_path.c_str(), &out_size);
    if (spv) {
        SDL_Log("[Shader] Cache hit: %s", cache_path.c_str());
        SDL_free(src);
        return spv;
    }

    // Компилируем HLSL → SPIR-V
    SDL_ShaderCross_HLSL_Info hlsl_info{};
    hlsl_info.source = src;
    hlsl_info.entrypoint = "main";
    hlsl_info.shader_stage = stage;
    hlsl_info.include_dir = nullptr;
    hlsl_info.defines = nullptr;
    hlsl_info.props = 0;

    size_t compiled_size = 0;
    void* compiled = SDL_ShaderCross_CompileSPIRVFromHLSL(&hlsl_info, &compiled_size);
    SDL_free(src);

    if (!compiled) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
            "ShaderCross compile failed: %s\n%s", hlsl_path, SDL_GetError());
        return nullptr;
    }

    // Сохраняем в кэш
    SDL_IOStream* f = SDL_IOFromFile(cache_path.c_str(), "wb");
    if (f) {
        SDL_WriteIO(f, compiled, compiled_size);
        SDL_CloseIO(f);
        SDL_Log("[Shader] Compiled and cached: %s", cache_path.c_str());
    }

    out_size = compiled_size;
    return (Uint8*)compiled;
}

VertexShaderData ShaderManager::CreateVertexShader(const char* hlsl_path) {
    size_t spv_size = 0;
    Uint8* spv_code = LoadOrCompileSPIRV(hlsl_path, SDL_SHADERCROSS_SHADERSTAGE_VERTEX, spv_size);
    if (!spv_code) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to load/compile vertex shader: %s", hlsl_path);
        return {};
    }

    VertexShaderData vs{};
    SDL_ShaderCross_GraphicsShaderMetadata* metadata =
        SDL_ShaderCross_ReflectGraphicsSPIRV(spv_code, spv_size, 0);

    if (!metadata) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to reflect vertex shader: %s", hlsl_path);
        SDL_free(spv_code);
        return {};
    }

    ReadVertexAttributes(metadata, vs.vb, vs.attributes);

    SDL_ShaderCross_SPIRV_Info spirv_info{};
    spirv_info.bytecode = spv_code;
    spirv_info.bytecode_size = spv_size;
    spirv_info.entrypoint = "main";
    spirv_info.shader_stage = SDL_SHADERCROSS_SHADERSTAGE_VERTEX;
    spirv_info.props = 0;

    vs.shader_data.shader = SDL_ShaderCross_CompileGraphicsShaderFromSPIRV(
        dev, &spirv_info, &metadata->resource_info, 0);

    SDL_free(metadata);
    SDL_free(spv_code);

    if (!vs.shader_data.shader) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to create GPU vertex shader: %s", hlsl_path);
    }
    return vs;
}

FragmentShaderData ShaderManager::CreateFragmentShader(const char* hlsl_path) {
    size_t spv_size = 0;
    Uint8* spv_code = LoadOrCompileSPIRV(hlsl_path, SDL_SHADERCROSS_SHADERSTAGE_FRAGMENT, spv_size);
    if (!spv_code) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to load/compile fragment shader: %s", hlsl_path);
        return {};
    }

    FragmentShaderData fs{};
    SDL_ShaderCross_GraphicsShaderMetadata* metadata =
        SDL_ShaderCross_ReflectGraphicsSPIRV(spv_code, spv_size, 0);

    if (!metadata) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to reflect fragment shader: %s", hlsl_path);
        SDL_free(spv_code);
        return {};
    }

    // Создаём GPU-шейдер
    SDL_ShaderCross_SPIRV_Info spirv_info{};
    spirv_info.bytecode = spv_code;
    spirv_info.bytecode_size = spv_size;
    spirv_info.entrypoint = "main";
    spirv_info.shader_stage = SDL_SHADERCROSS_SHADERSTAGE_FRAGMENT;
    spirv_info.props = 0;

    fs.shader_data.shader = SDL_ShaderCross_CompileGraphicsShaderFromSPIRV(
        dev, &spirv_info, &metadata->resource_info, 0);

    SDL_free(metadata);
    SDL_free(spv_code);

    if (!fs.shader_data.shader) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to create GPU fragment shader: %s", hlsl_path);
    }
    return fs;
}

ComputeShaderData ShaderManager::CreateComputeShader(const char* hlsl_path) {
    size_t spv_size = 0;
    Uint8* spv_code = LoadOrCompileSPIRV(hlsl_path, SDL_SHADERCROSS_SHADERSTAGE_COMPUTE, spv_size);
    if (!spv_code) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to load/compile compute shader: %s", hlsl_path);
        return {};
    }

    ComputeShaderData cs{};
    cs.spv_code = spv_code;
    cs.spv_size = spv_size;

    // Рефлекшн через SDL_ShaderCross (полностью заменяет старую ReadComputeMetadata + SPIRV-Reflect)
    SDL_ShaderCross_ComputePipelineMetadata* metadata =
        SDL_ShaderCross_ReflectComputeSPIRV(spv_code, spv_size, 0);

    if (metadata) {
        cs.threadcount_x = metadata->threadcount_x;
        cs.threadcount_y = metadata->threadcount_y;
        cs.threadcount_z = metadata->threadcount_z;
        cs.num_samplers = metadata->num_samplers;
        cs.num_readonly_storage_textures = metadata->num_readonly_storage_textures;
        cs.num_readonly_storage_buffers = metadata->num_readonly_storage_buffers;
        cs.num_readwrite_storage_textures = metadata->num_readwrite_storage_textures;
        cs.num_readwrite_storage_buffers = metadata->num_readwrite_storage_buffers;
        cs.num_uniform_buffers = metadata->num_uniform_buffers;
        SDL_free(metadata);
    }
    else {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to reflect compute shader: %s", hlsl_path);
    }

    return cs;
}