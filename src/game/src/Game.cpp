#include "PCH.h"
#include "Game.h"
#include "TexturesPresets.h"
#include "DefaultShaderSet.h"

Game::Game(Engine* engine)
{
    game_state = GameState::MAIN_MENU;
    this->engine = engine;
    this->game_state = GameState::MAIN_MENU;

	bufferManager = engine->GetBufferManager();
	textureManager = engine->GetTextureManager();
	shaderManager = engine->GetShaderManager();
	modelManager = engine->GetModelManager();
	passManager = engine->GetRenderManager();
	objectManager = engine->GetObjectManager();
	cameraManager = engine->GetCameraManager();
	materialManager = engine->GetMaterialManager();
    batchBuilder = engine->GetBatchBuilder();

	threadController = engine->GetThreadController();

	light_data_module = engine->GetLightDataModule();

	width = engine->GetWidth();
	height = engine->GetHeight();
}

SDL_AppResult Game::MainInit()
{
    objectManager->CreateScene("main_menu");
    Camera* camera = cameraManager->CreateCamera(width, height);
    cameraManager->SetActiveCamera(0);

    camera->SetView(
        glm::vec3(2.0f, 0.7f, 3.5f), // позиция камеры
        glm::vec3(0.43f, -0.4f, -0.8f), // точка взгляда
        glm::vec3(0.0f, 1.0f, 0.0f)  // вектор вверх
    );
    
	SDL_GPUSampler* sampler = textureManager->GetSampler(DefaultSamplersNames::DEFAULT_SAMPLER);
	using namespace TexturePresets;

	TextureAtlas* atlas = textureManager->CreateTextureAtlas("albedo_atlas", TexturePresets::GetCreateInfo(TexturePreset::Albedo_Atlas2048_1Layer), sampler);
    TextureAtlas* NAOPBR_atlas = textureManager->CreateTextureAtlas("NAOPBR_atlas", TexturePresets::GetCreateInfo(TexturePreset::NAOPBR_Atlas2048_1Layer), sampler);

    TextureHandle* texture_cube = textureManager->CreateTextureFromFile("albedo_cube", atlas, "textures/assets/cube_test.png");
	TextureHandle* texture_car_blue = textureManager->CreateTextureFromFile("albedo_blue", atlas, "textures/assets/car_snow_blue.png");
	TextureHandle* norm = textureManager->CreateTextureFromFile("new_car_normal", NAOPBR_atlas, "textures/assets/car_norm.png");

    TextureHandle* texture_car = textureManager->CreateTextureFromFile("new_car", atlas, "textures/assets/new_car.png");
    TextureHandle* ground = textureManager->CreateTextureFromFile("new_car_ground", atlas, "textures/assets/new_car_ground.png");
    TextureHandle* glass = textureManager->CreateTextureFromFile("new_car_glass", atlas, "textures/assets/new_car_glass.png");

    using namespace DefaultShaderProgramSet;
    SetMainShaderProgram(bufferManager, shaderManager, passManager);
    SetDefaultShadowShaderProgram(bufferManager, shaderManager, passManager);
    //SetShadowBlurPrograms(bufferManager, shaderManager, passManager, textureManager, objectManager, light_data_module);

    auto material_car = materialManager->CreateMaterial("car", {
        { TextureSlotRole::Albedo, texture_car },
        { TextureSlotRole::Normal, norm }
        }, { shaderManager->GetShaderProgram("sp"), shaderManager->GetShaderProgram("sp_shadow") });
    auto material_car2 = materialManager->CreateMaterial("car2", {
        { TextureSlotRole::Albedo, texture_car },
        { TextureSlotRole::Normal, norm }
        }, { shaderManager->GetShaderProgram("sp"), shaderManager->GetShaderProgram("sp_shadow") });
    auto material_ground = materialManager->CreateMaterial("glass", {
        { TextureSlotRole::Albedo, glass },
        { TextureSlotRole::Normal, norm }
        }, { shaderManager->GetShaderProgram("sp"), shaderManager->GetShaderProgram("sp_shadow")});
    auto material_glass = materialManager->CreateMaterial("ground", {
        { TextureSlotRole::Albedo, ground },
        { TextureSlotRole::Normal, norm }
        }, { shaderManager->GetShaderProgram("sp"), shaderManager->GetShaderProgram("sp_shadow")});
  //  auto material_blue = materialManager->CreateMaterial("textured_sphere_material_blue", {
  //      { TextureSlotRole::Albedo, texture_car_blue },
		//{ TextureSlotRole::Normal, norm }
		//}, { sp, sp_shadow });
    //modelManager->CreateModel("car", "models/cube_1_v.bin", "models/cube_1_i.bin");
    modelManager->CreateModel("car", "models/new_car_n_fixed.bin", "models/new_car_n_fixed_i.bin");

    objectManager->CreateEntity("main_menu",
        MaterialComponent{ {material_car, material_car2, material_ground, material_glass} },
        ModelComponent{ (*modelManager)["car"] },
        PositionProxy16{ 1,0,0,3,  0,1,0,0,  0,0,1,0,  0,0,0,1 },
        ShadowComponent{}
    );
    objectManager->CreateEntity("main_menu",
        MaterialComponent{ {material_car, material_car2, material_ground, material_glass} },
        ModelComponent{ (*modelManager)["car"] },
        PositionProxy16{
            -1, 0,  0, 0.5,     // X basis = (-1, 0, 0)
             0, 1,  0, 0.0,
             0, 0, -1, 0.0,     // Z basis = (0, 0, -1)
             0, 0,  0, 1.0
        }, ShadowComponent{}
    );
    objectManager->CreateEntity("main_menu",
        MaterialComponent{ {material_car, material_car2, material_ground, material_glass} },
        ModelComponent{ (*modelManager)["car"] },
        PositionProxy16{
            -1, 0,  0, 0.5f,     // X basis = (-1, 0, 0)
             0, 1,  0, 0.0f,
             0, 0, -1, 2.5f,     // Z basis = (0, 0, -1)
             0, 0,  0, 1.0f
        }, ShadowComponent{}
    );
    Entity parent_id = objectManager->CreateEntity("main_menu",
        MaterialComponent{ {material_car, material_car2, material_ground, material_glass} },
        ModelComponent{ (*modelManager)["car"] },
        PositionProxy16{ 1,0,0,3,  0,1,0,0,  0,0,1,2.5f,  0,0,0,1 },
        ShadowComponent{}
    );
    //objectManager->CreateEntity("main_menu",
    //    SpotLightComponent{ SpotLightComponent::SpotLightData{ 0, 1.0f, 0.0f, 0.0f, 0.18f, 1, 1, 1, 100 } },
    //    PositionProxy16{ 1,0,0,-2.5f,  0,1,0,0,  0,0, 1,1.25f,  0,0,0,1 },
    //    ShadowCasterComponent{}
    //);
    objectManager->CreateEntity("main_menu",
        SphereLightComponent{ SphereLightComponent::SphereLightData{ 0.0125f, 1.0f, 1.0f, 1.0f, 11.0f, 200.0f } },
        PositionProxy16{ 1,0,0, 0.0f,  0,1,0,0,  0,0, 1,1.25f,  0,0,0,1 },
        ShadowCasterComponent{}
    );


    ChangeState(GameState::MAIN_MENU);
    
    return SDL_APP_CONTINUE;
}

