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
    ImGui::Separator();
    DrawLightsPanel(objectManager);
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

void UI_ImGui::DrawLightsPanel(ObjectManager* objectManager)
{
    if (!ImGui::CollapsingHeader("Lights")) return;
    SceneData* scene = objectManager->GetActiveScene();

    // ---------- Spot Lights ----------
    if (ImGui::TreeNode("Spot Lights"))
    {
        int index = 0;
        objectManager->ForEach<Positions, SpotLightComponent>(scene,
            [&index](SoAElement<Positions> pos_el, SpotLightComponent& light)
        {
            Positions& P = pos_el.container();
            size_t i = pos_el.i();

            char label[32];
            snprintf(label, sizeof(label), "Spot %d", index++);
            if (!ImGui::TreeNode(label)) return;

            auto& d = light.light_data;
            bool changed = false;

            // --- Положение / поворот ---
            ImGui::SeparatorText("Transform");
            float pos[3] = { P.w[i], P.d[i], P.h[i] };
            if (ImGui::DragFloat3("Position", pos, 0.05f))
            {
                P.w[i] = pos[0]; P.d[i] = pos[1]; P.h[i] = pos[2];
            }
            float dir[3] = { d.dir_x, d.dir_y, d.dir_z };
            if (ImGui::DragFloat3("Direction", dir, 0.01f, -1.0f, 1.0f))
            {
                d.dir_x = dir[0]; d.dir_y = dir[1]; d.dir_z = dir[2];
                changed = true;
            }

            // --- Конус ---
            ImGui::SeparatorText("Cone");
            // source_angle в радианах -> SliderAngle показывает градусы.
            // Ограничиваем < 90°, иначе tan() уходит в бесконечность в ResolveDistance().
            changed |= ImGui::SliderAngle("Angle", &d.source_angle, 1.0f, 89.0f);
            // усечение конуса
            changed |= ImGui::DragFloat("Source Radius", &d.source_radius, 0.01f, 0.0f, FLT_MAX);

            // --- Цвет ---
            ImGui::SeparatorText("Color");
            changed |= ImGui::ColorEdit3("RGB", &d.r); // r,g,b лежат подряд в памяти

            // --- Затухание / мощность ---
            ImGui::SeparatorText("Falloff");
            changed |= ImGui::DragFloat("Power", &d.power, 0.05f, 0.0f, FLT_MAX);
            changed |= ImGui::DragFloat("Attenuation", &d.attenuation, 0.05f, 0.0f, FLT_MAX);

            d.ResolveDistance(); // лениво пересчитает max_distance, если что-то менялось
            ImGui::Text("Max Distance: %.3f", d.GetMaxDistance());

            if (changed) light.needsUpdate = true;
            ImGui::TreePop();
        });
        ImGui::TreePop();
    }

    // ---------- Sphere Lights ----------
    if (ImGui::TreeNode("Sphere Lights"))
    {
        int index = 0;
        objectManager->ForEach<Positions, SphereLightComponent>(scene,
            [&index](SoAElement<Positions> pos_el, SphereLightComponent& light)
        {
            Positions& P = pos_el.container();
            size_t i = pos_el.i();

            char label[32];
            snprintf(label, sizeof(label), "Sphere %d", index++);
            if (!ImGui::TreeNode(label)) return;

            auto& d = light.light_data;
            bool changed = false;

            // --- Положение ---
            ImGui::SeparatorText("Transform");
            float pos[3] = { P.w[i], P.d[i], P.h[i] };
            if (ImGui::DragFloat3("Position", pos, 0.05f))
            {
                P.w[i] = pos[0]; P.d[i] = pos[1]; P.h[i] = pos[2];
            }

            // --- Радиус ---
            ImGui::SeparatorText("Shape");
            changed |= ImGui::DragFloat("Radius", &d.source_radius, 0.01f, 0.0f, FLT_MAX);

            // --- Цвет ---
            ImGui::SeparatorText("Color");
            changed |= ImGui::ColorEdit3("RGB", &d.r);

            // --- Затухание / мощность ---
            ImGui::SeparatorText("Falloff");
            changed |= ImGui::DragFloat("Power", &d.power, 0.05f, 0.0f, FLT_MAX);
            changed |= ImGui::DragFloat("Attenuation", &d.attenuation, 0.05f, 0.0f, FLT_MAX);

            d.ResolveDistance();
            ImGui::Text("Max Distance: %.3f", d.GetMaxDistance());

            if (changed) light.needsUpdate = true;
            ImGui::TreePop();
        });
        ImGui::TreePop();
    }
}