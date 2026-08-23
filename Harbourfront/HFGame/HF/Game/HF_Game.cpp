#include "PCH/HFGame_PCH.h"
#include "HF_Game.h"

#include "HF/ECS/HF_Temp_PlayerComponent.h"

#include "EN/ECS/EN_TEST_BobbingComponent.h"
#include "EN/Libraries/HIDra/HIDra.h"
#include "EN/YakuEngine.h"

#include "CG/Camera/CG_CameraComponent.h"
#include "CG/Renderable/CG_Renderable.h"
#include "CG/Resource/Material/CG_Material.h"
#include "CG/Resource/Mesh/CG_Mesh.h"
#include "CG/Resource/Mesh/CG_MeshFactory.h"
#include "CG/Resource/Shader/CG_Shader.h"
#include "CG/Resource/Texture/CG_TextureFactory.h"

#include "YK/Core/YK_Core.h"
#include "YK/ECS/Components/YK_TransformComponent.h"
#include "YK/IO/File/YK_FilePath.h"
#include "YK/Math/YK_MatrixMath.h"

#include <cstdlib>
#include <ctime>

// Temp
Zen::Entity g_cameraEntity;

CG_Mesh* g_quadMesh;

bool HF_Game::Init(YK_Core& p_engine)
{
    // Temp
    YK_AssetManager& assetManager = p_engine.GetAssetManager();
    Zen::Garden& entityGarden = p_engine.GetZenGarden();

    // Add Camera
    g_cameraEntity = entityGarden.Spawn<YK_TransformComponent, CG_CameraComponent>();
    g_cameraEntity.GetComponent<YK_TransformComponent>()->m_position.y = 1.0f;
    CG_CameraComponent* camera = g_cameraEntity.GetComponent<CG_CameraComponent>();
    camera->m_fov = 60.0f;
    camera->m_nearPlane = 0.1f;
    camera->m_farPlane = 100.0f;

    // Gather Assets for Spawning
    CG_Mesh const& heartMesh = assetManager.GetAsset<CG_Mesh>(YK_FilePath("Models/HeartTest.obj"));
    g_quadMesh = CG_MeshFactory::Quad();

    CG_Material const& heartMaterial = assetManager.GetAsset<CG_Material>(YK_FilePath("Materials/Main.YKM"));
    CG_Material const& groundMaterial = assetManager.GetAsset<CG_Material>(YK_FilePath("Materials/Ground.YKM"));

    std::srand(static_cast<unsigned int>(time(NULL)));
    auto GetRandomFloat = [](float p_max) {
        auto randomValue = std::rand() % 10000;
        return static_cast<float>(randomValue) / 10000.0f * p_max;
    };

    Zen::Entity groundPlane = entityGarden.Spawn<YK_TransformComponent, CG_MeshComponent, CG_RendererComponent>();
    YK_TransformComponent* groundTransform = groundPlane.GetComponent<YK_TransformComponent>();
    constexpr float angle = 90 * (3.14159265f / 180.0f);
    groundTransform->m_orientation = YK_Quaternion(YK_Vector3f::Right(), angle);
    groundTransform->m_scale = YK_Vector3f(30.0f);

    groundPlane.GetComponent<CG_MeshComponent>()->m_mesh = g_quadMesh;
    groundPlane.GetComponent<CG_RendererComponent>()->m_material = &groundMaterial;

    for (auto i : Zen::LoopUtils::CountTo(5))
    {
        YK_Unused(i);

        Zen::Entity bobber =
          entityGarden.Spawn<YK_TransformComponent, CG_MeshComponent, CG_RendererComponent, BobbingComponent>();
        YK_TransformComponent* bobberTransform = bobber.GetComponent<YK_TransformComponent>();

        float x = GetRandomFloat(10.0f) - 5.0f;
        float y = GetRandomFloat(10.0f) - 5.0f;
        float z = GetRandomFloat(10.0f) - 5.0f;
        bobberTransform->m_position = YK_Vector3f(x, y, z);

        float bobOffset = GetRandomFloat(10.0f);
        bobber.GetComponent<BobbingComponent>()->m_phase = bobOffset;

        bobber.GetComponent<CG_MeshComponent>()->m_mesh = &heartMesh;
        bobber.GetComponent<CG_RendererComponent>()->m_material = &heartMaterial;
    }

    Zen::Entity player =
      entityGarden.Spawn<YK_TransformComponent, CG_MeshComponent, CG_RendererComponent, HF_PlayerComponent>();

    player.GetComponent<CG_MeshComponent>()->m_mesh = &heartMesh;
    player.GetComponent<CG_RendererComponent>()->m_material = &heartMaterial;
    return true;
}

void HF_Game::Update(YK_Core& /*p_engine*/)
{
    HIDra::Vec2f input /*= HIDra::GetAxis2D(HIDra::GamepadAxisID::AID_STICK_L)*/ = { 0.0f, 0.0f };
    if (HIDra::GetKey(HIDra::KEYCODE_S) || HIDra::GetButton(HIDra::BID_DPAD_SOUTH))
    {
        input.m_y = -1;
    }
    else if (HIDra::GetKey(HIDra::KEYCODE_W) || HIDra::GetButton(HIDra::BID_DPAD_NORTH))
    {
        input.m_y = 1;
    }

    if (HIDra::GetKey(HIDra::KEYCODE_A) || HIDra::GetButton(HIDra::BID_DPAD_WEST))
    {
        input.m_x = -1;
    }
    else if (HIDra::GetKey(HIDra::KEYCODE_D) || HIDra::GetButton(HIDra::BID_DPAD_EAST))
    {
        input.m_x = 1;
    }

    float raise = HIDra::GetButton(HIDra::BID_BUMPER_L) ? -1.0f : HIDra::GetButton(HIDra::BID_BUMPER_R) ? 1.0f : 0.0f;

    YK_TransformComponent* cameraTransform = g_cameraEntity.GetComponent<YK_TransformComponent>();

    // TODO: Converter function
    YK_Vector3f const flatForward = [&]() {
        YK_Vector3f result = cameraTransform->m_orientation * YK_Vector3f::Forward();
        result.y = 0.0f;
        return YK_Vector::GetNormalized(result);
    }();

    YK_Vector3f const right = YK_Vector::Cross(YK_Vector3f::Up(), flatForward);
    YK_Vector3f frameDelta = (right * input.m_x) + (flatForward * -input.m_y) + (YK_Vector3f(0.0f, raise, 0.0f));

    frameDelta *= YK_Time::DeltaTime() * 2.0f;
    cameraTransform->m_position += frameDelta;

    HIDra::Vec2f leftRight = HIDra::GetAxis2D(HIDra::GamepadAxisID::AID_STICK_R);
    YK_Vector3f newLookTarget = -flatForward + cameraTransform->m_position;
    newLookTarget += (right * leftRight.m_x) * YK_Time::DeltaTime();
    YK_Quaternion newOrientation = YK_Matrix::LookAt(cameraTransform->m_position, newLookTarget);

    cameraTransform->m_orientation = newOrientation;
}

void HF_Game::ShutDown(YK_Core& /*p_engine*/) { delete g_quadMesh; }