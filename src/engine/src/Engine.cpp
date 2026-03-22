#include "PCH.h"
#include "Engine.h"
#include "TexturesPresets.h"

//void Engine::Iterate()
//{
//	Uint64 now = SDL_GetTicks();
//	double elapsed = (double)(now - prevTicks);
//	if (elapsed < frameTime) {
//		SDL_Delay((Uint32)(frameTime - elapsed));
//		now = SDL_GetTicks();
//	}
//	prevTicks = now; 
//
//	SDL_GPUCommandBuffer* cb = SDL_AcquireGPUCommandBuffer(dev);
//
//	Uint32 w = 0, h = 0;
//	SDL_GPUTexture* tex = nullptr;
//
//	if (!SDL_AcquireGPUSwapchainTexture(cb, win, &tex, &w, &h)) {
//		SDL_Log("Acquire error: %s", SDL_GetError());
//		SDL_CancelGPUCommandBuffer(cb);
//		return;
//	}
//	if (!tex) {
//		SDL_CancelGPUCommandBuffer(cb);
//
//		return;
//	};
//
//	pass_manager->UpdateColorTargetInfoTex(tex);
//
//	buffer_manager->TrashBuffers();
//
//	SceneData* scene = object_manager->GetActiveScene();
//	buffer_manager->UpdateBuffer("cameraBuffer");
//	buffer_manager->UpdateBuffer("DefaultTransformBuffer", scene);
//	buffer_manager->UpdateBuffer("lightBuffer", &allLights);
//	buffer_manager->UpdateBuffer("DefaultPositionIndexBuffer");
//	model_manager->UploadModelBuffer(buffer_manager);
//	buffer_manager->_BuildUploadTasks();
//
//	SDL_GPUCopyPass* cp = SDL_BeginGPUCopyPass(cb);
//	ExecuteCP(cp);
//	SDL_EndGPUCopyPass(cp);
//
//	SDL_GPURenderPass* rp = SDL_BeginGPURenderPass(cb, &pass_manager->colorTargetInfo, 1, &pipe_manager->depthTargetInfo);
//
//	pass_manager->ExecutePassesSteps(rp, buffer_manager, 0);
//	SDL_EndGPURenderPass(rp);
//
//	SDL_SubmitGPUCommandBuffer(cb);
//}

