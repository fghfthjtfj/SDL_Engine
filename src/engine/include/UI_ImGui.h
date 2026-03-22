#pragma once
#include "imgui.h"
#include "ObjectManager.h"
#include "CameraManager.h"

class UI_ImGui
{
public:
    static void Iterate(ObjectManager* objectManager, CameraManager* cameraManager);

private:
    static void DrawCameraPanel(CameraManager* cameraManager);
    static void DrawObjectsPanel(ObjectManager* objectManager);
};