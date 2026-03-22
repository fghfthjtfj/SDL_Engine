#pragma once
#include <vector>
#include <unordered_map>
#include <fstream>
#include <iostream>
#include <string>
#include <SDL3/SDL.h>
#include <SDL3/SDL_gpu.h>
#include "PositionStructure.h"
#include "ModelData.h"

class BufferManager;
struct UploadTask;

class ModelManager
{
public:
	ModelManager();
	ModelData* CreateModel(const std::string& name, const std::string& path, const std::string& path_ind);
	uint32_t CalculateModelsVerticesSize();
	uint32_t CalculateModelsIndicesSize();
	/*void UploadModelBuffer(BufferManager* bm, UploadTask* task);*/
	void UploadModelVertexBuffer(BufferManager* bm, UploadTask* task);
	void UploadModelIndexBuffer(BufferManager* bm, UploadTask* task);
	bool CheckDirty() const { return dirty; };
	bool CheckDirtySpheres() const { return dirty_spheres; };
	void CommitSpheres() { dirty_spheres = false; };
	ModelData* operator[](const std::string& name);
	~ModelManager();
	std::vector<PosUVNormal> vertices;
	std::vector<Uint32> indices;

private:
	std::unordered_map<std::string, std::unique_ptr<ModelData>> models_data;
	bool dirty = false;
	bool dirty_spheres = true;
	uint32_t total_vertices_size = 0;
	uint32_t total_indices_size = 0;
};