//void Engine::PrepareFunc(uint8_t slot)
//{
//	slot_controller->SetSlotState(slot, PREPARING);
//	buffer_manager->logic_index = slot;
//
//	pipe_manager->CreateGraphicsPiplenes(shader_manager);
//	pipe_manager->CreateComputePipelines(shader_manager);
//
//	batch_builder->BuildRenderBatches(pipe_manager, pass_manager, object_manager, object_manager->GetActiveScene());
//	batch_builder->BuildComputeBatches(pipe_manager, shader_manager);
//	batch_builder->BuildComputePrepassBatches(pipe_manager, shader_manager);
//
//	SDL_GPUCommandBuffer* cb0 = SDL_AcquireGPUCommandBuffer(dev);
//	SDL_GPUCopyPass* cp0 = SDL_BeginGPUCopyPass(cb0);
//
//	buffer_manager->ExecutePrePassUpdateInstruction(cp0);
//	buffer_manager->ExecutePrePassUploadTasks(cp0, slot);
//
//	SDL_EndGPUCopyPass(cp0);
//
//	SDL_GPUFence* prepass_task_fence = SDL_SubmitGPUCommandBufferAndAcquireFence(cb0);
//	SDL_WaitForGPUFences(dev, true, &prepass_task_fence, 1);
//	SDL_ReleaseGPUFence(dev, prepass_task_fence);
//
//	SDL_GPUCommandBuffer* cb1 = SDL_AcquireGPUCommandBuffer(dev);
//	pass_manager->ExecutePrepassesSteps(cb1, slot);
//	SDL_GPUFence* prepass_fence = SDL_SubmitGPUCommandBufferAndAcquireFence(cb1);
//
//	SDL_WaitForGPUFences(dev, true, &prepass_fence, 1);
//	SDL_ReleaseGPUFence(dev, prepass_fence);
//
//	buffer_manager->MapUploadTransferBuffer();
//	buffer_manager->MapReadTransferBuffer();
//	texture_manager->MapUploadTransferBuffer();
//
//	// Полностью CPU логика
//	SDL_GPUCommandBuffer* cb2 = SDL_AcquireGPUCommandBuffer(dev);
//	SDL_GPUCopyPass* cp2 = SDL_BeginGPUCopyPass(cb2);
//
//	buffer_manager->ExecuteReadBackInstructionsSize();
//
//	buffer_manager->ExecuteUpdateInstructions(cp2);
//	// Не неачинает загрузку до submit, всё ещё CPU
//	buffer_manager->ExecuteUploadTasks(cp2, slot);
//	texture_manager->ExecuteUploadTasks(cp2);
//	buffer_manager->ExecuteDownloadTasks(cp2, slot);
//
//	//SDL_WaitForGPUFences(dev, true, &prepass_fence, 1);
//	//SDL_ReleaseGPUFence(dev, prepass_fence);
//
//	SDL_EndGPUCopyPass(cp2);
//	SDL_GPUFence* upload_download_fence = SDL_SubmitGPUCommandBufferAndAcquireFence(cb2);
//
//	buffer_manager->UnmapUploadTransferBuffer(); // Загрузок в обычном tb большей нет, анмапим
//	// buffer_manager->MapPostReadBackUploadTB() // Появится в будущем для post read back UI
//
//	SDL_WaitForGPUFences(dev, true, &upload_download_fence, 1);
//	SDL_ReleaseGPUFence(dev, upload_download_fence); // Ждём и завершения скачивания.
//
//
//	SDL_GPUCommandBuffer* cb3 = SDL_AcquireGPUCommandBuffer(dev);
//	SDL_GPUCopyPass* cp3 = SDL_BeginGPUCopyPass(cb3);
//
//	buffer_manager->ExecuteReadBackInstructionsReader();
//	// Потом появится тип задача со своим tb, которые для заливки данных в буфер тебуют завершения rb.
//	// buffer_manager->ExecutePostReadBackUpdateInstructions(cp3); // НУЖЕН CP3!
//	SDL_EndGPUCopyPass(cp3);
//
//	buffer_manager->UnmapReadTransferBuffer();
//	// buffer_manager->UnmapPostReadTransferBuffer();
//	texture_manager->UnmapUploadTransferBuffer();
//
//	texture_manager->GenerateMipmaps(cb3);
//
//	SDL_GPUFence* mip_fence = SDL_SubmitGPUCommandBufferAndAcquireFence(cb3);
//	SDL_WaitForGPUFences(dev, true, &mip_fence, 1);
//	SDL_ReleaseGPUFence(dev, mip_fence);
//
//	slot_controller->SetSlotState(slot, PREPARED);
//	slot_controller->SetSlotState(slot, UPLOADING);
//	slot_controller->SetSlotState(slot, UPLOADED);
//}
void Engine::PrepareFunc(uint8_t slot) {
	slot_controller->SetSlotState(slot, PREPARING);
	buffer_manager->logic_index = slot;

	pipe_manager->CreateGraphicsPiplenes(shader_manager);
	pipe_manager->CreateComputePipelines(shader_manager);

	batch_builder->BuildRenderBatches(pipe_manager, pass_manager, object_manager, object_manager->GetActiveScene());
	batch_builder->BuildComputeBatches(pipe_manager, shader_manager);
	batch_builder->BuildComputePrepassBatches(pipe_manager, shader_manager);

	// Запустить 2 потоках
	PrepareFuncPrepassUndepended(slot);
	PrepareFuncPrepassDepended(slot);

	slot_controller->SetSlotState(slot, PREPARED);
	slot_controller->SetSlotState(slot, UPLOADING);
	slot_controller->SetSlotState(slot, UPLOADED);
}


