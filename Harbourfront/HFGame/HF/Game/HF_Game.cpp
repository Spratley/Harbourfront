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
#include "YK/Types/Other/YK_DeferredConstructible.h"

#include "CG/2D/Canvas/CG_Canvas.h"
#include "CG/CG_RenderModule.h"
#include "CG/ECS/CG_Components.h"
#include "CG/Resource/Animation/CG_Animation.h"
#include "CG/Resource/Material/CG_Material.h"
#include "CG/Resource/Mesh/CG_Mesh.h"
#include "CG/Resource/Mesh/CG_MeshFactory.h"

#include "AM/ECS/AM_AnimationComponent.h"

#include "EN/Libraries/HIDra/HIDra.h"
#include "EN/Libraries/HIDra/HIDraEnums.h"
#include "EN/Libraries/HIDra/HIDraTypes.h"
#include "EN/Modules/EN_ModuleRegistry.h"
#include "EN/YakuEngine.h"

#include <Jolt/Jolt.h>
#include "PP/ECS/PP_RigidBodyComponent.h"
#include "PP/Libraries/Jolt/Math/Vec3.h"
#include "PP/Libraries/Jolt/Physics/Collision/Shape/BoxShape.h"
#include "PP/Libraries/Jolt/Physics/Collision/Shape/SphereShape.h"

#include "HF/Player/HF_Player.h"

#include <cstdlib>
#include <ctime>
#include <utility>

// Temp
CG_Mesh g_quadMesh;

YK_DeferredConstructible<HF_Player> HF_Game::m_player;

void HF_Game::Init(YK_Core& p_engine)
{
    // Temp
    YK_AssetManager& assetManager = p_engine.GetAssetManager();
    Zen::Garden& entityGarden = p_engine.GetZenGarden();

    m_player.Construct();

    // Gather Assets for Spawning
    g_quadMesh = CG_MeshFactory::Quad();

    // CG_Material const& heartMaterial = assetManager.GetAsset<CG_Material>(YK_FilePath("Materials/Main.YKM"));
    CG_Material const& groundMaterial = assetManager.GetAsset<CG_Material>(YK_FilePath("Materials/Ground.YKM"));

    std::srand(static_cast<unsigned int>(time(NULL)));

    Zen::Entity groundPlane = entityGarden.Spawn<YK_TransformComponent, CG_MeshComponent, CG_RendererComponent>();
    YK_TransformComponent* groundTransform = groundPlane.GetComponent<YK_TransformComponent>();
    constexpr float angle = 90 * (3.14159265f / 180.0f);
    groundTransform->m_orientation = YK_Quaternion(YK_Vector3f::Right(), angle);
    groundTransform->m_scale = YK_Vector3f(30.0f);

    groundPlane.GetComponent<CG_MeshComponent>()->m_mesh = &g_quadMesh;
    groundPlane.GetComponent<CG_RendererComponent>()->m_material = &groundMaterial;

    // Spawn ground collider
    YK_Transform groundCollider;
    groundCollider.m_position.y = -1.0f;

    entityGarden.Spawn(
      std::move(groundCollider),
      PP_RigidBodyComponent{ new JPH::BoxShape(JPH::Vec3(100.0f, 1.0f, 100.0f)), groundCollider, PP_BodyType::Static });
}

void HF_Game::Update(YK_Core& p_engine)
{
    m_player->Update();

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
}

void HF_Game::ShutDown(YK_Core&) { m_player.Destruct(); }