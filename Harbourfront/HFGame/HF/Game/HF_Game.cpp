#include "PCH/HFGame_PCH.h"
#include "HF_Game.h"

#include "HF/ECS/HF_Temp_PlayerComponent.h"

#include "EN/YakuEngine.h"

#include "CG/Material/CG_Material.h"
#include "CG/Renderable/CG_Renderable.h"
#include "CG/Resource/Mesh/CG_MeshFactory.h"
#include "CG/Resource/Shader/CG_Shader.h"
#include "CG/Resource/Shader/CG_ShaderResource.h"
#include "CG/Resource/Texture/CG_TextureFactory.h"

#include "YK/ECS/Components/YK_TransformComponent.h"

// HORRIBLE!!! DON'T DO THIS!!!
CG_Material g_heartMaterial2;
CG_Shader* g_mainShader2;

bool HF_Game::Init(YakuEngine& p_engine)
{
    Zen::Entity player =
      p_engine.GetGarden().Spawn<YK_TransformComponent, CG_MeshComponent, CG_RendererComponent, HF_PlayerComponent>({},
                                                                                                                    {},
                                                                                                                    {},
                                                                                                                    {});

    // DO NOT DO THIS!!! THIS IS THE OPPOSITE OF FLYWEIGHT!!!
    player.GetComponent<CG_MeshComponent>()->m_mesh =
      CG_MeshFactory::LoadOBJ("J:/Harbourfront/Data/Models/HeartTest.obj");
    player.GetComponent<CG_RendererComponent>()->m_material = &g_heartMaterial2;

    g_mainShader2 = new CG_Shader(CG_ShaderLoader::Load("J:/Harbourfront/Data/Shaders/ShaderCode/Vertex.vs",
                                                        "J:/Harbourfront/Data/Shaders/ShaderCode/Fragment.fs"));

    g_heartMaterial2.m_shader = g_mainShader2;
    g_heartMaterial2.m_texture = CG_TextureFactory::LoadPNG("J:/Harbourfront/Data/Textures/HeartTest.png");

    return true;
}