void Engine::PrepareFuncPrepassUndepended(uint8_t slot)
{
	buffer_manager->MapUploadTransferBuffer();
	texture_manager->MapUploadTransferBuffer();

	SDL_GPUCommandBuffer* cb2 = SDL_AcquireGPUCommandBuffer(dev);
	SDL_GPUCopyPass* cp2 = SDL_BeginGPUCopyPass(cb2);

	buffer_manager->ExecuteUpdateInstructions(cp2);
	buffer_manager->ExecuteUploadTasks(cp2, slot);
	texture_manager->ExecuteUploadTasks(cp2);

	SDL_EndGPUCopyPass(cp2);

	texture_manager->GenerateMipmaps(cb2);

	SDL_GPUFence* upload_fence = SDL_SubmitGPUCommandBufferAndAcquireFence(cb2);



	SDL_WaitForGPUFences(dev, true, &upload_fence, 1);
	SDL_ReleaseGPUFence(dev, upload_fence); // Ждём и завершения скачивания.

	buffer_manager->UnmapUploadTransferBuffer(); // Загрузок в обычном tb большей нет, анмапим
	texture_manager->UnmapUploadTransferBuffer();
}

void Engine::PrepareFuncPrepassDepended(uint8_t slot)
{
	SDL_GPUCommandBuffer* cb0 = SDL_AcquireGPUCommandBuffer(dev);
	SDL_GPUCopyPass* cp0 = SDL_BeginGPUCopyPass(cb0);

	buffer_manager->ExecutePrePassUpdateInstruction(cp0);
	buffer_manager->ExecutePrePassUploadTasks(cp0, slot);

	SDL_EndGPUCopyPass(cp0);
	SDL_GPUFence* prepass_task_fence = SDL_SubmitGPUCommandBufferAndAcquireFence(cb0);
	SDL_WaitForGPUFences(dev, true, &prepass_task_fence, 1);
	SDL_ReleaseGPUFence(dev, prepass_task_fence);


	SDL_GPUCommandBuffer* cb1 = SDL_AcquireGPUCommandBuffer(dev);

	pass_manager->ExecutePrepassesSteps(cb1, slot);

	SDL_GPUFence* prepass_fence = SDL_SubmitGPUCommandBufferAndAcquireFence(cb1);
	SDL_WaitForGPUFences(dev, true, &prepass_fence, 1);
	SDL_ReleaseGPUFence(dev, prepass_fence);


	buffer_manager->MapReadTransferBuffer();
	SDL_GPUCommandBuffer* cb2 = SDL_AcquireGPUCommandBuffer(dev);
	SDL_GPUCopyPass* cp2 = SDL_BeginGPUCopyPass(cb2);

	buffer_manager->ExecuteReadBackInstructionsSize();
	buffer_manager->ExecuteDownloadTasks(cp2, slot);

	SDL_EndGPUCopyPass(cp2);
	SDL_GPUFence* download_fence = SDL_SubmitGPUCommandBufferAndAcquireFence(cb2);
	SDL_WaitForGPUFences(dev, true, &download_fence, 1);
	SDL_ReleaseGPUFence(dev, download_fence);


	buffer_manager->ExecuteReadBackInstructionsReader();
	buffer_manager->UnmapReadTransferBuffer();

	// buffer_manager->MapPostReadBackUploadTB() // Появится в будущем для post read back UI
	SDL_GPUCommandBuffer* cb3 = SDL_AcquireGPUCommandBuffer(dev);
	SDL_GPUCopyPass* cp3 = SDL_BeginGPUCopyPass(cb3);
	// buffer_manager->ExecutePostReadBackUpdateInstructions(cp3); // Появится потом!
	// buffer_manager->ExecutePostReadBackUploadTask;
	// buffer_manager->UnmapPostReadTransferBuffer();

	SDL_GPUFence* postreadbackUI_fence = SDL_SubmitGPUCommandBufferAndAcquireFence(cb3);
	SDL_WaitForGPUFences(dev, true, &postreadbackUI_fence, 1);
	SDL_ReleaseGPUFence(dev, postreadbackUI_fence);
}

