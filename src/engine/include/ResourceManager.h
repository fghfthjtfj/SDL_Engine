#pragma once
#include <unordered_map>
#include <SDL3/SDL.h>
#include <SDL3/SDL_gpu.h>
#include <string_view>
#include <iostream>
#include <vector>
#include "Utils.h"
#include "config.h"
static constexpr uint32_t BASE_TB_SIZE = 10 * 1024 * 1024;
//static constexpr Uint32 BASE_TB_SIZE = 1 * 1 * 1;

class ResourceManager
{
public:
	void MapUploadTransferBuffer();
	void MapReadTransferBuffer();

	void UnmapUploadTransferBuffer(); 
	void UnmapReadTransferBuffer();

	void MapPrepassDependedTransferBuffer();
	void UnmapPrepassDependedTransferBuffer();

	
protected:
	ResourceManager(SDL_GPUDevice* device);
	void EnsureUploadTransferBufferCapacity(uint32_t size);
	void EnsureReadTransferBufferCapacity(uint32_t size);
	void EnsurePrepassDependedTransferBufferCapacity(uint32_t size);

	SDL_GPUDevice* dev = nullptr;
	SDL_GPUTransferBuffer* upload_transfer_buffer;
	SDL_GPUTransferBuffer* read_transfer_buffer;
	SDL_GPUTransferBuffer* prepass_dep_transfer_buffer;

	uint32_t current_upload_tb_size = BASE_TB_SIZE;
	uint32_t current_upload_tb_offset = 0;

	uint32_t current_read_tb_size = BASE_TB_SIZE;
	uint32_t current_read_tb_offset = 0; 

	uint32_t current_prepass_dep_tb_size = BASE_TB_SIZE;
	uint32_t current_prepass_dep_tb_offset = 0;

	void* mapped_upload_tb = nullptr;
	void* mapped_read_tb = nullptr;
	void* mapped_prepass_dep_tb = nullptr;

	virtual ~ResourceManager();
private:
	SDL_GPUTransferBuffer* CreateUploadTransferBuffer(uint32_t size);
	SDL_GPUTransferBuffer* CreateReadTransferBuffer(uint32_t size);
	SDL_GPUTransferBuffer* CreatePrepassDependedTransferBuffer(uint32_t size);
};
