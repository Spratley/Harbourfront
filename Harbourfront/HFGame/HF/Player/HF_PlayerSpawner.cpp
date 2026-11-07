#include "PCH/HFGame_PCH.h"
#include "HF_PlayerSpawner.h"

#include "YK/ECS/Components/YK_TransformComponent.h"
#include "YK/IO/Asset/YK_AssetManager.h"
#include "YK/IO/File/YK_FilePath.h"
#include "YK/Libraries/Zen/Entity/Zen_Entity.h"
#include "YK/Libraries/Zen/Zen_Garden.h"
#include "YK/Types/Math/YK_Vector.h"

#include "CG/CG_RenderModule.h"
#include "CG/Camera/CG_CameraComponent.h"
#include "CG/ECS/CG_Components.h"
#include "CG/Resource/Animation/CG_Animation.h"
#include "CG/Resource/CG_ResourceHandles.h"
#include "CG/Resource/Material/CG_Material.h"
#include "CG/Resource/Mesh/CG_Mesh.h"
#include "CG/Resource/Skeleton/CG_Skeleton.h"

#include "AM/ECS/AM_AnimationComponent.h"
#include "AM/PoseSampler/AM_AnimationPoseSampler.h"
#include "AM/PoseSampler/AM_BlendPoseSampler.h"
#include "AM/PoseSampler/AM_LerpPoseSampler.h"
#include "AM/PoseSampler/AM_MaskedPoseSampler.h"
#include "AM/Resource/AM_BoneMask.h"
#include "AM/Resource/AM_ResourceHandles.h"

#include "EN/YakuEngine.h"

#include "HF/Player/HF_HammerComponent.h"
#include "HF/Player/HF_PlayerComponent.h"

#include <utility>

namespace HF_PlayerSpawner_Private
{
    // TODO: Create asset type for pose samplers so I don't have to do this
    AM_AnimationPoseSampler IdlePoseSampler;
    AM_AnimationPoseSampler WalkPoseSampler;
    AM_LerpPoseSampler MovementPoseSampler;
    AM_AnimationPoseSampler HammerPoseSamplers[5]; // From left to right
    AM_BlendPoseSampler HammerBlendPoseSampler;

    constexpr float HammerDistance = -2.5f;
} // namespace HF_PlayerSpawner_Private