void Engine::UploadFunc(uint8_t slot)
{
	//slot_controller->SetSlotState(slot, UPLOADING);

	//SDL_GPUCommandBuffer* cb = SDL_AcquireGPUCommandBuffer(dev);
	//SDL_GPUCopyPass* cp = SDL_BeginGPUCopyPass(cb);
	//buffer_manager->MapUploadTransferBuffer();
	//texture_manager->MapUploadTransferBuffer();
	//texture_manager->ExecuteUploadTasks(cp);
	//buffer_manager->UnmapUploadTransferBuffer();
	//texture_manager->UnmapUploadTransferBuffer();
	//SDL_EndGPUCopyPass(cp);
	//SDL_SubmitGPUCommandBuffer(cb);

	//slot_controller->SetSlotState(slot, UPLOADED);

}

bool Engine::RenderFunc(uint8_t slot)
{
	SDL_GPUCommandBuffer* cb = SDL_AcquireGPUCommandBuffer(dev);

	Uint32 w = 0, h = 0;
	SDL_GPUTexture* tex = nullptr;

	if (!SDL_AcquireGPUSwapchainTexture(cb, win, &tex, &w, &h)) {
		SDL_CancelGPUCommandBuffer(cb);
		return false;
	}
	if (!tex) {
		SDL_CancelGPUCommandBuffer(cb);
		return false;
	}

	pass_manager->GetRenderPassStep(MAIN_PASS)->renderPassTexsData.SetColorTexture(tex);
	pass_manager->SetRenderFrame(slot);
	pass_manager->ExecutePassesSteps(cb, slot);

	// === ImGui — только если данные готовы ===
	if (imgui_draw_data && imgui_draw_data->CmdListsCount > 0)
	{
		ImGui_ImplSDLGPU3_PrepareDrawData(imgui_draw_data, cb);

		SDL_GPUColorTargetInfo imgui_color_target = {};
		imgui_color_target.texture = tex;
		imgui_color_target.load_op = SDL_GPU_LOADOP_LOAD;
		imgui_color_target.store_op = SDL_GPU_STOREOP_STORE;
		imgui_color_target.cycle = false;

		SDL_GPURenderPass* imgui_rp = SDL_BeginGPURenderPass(cb, &imgui_color_target, 1, nullptr);
		ImGui_ImplSDLGPU3_RenderDrawData(imgui_draw_data, cb, imgui_rp);
		SDL_EndGPURenderPass(imgui_rp);
	}

	SDL_GPUFence* fence = SDL_SubmitGPUCommandBufferAndAcquireFence(cb);
	slot_controller->SetSlotFence(slot, fence);
	slot_controller->SetSlotState(slot, RENDERING);

	return true;
}

void Engine::FenceFunc(uint8_t slot) {
	SlotData* slots = slot_controller->GetSlotsData();
	SlotData& sd = slots[slot];
	SDL_GPUFence* fence = sd.fence;

	if (!fence) return;

	if (!SDL_QueryGPUFence(dev, fence))
		return;

	SDL_ReleaseGPUFence(dev, fence);
	sd.fence = nullptr;
	buffer_manager->TrashBuffers();

	//slot_controller->RemoveSlotFence(slot);
	slot_controller->SetSlotState(slot, RENDERED);

}

void Engine::BeginImGuiFrame()
{
	ImGui_ImplSDLGPU3_NewFrame();
	ImGui_ImplSDL3_NewFrame();
	ImGui::NewFrame();
}

void Engine::EndImGuiFrame()
{
	ImGui::Render();
	imgui_draw_data = ImGui::GetDrawData();
}

void Engine::OnWindowResized(Sint32 w, Sint32 h)
{
	width = safe_sint32_f(w);
	height = safe_sint32_f(h);
	auto tci = TexturePresets::GetCreateInfo(TexturePreset::SingleDepth2048);
	tci.width = safe_f_u32(width);
	tci.height = safe_f_u32(height);

	SDL_GPUTexture* new_depth_texture = texture_manager->CreateGPU_Texture(tci);

	//texture_manager->DeleteTexture(texture_manager->main_pass_depth_texture);
	texture_manager->main_pass_depth_texture = new_depth_texture;

	pass_manager->GetRenderPassStep(MAIN_PASS)->renderPassTexsData.SetDepthTexture(texture_manager->main_pass_depth_texture);
}

