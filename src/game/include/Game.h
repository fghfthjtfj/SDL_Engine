#pragma once
#include <SDL3/SDL.h>
#include "Engine.h"

enum class GameState {
	MAIN_MENU,
	GAMEPLAY,
	EDITOR,
	SETTINGS
};

struct MainMenuResources {
};

class Game {
public:
	Game(Engine* engine);
	SDL_AppResult MainInit();
	SDL_AppResult MainIterate();
	SDL_AppResult SDL_AppEvent(SDL_Event* event);
	void SDL_AppQuit();

	bool lmb_down = false;
	bool rmb_down = false;
private:
	Engine* engine = nullptr;
	
	BufferManager* bufferManager;
	TextureManager* textureManager;
	ShaderManager* shaderManager;
	ModelManager* modelManager;
	PassManager* passManager;
	ObjectManager* objectManager;
	CameraManager* cameraManager;
	MaterialManager* materialManager;
	BatchBuilder* batchBuilder;

	ThreadController* threadController;

	LightDataModule* light_data_module;

	void ChangeState(GameState newState);
	void MainMenu_Init();
	void MainMenu_Iterate();
	void MainMenu_Update();
	void MainMenu_Event(SDL_Event* event);
	void MainMenu_Quit();

	float width;
	float height;
	float mouse_x = 0;
	float mouse_y = 0;
	GameState game_state;
	MainMenuResources main_menu_resources;
};