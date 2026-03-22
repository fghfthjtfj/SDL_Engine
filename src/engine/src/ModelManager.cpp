#include "PCH.h"
#include "ModelManager.h"
#include "BufferManager.h"

ModelManager::ModelManager() {};

struct SubMeshFileEntry {
    uint32_t vertexOffset;
    uint32_t indexOffset;
    uint32_t vertexCount;
    uint32_t indexCount;
    uint32_t material_index;
};

ModelData* ModelManager::CreateModel(const std::string& name, const std::string& path_vert, const std::string& path_ind)
{
    auto it = models_data.find(name);
    if (it != models_data.end()) {
        SDL_Log("Model '%s' already exists, returning existing model data.", name.c_str());
        return it->second.get();
    }

    std::ifstream vf(path_vert, std::ios::binary);
    if (!vf) {
        SDL_Log("Failed to open vertex file: %s", path_vert.c_str());
        return nullptr;
    }

    // „итаем заголовок: количество сабмешей
    uint32_t submesh_count = 0;
    vf.read(reinterpret_cast<char*>(&submesh_count), sizeof(uint32_t));
    if (!vf || submesh_count == 0) {
        SDL_Log("Failed to read submesh count from: %s", path_vert.c_str());
        return nullptr;
    }

    // „итаем таблицу сабмешей
    std::vector<SubMeshFileEntry> entries(submesh_count);
    vf.read(reinterpret_cast<char*>(entries.data()), submesh_count * sizeof(SubMeshFileEntry));
    if (!vf) {
        SDL_Log("Failed to read submesh entries from: %s", path_vert.c_str());
        return nullptr;
    }

    // „итаем вершины (остаток файла)
    size_t header_size = sizeof(uint32_t) + submesh_count * sizeof(SubMeshFileEntry);
    vf.seekg(0, std::ios::end);
    size_t file_size = vf.tellg();
    size_t vdata_size = file_size - header_size;

    if (vdata_size == 0 || vdata_size % sizeof(PosUVNormal) != 0) {
        SDL_Log("Invalid vertex data size in: %s", path_vert.c_str());
        return nullptr;
    }

    vf.seekg(static_cast<std::streamoff>(header_size), std::ios::beg);
    size_t vcount = vdata_size / sizeof(PosUVNormal);
    std::vector<PosUVNormal> verts(vcount, PosUVNormal{});
    vf.read(reinterpret_cast<char*>(verts.data()), vdata_size);
    if (!vf) {
        SDL_Log("Warning: read incomplete vertex data (%zu / %zu bytes).", size_t(vf.gcount()), vdata_size);
    }

    // „итаем индексы
    std::ifstream indf(path_ind, std::ios::binary);
    if (!indf) {
        SDL_Log("Failed to open index file: %s", path_ind.c_str());
        return nullptr;
    }
    indf.seekg(0, std::ios::end);
    size_t isize = indf.tellg();
    indf.seekg(0, std::ios::beg);
    if (isize == 0 || isize % sizeof(uint32_t) != 0) {
        SDL_Log("Index file is empty or invalid: %s", path_ind.c_str());
        return nullptr;
    }

    size_t icount = isize / sizeof(uint32_t);
    std::vector<uint32_t> model_indices(icount, 0);
    indf.read(reinterpret_cast<char*>(model_indices.data()), isize);
    if (!indf) {
        SDL_Log("Warning: read incomplete index data (%zu / %zu bytes).", size_t(indf.gcount()), isize);
    }

    // √лобальные offsets до вставки
    uint32_t global_voffset = safe_u32(this->vertices.size());
    uint32_t global_ioffset = safe_u32(this->indices.size());

    auto model_data = std::make_unique<ModelData>();

    for (const auto& e : entries) {
        SubMeshData sub{};
        sub.vertexOffset = global_voffset + e.vertexOffset;
        sub.indexOffset = global_ioffset + e.indexOffset;
        sub.vertexCount = e.vertexCount;
        sub.indexCount = e.indexCount;
        sub.material_index = e.material_index;

        // e.vertexOffset Ч локальный offset внутри verts[]
        const uint32_t base = e.vertexOffset;
        const uint32_t vc = e.vertexCount;

        glm::vec3 center(0.0f);
        for (uint32_t i = 0; i < vc; ++i) {
            const auto& v = verts[base + i];
            center += glm::vec3(v.x, v.y, v.z);
        }
        center /= static_cast<float>(vc);

        float radius = 0.0f;
        for (uint32_t i = 0; i < vc; ++i) {
            const auto& v = verts[base + i];
            float d = glm::distance(center, glm::vec3(v.x, v.y, v.z));
            if (d > radius) radius = d;
        }

        sub.sphere = glm::vec4(center, radius);
        model_data->submeshes.push_back(sub);
    }

    this->vertices.insert(this->vertices.end(), verts.begin(), verts.end());
    this->indices.insert(this->indices.end(), model_indices.begin(), model_indices.end());

    ModelData* ptr = model_data.get();
    models_data[name] = std::move(model_data);
    dirty = true;
    dirty_spheres = true;
    return ptr;
}

uint32_t ModelManager::CalculateModelsVerticesSize()
{
	if (!dirty)
		return 0;
	total_vertices_size = safe_u32(vertices.size() * sizeof(PosUVNormal));
	return total_vertices_size;
}

uint32_t ModelManager::CalculateModelsIndicesSize()
{
	if (!dirty)
		return 0;
	total_indices_size = safe_u32(indices.size() * sizeof(Uint32));
	return total_indices_size;
}

void ModelManager::UploadModelVertexBuffer(BufferManager* bm, UploadTask* task)
{
	if (!dirty) return;
	bm->UploadToTransferBuffer(task, total_vertices_size, vertices.data());
}

void ModelManager::UploadModelIndexBuffer(BufferManager* bm, UploadTask* task)
{
	if (!dirty) return;
	bm->UploadToTransferBuffer(task, total_indices_size, indices.data());
	dirty = false;
}



//void ModelManager::UploadModelBuffer(BufferManager* bm, UploadTask* task) {
//	const Uint32 vbBytes = (Uint32)(vertices.size() * sizeof(PosUVNormal));
//	const Uint32 ibBytes = (Uint32)(indices.size() * sizeof(Uint32));
//
//	//bm->uploadToGPUBuffer("DefaultVertexBuffer", vertices.data(), vbBytes);
//	//bm->uploadToGPUBuffer("DefaultIndexBuffer", indices.data(), ibBytes);
//	bm->UploadToTransferBuffer(task, vbBytes, vertices.data());
//	bm->UploadToTransferBuffer(task, ibBytes, indices.data());
//	std::cout << "Uploaded model buffers: " << vbBytes << " bytes for vertices, " << ibBytes << " bytes for indices." << std::endl;
//}


ModelData* ModelManager::operator[](const std::string& name)
{
	auto it = models_data.find(name);
	if (it != models_data.end()) {
		return it->second.get();
	}
	SDL_Log("Model '%s' not found", name.c_str());
	return nullptr;
}

ModelManager::~ModelManager()
{
	models_data.clear();
	vertices.clear();
	indices.clear();
}