Engine::Engine(SDL_Window* window, SDL_GPUDevice* dev, float width, float height)
{
	this->win = window;
	this->dev = dev;
	this->width = width;
	this->height = height;
	buffer_manager = new BufferManager(dev);
	texture_manager = new TextureManager(dev);
	shader_manager = new ShaderManager(dev);
	pipe_manager = new PipeManager(dev, win);
	model_manager = new ModelManager();
	pass_manager = new PassManager();
	object_manager = new ObjectManager();
	camera_manager = new CameraManager();
	slot_controller = new SlotController();
	thread_controller = new ThreadController(slot_controller);
	material_manager = new MaterialManager();

	batch_builder = new BatchBuilder();

	pib_data_module = new PIB_DataModule();
	transform_data_module = new TransformDataModule();
	light_data_module = new LightDataModule();
	indirect_data_module = new InderectDataModule();
	bound_sphere_data_module = new BoundSphereDataModule();
	count_data_module = new CountBufferDataModule();

	using namespace DefaultUpdateSet;
	SetDefaultCountBufferUpdater(buffer_manager, object_manager, count_data_module, light_data_module, batch_builder);

	SetDefaultCameraUpdater(buffer_manager, camera_manager);
	SetDefaultPositionUpdater(buffer_manager, object_manager, transform_data_module);
	SetDefaultLightUpdater(buffer_manager, object_manager, camera_manager, light_data_module);
	SetDefaultPositionIndexUpdater(buffer_manager, pass_manager, object_manager, pib_data_module, batch_builder);
	SetDefaultVertexUpdater(buffer_manager, model_manager);
	SetDefaultIndexUpdater(buffer_manager, model_manager);
	SetDefaultLightCamerasUpdater(buffer_manager, object_manager, light_data_module);
	SetDefaultIndirectUpdater(buffer_manager, pass_manager, indirect_data_module);
	SetDefaultBoundSphereUpdater(buffer_manager, pass_manager, model_manager, bound_sphere_data_module);

	SetDefaultCountReader(buffer_manager, transform_data_module);
	InitPasses();
	pass_manager->FillRenderPasses();

	thread_controller->SetPrepareCallback([this](uint8_t slot){this->PrepareFunc(slot);});
	thread_controller->SetUploadCallback([this](uint8_t slot) {this->UploadFunc(slot); });
	thread_controller->SetRenderCallback(
		[this](uint8_t slot) {
			return this->RenderFunc(slot);   // !!! return
		}
	);
	thread_controller->SetFenceCallback([this](uint8_t slot) {this->FenceFunc(slot); });

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

	ImGui_ImplSDL3_InitForSDLGPU(window);

	ImGui_ImplSDLGPU3_InitInfo init_info = {};
	init_info.Device = dev;
	init_info.ColorTargetFormat = SDL_GetGPUSwapchainTextureFormat(dev, window);
	init_info.MSAASamples = SDL_GPU_SAMPLECOUNT_1;
	ImGui_ImplSDLGPU3_Init(&init_info);
}

void Engine::InitPasses()
{
	using namespace DefaultRenderPassSet;
	SetDefaultCullingComputeZerosPass(pass_manager, buffer_manager);
	SetDefaultCullingComputeCountPass(pass_manager, buffer_manager, object_manager, transform_data_module, light_data_module, indirect_data_module);

	SetDefaultShadowRenderPass(pass_manager, texture_manager, buffer_manager, object_manager);
	SetDefaultMainRenderPass(pass_manager, texture_manager, buffer_manager);
}


Engine::~Engine()
{
	ImGui_ImplSDLGPU3_Shutdown();
	ImGui_ImplSDL3_Shutdown();
	ImGui::DestroyContext();

	delete buffer_manager;
	delete texture_manager;
	delete shader_manager;
	delete pipe_manager;
	delete model_manager;
	delete pass_manager;
	delete object_manager;
	delete camera_manager;
	delete slot_controller;
	delete thread_controller;
	delete material_manager;
	delete pib_data_module;
	delete transform_data_module;
	delete light_data_module;

	dev = nullptr;
	win = nullptr;
}
