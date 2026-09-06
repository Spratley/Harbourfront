#include "PCH/HFGame_PCH.h"
#include "HF_Game.h"

#include "YK/Core/YK_Core.h"
#include "YK/ECS/Components/YK_TransformComponent.h"
#include "YK/IO/Asset/YK_AssetManager.h"
#include "YK/IO/File/YK_FilePath.h"
#include "YK/Libraries/Zen/Entity/Zen_Entity.h"
#include "YK/Libraries/Zen/Zen_Garden.h"
#include "YK/Math/YK_MatrixMath.h"
#include "YK/Math/YK_NumericLimits.h"
#include "YK/Math/YK_VectorMath.h"
#include "YK/Time/YK_Time.h"
#include "YK/Types/Math/YK_Integer.h"
#include "YK/Types/Math/YK_Quaternion.h"
#include "YK/Types/Math/YK_Vector.h"

#include "PP/ECS/PP_RigidBodyComponent.h"

#include "CG/2D/Canvas/CG_Canvas.h"
#include "CG/CG_RenderModule.h"
#include "CG/Camera/CG_CameraComponent.h"
#include "CG/ECS/CG_Components.h"
#include "CG/Resource/Material/CG_Material.h"
#include "CG/Resource/Mesh/CG_Mesh.h"
#include "CG/Resource/Mesh/CG_MeshFactory.h"

#include "EN/Libraries/HIDra/HIDra.h"
#include "EN/Libraries/HIDra/HIDraEnums.h"
#include "EN/Libraries/HIDra/HIDraTypes.h"
#include "EN/Modules/EN_ModuleRegistry.h"
#include "EN/YakuEngine.h"

#include <Jolt/Jolt.h>
#include "PP/Libraries/Jolt/Physics/Collision/Shape/BoxShape.h"
#include "PP/Libraries/Jolt/Physics/Collision/Shape/SphereShape.h"

#include <cstdlib>
#include <ctime>
#include <utility>
#include <vector>

// Temp
Zen::Entity g_cameraEntity;

CG_Mesh g_quadMesh;

auto GetRandomFloat = [](float p_max) {
    auto randomValue = std::rand() % 10000;
    return static_cast<float>(randomValue) / 10000.0f * p_max;
};

std::vector<Zen::Entity> g_moverEntities;
void Temp_SpawnMover()
{
    YakuEngine& engine = YakuEngine::GetEngine();
    YK_AssetManager& assetManager = engine.GetAssetManager();
    Zen::Garden& entityGarden = engine.GetZenGarden();

    YK_TransformComponent transform{
        .m_position = YK_Vector3f{ GetRandomFloat(0.2f) - 0.1f, 3.0f, GetRandomFloat(0.2f) - 0.1f },
        .m_orientation = YK_Quaternion(),
        .m_scale = YK_Vector3f::One()
    };

    Zen::Entity mover = entityGarden.Spawn(
      std::move(transform),
      CG_MeshComponent{ .m_mesh = &assetManager.GetAsset<CG_Mesh>(YK_FilePath("Models/HeartTest.obj")) },
      CG_RendererComponent{ .m_material = &assetManager.GetAsset<CG_Material>(YK_FilePath("Materials/Main.YKM")) },
      PP_RigidBodyComponent{ new JPH::SphereShape(0.5f), transform, PP_BodyType::Dynamic });

    PP_RigidBodyComponent* rb = mover.GetComponent<PP_RigidBodyComponent>();
    rb->m_bodyOffset = YK_Vector3f(0.0f, 0.5f, 0.0f);

    g_moverEntities.push_back(mover);
}

