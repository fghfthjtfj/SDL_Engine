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

	ctx = engine->GetEngineContext();
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

    TextureAtlas* atlas = ctx->CreateTextureAtlas("albedo_atlas", TexturePresets::GetCreateInfo(TexturePreset::Albedo_Atlas2048_1Layer), DefaultSamplersNames::DEFAULT_SAMPLER);
	TextureAtlas* NAOPBR_atlas = ctx->CreateTextureAtlas("NAOPBR_atlas", TexturePresets::GetCreateInfo(TexturePreset::NAOPBR_Atlas2048_1Layer), DefaultSamplersNames::DEFAULT_SAMPLER);

	TextureHandle* texture_cube = ctx->CreateTextureFromFile("albedo_cube", "albedo_atlas", "textures/assets/cube_test.png");
	TextureHandle* norm = ctx->CreateTextureFromFile("norm", "NAOPBR_atlas", "textures/assets/car_norm.png");

    TextureHandle* texture_car = ctx->CreateTextureFromFile("new_car", "albedo_atlas", "textures/assets/new_car.png");
	TextureHandle* ground = ctx->CreateTextureFromFile("new_car_ground", "albedo_atlas", "textures/assets/new_car_ground.png");
	TextureHandle* glass = ctx->CreateTextureFromFile("new_car_glass", "albedo_atlas", "textures/assets/new_car_glass.png");

    {
        using namespace DefaultShaderProgramSet;
        SetMainShaderProgram(ctx);
        SetDefaultShadowShaderProgram(ctx);
    }
    
    auto material_car = ctx->CreateMaterial("car", {
        {TextureSlotRole::Albedo, "new_car"},
        {TextureSlotRole::Normal, "norm"} },
        { "sp", "sp_shadow" });
	auto material_car2 = ctx->CreateMaterial("car2", {
        {TextureSlotRole::Albedo, "new_car"},
        {TextureSlotRole::Normal, "norm"} },
		{ "sp", "sp_shadow" });
    auto material_ground = ctx->CreateMaterial("ground", {
        {TextureSlotRole::Albedo, "new_car_ground"},
        {TextureSlotRole::Normal, "norm"} },
		{ "sp", "sp_shadow" });
    auto material_glass = ctx->CreateMaterial("glass", {
        {TextureSlotRole::Albedo, "new_car_glass"},
        {TextureSlotRole::Normal, "norm"} },
		{ "sp", "sp_shadow" });

    ModelData* model_car = ctx->CreateModel("car", "models/new_car_n_fixed.bin", "models/new_car_n_fixed_i.bin");

    ctx->CreateEntity("main_menu",
        MaterialComponent{ {material_car, material_car2, material_glass, material_ground} },
        ModelComponent{ model_car },
        PositionProxy16{ 1,0,0,3,  0,1,0,0,  0,0,1,0,  0,0,0,1 },
        ShadowComponent{}
    );
    ctx->CreateEntity("main_menu",
        MaterialComponent{ {material_car, material_car2, material_glass, material_ground} },
        ModelComponent{ model_car },
        PositionProxy16{
            -1, 0,  0, 0.5,     // X basis = (-1, 0, 0)
             0, 1,  0, 0.0,
             0, 0, -1, 0.0,     // Z basis = (0, 0, -1)
             0, 0,  0, 1.0
        }, ShadowComponent{}
    );
    ctx->CreateEntity("main_menu",
        MaterialComponent{ {material_car, material_car2, material_glass, material_ground} },
        ModelComponent{ model_car },
        PositionProxy16{
            -1, 0,  0, 0.5f,     // X basis = (-1, 0, 0)
             0, 1,  0, 0.0f,
             0, 0, -1, 2.5f,     // Z basis = (0, 0, -1)
             0, 0,  0, 1.0f
        }, ShadowComponent{}
    );
    Entity parent_id = ctx->CreateEntity("main_menu",
        MaterialComponent{ {material_car, material_car2, material_glass, material_ground} },
        ModelComponent{ model_car },
        PositionProxy16{ 1,0,0,3,  0,1,0,0,  0,0,1,2.5f,  0,0,0,1 },
        ShadowComponent{}
    );
    //ctx->CreateEntity("main_menu",
    //    SpotLightComponent{ SpotLightComponent::SpotLightData{ 0, 1.0f, 0.0f, 0.0f, 0.18f, 1, 1, 1, 100 } },
    //    PositionProxy16{ 1,0,0,-2.5f,  0,1,0,0,  0,0, 1,1.25f,  0,0,0,1 },
    //    ShadowCasterComponent{}
    //);
    ctx->CreateEntity("main_menu",
        SphereLightComponent{ SphereLightComponent::SphereLightData{ 0.0125f, 1.0f, 1.0f, 1.0f, 5.0f, 20.0f } },
        PositionProxy16{ 1,0,0, 0.0f,  0,1,0,0,  0,0, 1,1.25f,  0,0,0,1 },
        ShadowCasterComponent{}
    );


    ChangeState(GameState::MAIN_MENU);
    
    return SDL_APP_CONTINUE;
}

SDL_AppResult Game::MainIterate()
{
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
