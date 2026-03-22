#include "PCH.h"
#include "UI_ImGui.h"

void UI_ImGui::Iterate(ObjectManager* objectManager, CameraManager* cameraManager)
{
    ImGui::Begin("Debug");
    ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
    ImGui::Separator();

    DrawCameraPanel(cameraManager);
    ImGui::Separator();
    DrawObjectsPanel(objectManager);

    ImGui::End();
}

void UI_ImGui::DrawCameraPanel(CameraManager* cameraManager)
{
    Camera* cam = cameraManager->GetActiveCamera();
    if (!cam) return;

    if (ImGui::CollapsingHeader("Camera"))
    {
        glm::vec3 pos = cam->GetPosition();
		glm::vec3 tgt = cam->GetTarget();
        if (ImGui::DragFloat3("Cam Position", &pos.x, 0.05f))
            cam->SetPosition(pos);
		if (ImGui::DragFloat3("Cam Target", &tgt.x, 0.05f))
			cam->SetView(pos, tgt, glm::vec3(0.0f, 1.0f, 0.0f));
    }
}

void UI_ImGui::DrawObjectsPanel(ObjectManager* objectManager)
{
    if (!ImGui::CollapsingHeader("Objects")) return;

	SceneData* scene = objectManager->GetActiveScene();
    // Итерируемся по объектам со спотлайтами как у тебя в Update
    int index = 0;
    objectManager->ForEach<LocalOffsets, SpotLightComponent>(
       scene,
        [&index](SoAElement<LocalOffsets> pos_el, SpotLightComponent&)
    {
        LocalOffsets& P = pos_el.container();
        size_t i = pos_el.i();

        // Уникальный лейбл для каждого объекта
        char label[32];
        snprintf(label, sizeof(label), "SpotLight %d", index++);

        if (ImGui::TreeNode(label))
        {
            float pos[3] = { P.ox[i], P.oy[i], P.oz[i] };
            if (ImGui::DragFloat3("Offset", pos, 0.05f))
            {
                P.ox[i] = pos[0];
                P.oy[i] = pos[1];
                P.oz[i] = pos[2];
            }
            ImGui::TreePop();
        }
    });
    if (ImGui::TreeNode("Mesh Objects"))
    {
        int index = 0;
        objectManager->ForEach<Positions, MaterialComponent, ModelComponent>(scene,  // ← свой компонент
            [&index](SoAElement<Positions> pos_el, MaterialComponent&, ModelComponent&)
        {
            Positions& P = pos_el.container();
            size_t i = pos_el.i();

            char label[32];
            snprintf(label, sizeof(label), "Mesh %d", index++);

            if (ImGui::TreeNode(label))
            {
                float pos[3] = { P.w[i], P.d[i], P.h[i] };
                if (ImGui::DragFloat3("Offset", pos, 0.05f))
                {
                    P.w[i] = pos[0];
                    P.d[i] = pos[1];
                    P.h[i] = pos[2];
                }
                ImGui::TreePop();
            }
        });
        ImGui::TreePop();
    }
}