SDL_AppResult Game::MainIterate()
{
    engine->BeginImGuiFrame();
    UI_ImGui::Iterate(objectManager, cameraManager);
    engine->EndImGuiFrame();

    int mouse_buttons = SDL_GetMouseState(&mouse_x, &mouse_y);

    cameraManager->GetActiveCamera()->RotateView(mouse_x, mouse_y, lmb_down);
    this->MainMenu_Update();

    switch (game_state) {
    case GameState::MAIN_MENU:
        MainMenu_Iterate();
        break;
    }

    return SDL_APP_CONTINUE;
}

SDL_AppResult Game::SDL_AppEvent(SDL_Event* event)
{
    switch (event->type)
    {
    case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
        return SDL_APP_SUCCESS;
    case SDL_EVENT_WINDOW_RESIZED:
        engine->OnWindowResized(event->window.data1, event->window.data2);
        return SDL_APP_CONTINUE;
    default:
        switch (game_state) {
        case GameState::MAIN_MENU:
            MainMenu_Event(event);
            break;
        }
        return SDL_APP_CONTINUE;
    }
}

void Game::SDL_AppQuit()
{
    ChangeState(GameState::MAIN_MENU);
    MainMenu_Quit();
}

void Game::ChangeState(GameState newState)
{
    bool skipQuit = ((game_state == GameState::MAIN_MENU && newState == GameState::SETTINGS) or (newState == GameState::MAIN_MENU && game_state == GameState::SETTINGS));
    bool skipInit = skipQuit;

    if (!skipQuit && newState != game_state) {
        switch (game_state) {
        case GameState::MAIN_MENU:
            MainMenu_Quit();
            break;
        default:
            break;
        }
    }

    game_state = newState;

    if (!skipInit) {
        switch (game_state) {
        case GameState::MAIN_MENU:
            MainMenu_Init();
            break;
        default:
            break;
        }
    }
}