void HF_Game::Init(YK_Core& p_engine)
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
    static_cast<YakuEngine&>(p_engine).GetModules().GetRenderModule().SetActiveCamera(*camera);

    // Gather Assets for Spawning
    CG_Mesh const& heartMesh = assetManager.GetAsset<CG_Mesh>(YK_FilePath("Models/HeartTest.obj"));
    g_quadMesh = CG_MeshFactory::Quad();

    CG_Material const& heartMaterial = assetManager.GetAsset<CG_Material>(YK_FilePath("Materials/Main.YKM"));
    CG_Material const& groundMaterial = assetManager.GetAsset<CG_Material>(YK_FilePath("Materials/Ground.YKM"));

    std::srand(static_cast<unsigned int>(time(NULL)));

    Zen::Entity groundPlane = entityGarden.Spawn<YK_TransformComponent, CG_MeshComponent, CG_RendererComponent>();
    YK_TransformComponent* groundTransform = groundPlane.GetComponent<YK_TransformComponent>();
    constexpr float angle = 90 * (3.14159265f / 180.0f);
    groundTransform->m_orientation = YK_Quaternion(YK_Vector3f::Right(), angle);
    groundTransform->m_scale = YK_Vector3f(30.0f);

    groundPlane.GetComponent<CG_MeshComponent>()->m_mesh = &g_quadMesh;
    groundPlane.GetComponent<CG_RendererComponent>()->m_material = &groundMaterial;

    Zen::Entity player = entityGarden.Spawn<YK_TransformComponent, CG_MeshComponent, CG_RendererComponent>();

    player.GetComponent<CG_MeshComponent>()->m_mesh = &heartMesh;
    player.GetComponent<CG_RendererComponent>()->m_material = &heartMaterial;

    // Spawn ground collider
    YK_TransformComponent groundCollider;
    groundCollider.m_position.y = -1.0f;

    entityGarden.Spawn(
      std::move(groundCollider),
      PP_RigidBodyComponent{ new JPH::BoxShape(JPH::Vec3(100.0f, 1.0f, 100.0f)), groundCollider, PP_BodyType::Static });
}

void HF_Game::Update(YK_Core& p_engine)
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

    // Test 2DR
    float uiUpDown = YK_Time::DeltaTime()
                     * (HIDra::GetKey(HIDra::KEYCODE_I) ? 1.0f :
                        HIDra::GetKey(HIDra::KEYCODE_K) ? -1.0f :
                                                          0.0f);
    float uiLeftRight = YK_Time::DeltaTime()
                        * (HIDra::GetKey(HIDra::KEYCODE_J) ? -1.0f :
                           HIDra::GetKey(HIDra::KEYCODE_L) ? 1.0f :
                                                             0.0f);
    float uiSpin = YK_Time::DeltaTime()
                   * (HIDra::GetKey(HIDra::KEYCODE_O) ? 1.0f :
                      HIDra::GetKey(HIDra::KEYCODE_U) ? -1.0f :
                                                        0.0f);

    CG_RenderModule& renderModule = static_cast<YakuEngine&>(p_engine).GetModules().GetRenderModule();
    CG_Canvas& canvas = renderModule.Get2DRenderer().GetCanvas(0);
    canvas.Scroll(YK_Vector2f(uiLeftRight, uiUpDown));
    canvas.Spin(uiSpin * 90.0f);

    // Test despawning and re-spawning
    static float timeSinceLastSpawn = 0;
    timeSinceLastSpawn += YK_Time::DeltaTime();

    if (timeSinceLastSpawn > 1.0f && g_moverEntities.size() < 10)
    {
        timeSinceLastSpawn = 0.0f;
        Temp_SpawnMover();
    }

    Zen::Garden& entityGarden = p_engine.GetZenGarden();
    if (g_moverEntities.size() != 0)
    {
        for (YK_SizeT i = g_moverEntities.size() - 1; i >= 0 && i != YK_NumericLimits<YK_SizeT>::Max; --i)
        {
            Zen::Entity const& e = g_moverEntities[i];

            YK_TransformComponent* t = e.GetComponent<YK_TransformComponent>();

            if (YK_Vector::Magnitude(t->m_position) > 6.0f)
            {
                entityGarden.Destroy(e);
                g_moverEntities.erase(g_moverEntities.begin() + i);
            }
        }
    }
}