Zen::Entity HF_PlayerSpawner::SpawnPlayer(YK_Vector3f const& p_position)
{
    YakuEngine& engine = YakuEngine::GetEngine();
    Zen::Garden& entityGarden = engine.GetZenGarden();
    YK_AssetManager& assetManager = engine.GetAssetManager();

    YK_FilePath playerMeshPath = YK_FilePath("Models/Debugger.glb");
    CG_MeshHandle mesh = assetManager.GetAsset<CG_Mesh>(playerMeshPath);
    CG_SkeletonHandle skeleton = assetManager.GetAsset<CG_Skeleton>(playerMeshPath);
    CG_MaterialHandle material = assetManager.GetAsset<CG_Material>(YK_FilePath("Materials/MainSkeletal.YKM"));

    // Load Pose Samplers
    HF_PlayerSpawner_Private::IdlePoseSampler =
      AM_AnimationPoseSampler(assetManager.GetAsset<CG_Animation>(YK_FilePath("Animation/Clips/Idle.YKA")));
    HF_PlayerSpawner_Private::WalkPoseSampler =
      AM_AnimationPoseSampler(assetManager.GetAsset<CG_Animation>(YK_FilePath("Animation/Clips/Run_F.YKA")));

    HF_PlayerSpawner_Private::HammerPoseSamplers[0] =
      AM_AnimationPoseSampler(assetManager.GetAsset<CG_Animation>(YK_FilePath("Animation/Clips/P_H_L.YKA")));
    HF_PlayerSpawner_Private::HammerPoseSamplers[1] =
      AM_AnimationPoseSampler(assetManager.GetAsset<CG_Animation>(YK_FilePath("Animation/Clips/P_H_BL.YKA")));
    HF_PlayerSpawner_Private::HammerPoseSamplers[2] =
      AM_AnimationPoseSampler(assetManager.GetAsset<CG_Animation>(YK_FilePath("Animation/Clips/P_H_Back.YKA")));
    HF_PlayerSpawner_Private::HammerPoseSamplers[3] =
      AM_AnimationPoseSampler(assetManager.GetAsset<CG_Animation>(YK_FilePath("Animation/Clips/P_H_BR.YKA")));
    HF_PlayerSpawner_Private::HammerPoseSamplers[4] =
      AM_AnimationPoseSampler(assetManager.GetAsset<CG_Animation>(YK_FilePath("Animation/Clips/P_H_R.YKA")));
    // End Load Pose Samplers

    // Setup Intermediary Pose Samplers
    HF_PlayerSpawner_Private::MovementPoseSampler =
      AM_LerpPoseSampler(&HF_PlayerSpawner_Private::IdlePoseSampler, &HF_PlayerSpawner_Private::WalkPoseSampler);

    HF_PlayerSpawner_Private::HammerBlendPoseSampler = AM_BlendPoseSampler();
    HF_PlayerSpawner_Private::HammerBlendPoseSampler.AddSampler(&HF_PlayerSpawner_Private::HammerPoseSamplers[0],
                                                                -1.0f);
    HF_PlayerSpawner_Private::HammerBlendPoseSampler.AddSampler(&HF_PlayerSpawner_Private::HammerPoseSamplers[1],
                                                                -0.5f);
    HF_PlayerSpawner_Private::HammerBlendPoseSampler.AddSampler(&HF_PlayerSpawner_Private::HammerPoseSamplers[2], 0.0f);
    HF_PlayerSpawner_Private::HammerBlendPoseSampler.AddSampler(&HF_PlayerSpawner_Private::HammerPoseSamplers[3], 0.5f);
    HF_PlayerSpawner_Private::HammerBlendPoseSampler.AddSampler(&HF_PlayerSpawner_Private::HammerPoseSamplers[4], 1.0f);
    // End Setup Intermediary Pose Samplers

    // Create components
    YK_TransformComponent playerTransform;
    playerTransform.m_position = p_position;

    CG_SkeletalMeshComponent skeletalMesh;
    skeletalMesh.m_mesh = mesh;
    skeletalMesh.m_skeleton = skeleton;

    CG_RendererComponent rendererComponent;
    rendererComponent.m_material = material;

    CG_PoseComponent poseComponent;
    poseComponent.m_pose.resize(skeleton->m_bones.size());

    AM_AnimationComponent animationComponent;

    YK_FilePath armsAndHammerMaskPath = YK_FilePath("Animation/BoneMasks/Debugger_ArmsAndHammer.AMBoneMask");
    AM_BoneMaskHandle armsAndHammerMask = assetManager.GetAsset<AM_BoneMask>(armsAndHammerMaskPath);
    animationComponent.m_poseSampler = AM_MaskedPoseSampler(*armsAndHammerMask,
                                                            &HF_PlayerSpawner_Private::HammerBlendPoseSampler,
                                                            &HF_PlayerSpawner_Private::MovementPoseSampler);

    CG_CameraComponent playerCamera;
    playerCamera.m_transform.m_position = YK_Vector3f::Up();

    HF_PlayerComponent playerComponent{ playerTransform };

    HF_HammerComponent hammerComponent{ playerTransform, playerComponent.m_playerBody };

    Zen::Entity playerEntity = entityGarden.Spawn(std::move(playerTransform),
                                                  std::move(skeletalMesh),
                                                  std::move(rendererComponent),
                                                  std::move(poseComponent),
                                                  std::move(animationComponent),
                                                  std::move(playerCamera),
                                                  std::move(playerComponent),
                                                  std::move(hammerComponent));

    engine.GetModules().GetRenderModule().SetActiveCamera(playerEntity);
    return playerEntity;
}