void Game::MainMenu_Init()
{
}

void Game::MainMenu_Iterate()
{
}

void Game::MainMenu_Update()
{
    Camera* camera = cameraManager->GetActiveCamera();
    const bool* keys = SDL_GetKeyboardState(nullptr);
    ImGuiIO& io = ImGui::GetIO();

    const float camSpeed = 0.05f;
    const float lightSpeed = 0.1f;

    if (!io.WantCaptureMouse) {
        // Камера
        if (keys[SDL_SCANCODE_LEFT])  camera->Move(glm::vec3(-camSpeed, 0.0f, 0.0f));
        if (keys[SDL_SCANCODE_RIGHT]) camera->Move(glm::vec3(camSpeed, 0.0f, 0.0f));
        if (keys[SDL_SCANCODE_UP])    camera->Move(glm::vec3(0.0f, 0.0f, camSpeed));
        if (keys[SDL_SCANCODE_DOWN])  camera->Move(glm::vec3(0.0f, 0.0f, -camSpeed));
        if (keys[SDL_SCANCODE_SPACE]) camera->Move(glm::vec3(0.0f, camSpeed, 0.0f));
        if (keys[SDL_SCANCODE_LSHIFT])camera->Move(glm::vec3(0.0f, -camSpeed, 0.0f));

        // Спотлайты
        if (keys[SDL_SCANCODE_A] || keys[SDL_SCANCODE_D] ||
            keys[SDL_SCANCODE_W] || keys[SDL_SCANCODE_S] ||
            keys[SDL_SCANCODE_E] || keys[SDL_SCANCODE_Q])
        {
            objectManager->ForEach<Positions, SpotLightComponent>(
                objectManager->GetActiveScene(),
                [keys, lightSpeed](SoAElement<Positions> pos_el, SpotLightComponent&)
            {
                Positions& P = pos_el.container();
                size_t i = pos_el.i();

                if (keys[SDL_SCANCODE_A]) P.w[i] -= lightSpeed;
                if (keys[SDL_SCANCODE_D]) P.w[i] += lightSpeed;
                if (keys[SDL_SCANCODE_W]) P.h[i] += lightSpeed;
                if (keys[SDL_SCANCODE_S]) P.h[i] -= lightSpeed;
                if (keys[SDL_SCANCODE_E]) P.d[i] += lightSpeed;
                if (keys[SDL_SCANCODE_Q]) P.d[i] -= lightSpeed;
            });

            objectManager->ForEach<Positions, SphereLightComponent>(
                objectManager->GetActiveScene(),
                [keys, lightSpeed](SoAElement<Positions> pos_el, SphereLightComponent&)
            {
                Positions& P = pos_el.container();
                size_t i = pos_el.i();

                if (keys[SDL_SCANCODE_A]) P.w[i] -= lightSpeed;
                if (keys[SDL_SCANCODE_D]) P.w[i] += lightSpeed;
                if (keys[SDL_SCANCODE_W]) P.h[i] += lightSpeed;
                if (keys[SDL_SCANCODE_S]) P.h[i] -= lightSpeed;
                if (keys[SDL_SCANCODE_E]) P.d[i] += lightSpeed;
                if (keys[SDL_SCANCODE_Q]) P.d[i] -= lightSpeed;
            });
        }
    }
}

void Game::MainMenu_Event(SDL_Event* event)
{
    Camera* camera = cameraManager->GetActiveCamera();
    ImGuiIO& io = ImGui::GetIO();

    if (!io.WantCaptureMouse) {
        switch (event->type)
        {
        case SDL_EVENT_MOUSE_BUTTON_DOWN:
            if (event->button.button == SDL_BUTTON_RIGHT) rmb_down = true;
            if (event->button.button == SDL_BUTTON_LEFT)  lmb_down = true;
            break;
        case SDL_EVENT_MOUSE_BUTTON_UP:
            if (event->button.button == SDL_BUTTON_RIGHT) rmb_down = false;
            if (event->button.button == SDL_BUTTON_LEFT)  lmb_down = false;
            break;
        case SDL_EVENT_MOUSE_WHEEL:
        {
            float dir = (event->wheel.direction == SDL_MOUSEWHEEL_FLIPPED) ? -1.0f : 1.0f;
            camera->SpeedChange(dir * event->wheel.y * 0.5f);
        }
        break;
        case SDL_EVENT_KEY_DOWN:
            if (event->key.scancode == SDL_SCANCODE_ESCAPE)
            {
                // Выход
            }
            break;
        default:
            break;
        }
    }
}


void Game::MainMenu_Quit()
{
    // Пока ничего
}
