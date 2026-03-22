#pragma once
#include <SDL3/SDL.h>
#include "ResourceManager.h"
#include "TextureData.h"

struct UploadTaskTexture {
	SDL_GPUTextureRegion dst{};
	const char* path;
	TextureHandle* target_handle;
	Uint32 offset;                          
	Uint32 size;
	Uint32 width, height, pitch;

};

inline constexpr const char* DEFAULT_SAMPLER = "_DefaultSampler";
inline constexpr const char* DEFAULT_SHADOW_SAMPLER = "_DefaultShadowSampler";


class TextureManager:public ResourceManager
{
public:
	TextureManager(SDL_GPUDevice* device);

	TextureAtlas* CreateTextureAtlas(const std::string& name, SDL_GPUTextureCreateInfo tci, SDL_GPUSampler* sampler);
	// Создание TextureAtlas из уже существующим TextureAtlas
// Create TextureAtlas from an already existing TextureAtlas
	TextureAtlas* CreateTextureAtlas(const std::string& name, TextureAtlas* existing_atlas, SDL_GPUSampler* sampler);
	// Создание TextureData из файла
	// Create TextureData from a file
	TextureHandle* CreateTextureFromFile(const std::string& name, const std::string& atlas_name, const char* path);
	TextureHandle* CreateTextureFromFile(const std::string& name, TextureAtlas* atlas, const char* path);

	//// Создание пустой TextureData с заданными параметрами, без загрузки данных в текстуру
	//// Create an empty TextureData with specified parameters, without uploading data to the texture
	//TextureData* CreateTextureData(const std::string& name, SDL_GPUTextureCreateInfo tci, SDL_GPUSampler* sampler);

	//// Создание TextureData из уже существующим SDL_GPUTexture
	//// Create TextureData from an already existing SDL_GPUTexture
	//TextureData* CreateTextureData(const std::string& name, SDL_GPUTexture* texture, SDL_GPUSampler* sampler);

	// Создание пустой GPU текстуры
	SDL_GPUTexture* CreateGPU_Texture(SDL_GPUTextureCreateInfo tci);

	void GenerateMipmaps(SDL_GPUCommandBuffer* cb);

	void ExecuteUploadTasks(SDL_GPUCopyPass* cp);
	SDL_GPUSampler* CreateSampler(const std::string& name, SDL_GPUSamplerCreateInfo sci);
	SDL_GPUSampler* GetSampler(const std::string& name);
	
	void DeleteTexture(const std::string& name);
	void DeleteTexture(SDL_GPUTexture* texture);
	~TextureManager();

	SDL_GPUTexture* main_pass_depth_texture = nullptr;

public:
	/*TextureData* GetTextureData(const std::string& name) {
		auto it = textures_data.find(name);
		if (it != textures_data.end()) {
			return it->second.get();
		}
		else {
			SDL_Log("Texture '%s' not found", name.c_str());
			return nullptr;
		}
	};*/
	TextureHandle* GetTextureHandle(const std::string& name) {
		auto it = handles_data.find(name);
		if (it != handles_data.end()) {
			return it->second.get();
		}
		else {
			SDL_Log("Texture '%s' not found", name.c_str());
			return nullptr;
		}
	};
	TextureAtlas* GetTextureAtlas(const std::string& name) {
		auto it = atlases_data.find(name);
		if (it != atlases_data.end()) {
			return it->second.get();
		}
		else {
			SDL_Log("Texture atlas '%s' not found", name.c_str());
			return nullptr;
		}
	};
private:
	void CreateUploadTask(TextureHandle* handle, int w, int h, const char* path);
	void _BuildUploadTasks();
	std::unordered_map<std::string, std::unique_ptr<TextureAtlas>> atlases_data;
	std::unordered_map<std::string, std::unique_ptr<TextureHandle>> handles_data;
	std::unordered_map<std::string, SDL_GPUSampler*> samplers_data;
	std::vector<UploadTaskTexture> upload_tasks;

};

