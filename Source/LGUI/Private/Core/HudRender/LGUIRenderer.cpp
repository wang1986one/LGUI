﻿// Copyright 2019-2021 LexLiu. All Rights Reserved.

#include "Core/HudRender/LGUIRenderer.h"
#include "Core/HudRender/LGUIHudShaders.h"
#include "Core/HudRender/LGUIHudVertex.h"
#include "Core/HudRender/LGUIPostProcessShaders.h"
#include "Modules/ModuleManager.h"
#include "LGUI.h"
#include "SceneView.h"
#include "Widgets/SWindow.h"
#include "StaticMeshVertexData.h"
#include "PipelineStateCache.h"
#include "SceneRendering.h"
#include "Core/HudRender/ILGUIHudPrimitive.h"
#include "Core/ActorComponent/LGUICanvas.h"
#include "MeshPassProcessor.inl"
#include "ScenePrivate.h"
#if WITH_EDITOR
#include "Engine/Engine.h"
#include "Editor/EditorEngine.h"
#endif
#include "Slate/SceneViewport.h"
#include "Core/ActorComponent/UIPostProcessRenderable.h"
#include "Core/Actor/LGUIManagerActor.h"
#include "Core/LGUISettings.h"
#include "Engine/TextureRenderTarget2D.h"

class FLGUIMeshElementCollector : FMeshElementCollector//why use a custom collector? because default FMeshElementCollector have no public constructor
{
public:
	FLGUIMeshElementCollector(ERHIFeatureLevel::Type InFeatureLevel) :FMeshElementCollector(InFeatureLevel)
	{

	}
};


#if WITH_EDITORONLY_DATA
uint32 FLGUIHudRenderer::EditorPreview_ViewKey = 0;
#endif
FLGUIHudRenderer::FLGUIHudRenderer(const FAutoRegister& AutoRegister, UWorld* InWorld, UTextureRenderTarget2D* InCustomRenderTarget)
	:FSceneViewExtensionBase(AutoRegister)
{
	World = InWorld;
	CustomRenderTarget = InCustomRenderTarget;

#if WITH_EDITORONLY_DATA
	bIsEditorPreview = !World->IsGameWorld();
#endif
}
FLGUIHudRenderer::~FLGUIHudRenderer()
{
	
}

void FLGUIHudRenderer::SetupView(FSceneViewFamily& InViewFamily, FSceneView& InView)
{
	if (!World.IsValid())return;
	if (World.Get() != InView.Family->Scene->GetWorld())return;
	//world space
	{

	}
	//screen space
	if (ScreenSpaceRenderParameter.RenderCanvas.IsValid())
	{
		//@todo: these parameters should use ENQUE_RENDER_COMMAND to pass to render thread
		auto ViewLocation = ScreenSpaceRenderParameter.RenderCanvas->GetViewLocation();
		auto ViewRotationMatrix = FInverseRotationMatrix(ScreenSpaceRenderParameter.RenderCanvas->GetViewRotator()) * FMatrix(
			FPlane(0, 0, 1, 0),
			FPlane(1, 0, 0, 0),
			FPlane(0, 1, 0, 0),
			FPlane(0, 0, 0, 1));
		auto ProjectionMatrix = ScreenSpaceRenderParameter.RenderCanvas->GetProjectionMatrix();
		auto ViewProjectionMatrix = ScreenSpaceRenderParameter.RenderCanvas->GetViewProjectionMatrix();

		ScreenSpaceRenderParameter.ViewOrigin = ViewLocation;
		ScreenSpaceRenderParameter.ViewRotationMatrix = ViewRotationMatrix;
		ScreenSpaceRenderParameter.ProjectionMatrix = ProjectionMatrix;
		ScreenSpaceRenderParameter.ViewProjectionMatrix = ViewProjectionMatrix;
	}
	MultiSampleCount = (uint16)ULGUISettings::GetAntiAliasingSampleCount();
}
void FLGUIHudRenderer::SetupViewPoint(APlayerController* Player, FMinimalViewInfo& InViewInfo)
{

}
void FLGUIHudRenderer::SetupViewProjectionMatrix(FSceneViewProjectionData& InOutProjectionData)
{
	
}
void FLGUIHudRenderer::BeginRenderViewFamily(FSceneViewFamily& InViewFamily)
{

}
void FLGUIHudRenderer::PostRenderView_RenderThread(FRHICommandListImmediate& RHICmdList, FSceneView& InView)
{
#if ENGINE_MAJOR_VERSION < 5
	RenderLGUI_RenderThread(RHICmdList, InView);
#endif
}
#if ENGINE_MAJOR_VERSION >= 5
void FLGUIHudRenderer::PostRenderView_RenderThread(FRDGBuilder& GraphBuilder, FSceneView& InView)
{
	AddPass(GraphBuilder, [this, &GraphBuilder, &InView](FRHICommandListImmediate& RHICmdList)
		{
			RenderLGUI_RenderThread(GraphBuilder, InView);
		});
}
#endif
int32 FLGUIHudRenderer::GetPriority() const
{
#if WITH_EDITOR
	auto Priority = ULGUISettings::GetPriorityInSceneViewExtension();
#else
	static auto Priority = ULGUISettings::GetPriorityInSceneViewExtension();
#endif
	return Priority;
}
#if ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION <= 26
bool FLGUIHudRenderer::IsActiveThisFrame(class FViewport* InViewport) const
{
	if (!World.IsValid())return false;
	if (InViewport == nullptr)return false;
	if (InViewport->GetClient() == nullptr)return false;
	if (World.Get() != InViewport->GetClient()->GetWorld())return false;//only render self world
	return true;
}
#else
bool FLGUIHudRenderer::IsActiveThisFrame_Internal(const FSceneViewExtensionContext& Context) const
{
	if (!World.IsValid())return false;
	if (World.Get() != Context.GetWorld())return false;//only render self world
	return true;
}
#endif
void FLGUIHudRenderer::PreRenderView_RenderThread(FRHICommandListImmediate& RHICmdList, FSceneView& InView)
{
	
}
void FLGUIHudRenderer::PostRenderBasePass_RenderThread(FRHICommandListImmediate& RHICmdList, FSceneView& InView)
{

}

void FLGUIHudRenderer::CopyRenderTarget(FRHICommandListImmediate& RHICmdList, FGlobalShaderMap* GlobalShaderMap, FTextureRHIRef Src, FTextureRHIRef Dst)
{
	RHICmdList.BeginRenderPass(FRHIRenderPassInfo(Dst, ERenderTargetActions::Clear_DontStore), TEXT("LGUICopyRenderTarget"));
	RHICmdList.SetViewport(0, 0, 0, Dst->GetSizeXYZ().X, Dst->GetSizeXYZ().Y, 1.0f);

	TShaderMapRef<FLGUISimplePostProcessVS> VertexShader(GlobalShaderMap);
	TShaderMapRef<FLGUISimpleCopyTargetPS> PixelShader(GlobalShaderMap);
	FGraphicsPipelineStateInitializer GraphicsPSOInit;
	RHICmdList.ApplyCachedRenderTargets(GraphicsPSOInit);
	GraphicsPSOInit.DepthStencilState = TStaticDepthStencilState<false, ECompareFunction::CF_Always>::GetRHI();
	GraphicsPSOInit.RasterizerState = TStaticRasterizerState<FM_Solid, CM_None, false>::GetRHI();
	GraphicsPSOInit.BlendState = TStaticBlendState<>::GetRHI();
	GraphicsPSOInit.BoundShaderState.VertexDeclarationRHI = GetLGUIPostProcessVertexDeclaration();
	GraphicsPSOInit.BoundShaderState.VertexShaderRHI = VertexShader.GetVertexShader();
	GraphicsPSOInit.BoundShaderState.PixelShaderRHI = PixelShader.GetPixelShader();
	GraphicsPSOInit.PrimitiveType = EPrimitiveType::PT_TriangleList;
	GraphicsPSOInit.NumSamples = Dst->GetNumSamples();
	SetGraphicsPipelineState(RHICmdList, GraphicsPSOInit);

	VertexShader->SetParameters(RHICmdList);
	PixelShader->SetParameters(RHICmdList, Src);

	DrawFullScreenQuad(RHICmdList);

	RHICmdList.EndRenderPass();
}
void FLGUIHudRenderer::CopyRenderTargetOnMeshRegion(FRHICommandListImmediate& RHICmdList
	, FGlobalShaderMap* GlobalShaderMap
	, FTextureRHIRef Src
	, FTextureRHIRef Dst
	, const TArray<FLGUIPostProcessCopyMeshRegionVertex>& RegionVertexData
	, const FMatrix& MVP
	, const FIntRect& ViewRect
	, const FVector4& SrcTextureScaleOffset
)
{
	RHICmdList.BeginRenderPass(FRHIRenderPassInfo(Dst, ERenderTargetActions::Clear_DontStore), TEXT("LGUICopyRenderTargetOnMeshRegion"));
	RHICmdList.SetViewport(ViewRect.Min.X, ViewRect.Min.Y, 0.0f, ViewRect.Max.X, ViewRect.Max.Y, 1.0f);

	TShaderMapRef<FLGUICopyMeshRegionVS> VertexShader(GlobalShaderMap);
	TShaderMapRef<FLGUICopyMeshRegionPS> PixelShader(GlobalShaderMap);
	FGraphicsPipelineStateInitializer GraphicsPSOInit;
	RHICmdList.ApplyCachedRenderTargets(GraphicsPSOInit);
	GraphicsPSOInit.DepthStencilState = TStaticDepthStencilState<false, ECompareFunction::CF_Always>::GetRHI();
	GraphicsPSOInit.RasterizerState = TStaticRasterizerState<FM_Solid, CM_None, false>::GetRHI();
	GraphicsPSOInit.BlendState = TStaticBlendState<>::GetRHI();
	GraphicsPSOInit.BoundShaderState.VertexDeclarationRHI = GetLGUIPostProcessCopyMeshRegionVertexDeclaration();
	GraphicsPSOInit.BoundShaderState.VertexShaderRHI = VertexShader.GetVertexShader();
	GraphicsPSOInit.BoundShaderState.PixelShaderRHI = PixelShader.GetPixelShader();
	GraphicsPSOInit.PrimitiveType = EPrimitiveType::PT_TriangleList;
	GraphicsPSOInit.NumSamples = Dst->GetNumSamples();
	SetGraphicsPipelineState(RHICmdList, GraphicsPSOInit);

	//VertexShader->SetParameters(RHICmdList);
	PixelShader->SetParameters(RHICmdList, MVP, SrcTextureScaleOffset, Src);

	uint32 VertexBufferSize = 4 * sizeof(FLGUIPostProcessCopyMeshRegionVertex);
#if ENGINE_MAJOR_VERSION >= 5
	FRHIResourceCreateInfo CreateInfo(TEXT("CopyRenderTargetOnMeshRegion"));
	FBufferRHIRef VertexBufferRHI = RHICreateVertexBuffer(VertexBufferSize, BUF_Volatile, CreateInfo);
	void* VoidPtr = RHILockBuffer(VertexBufferRHI, 0, VertexBufferSize, RLM_WriteOnly);
	FPlatformMemory::Memcpy(VoidPtr, RegionVertexData.GetData(), VertexBufferSize);
	RHIUnlockBuffer(VertexBufferRHI);
#else
	FRHIResourceCreateInfo CreateInfo;
	FVertexBufferRHIRef VertexBufferRHI = RHICreateVertexBuffer(VertexBufferSize, BUF_Volatile, CreateInfo);
	void* VoidPtr = RHILockVertexBuffer(VertexBufferRHI, 0, VertexBufferSize, RLM_WriteOnly);
	FPlatformMemory::Memcpy(VoidPtr, RegionVertexData.GetData(), VertexBufferSize);
	RHIUnlockVertexBuffer(VertexBufferRHI);
#endif

	RHICmdList.SetStreamSource(0, VertexBufferRHI, 0);
	RHICmdList.DrawIndexedPrimitive(GLGUIFullScreenQuadIndexBuffer.IndexBufferRHI, 0, 0, 4, 0, 2, 1);

	VertexBufferRHI.SafeRelease();

	RHICmdList.EndRenderPass();
}
void FLGUIHudRenderer::DrawFullScreenQuad(FRHICommandListImmediate& RHICmdList)
{
	RHICmdList.SetStreamSource(0, GLGUIFullScreenQuadVertexBuffer.VertexBufferRHI, 0);
	RHICmdList.DrawIndexedPrimitive(GLGUIFullScreenQuadIndexBuffer.IndexBufferRHI, 0, 0, 4, 0, 2, 1);
}
void FLGUIHudRenderer::SetGraphicPipelineStateFromMaterial(FGraphicsPipelineStateInitializer& GraphicsPSOInit, const FMaterial* Material)
{
	EBlendMode BlendMode = Material->GetBlendMode();
	switch (BlendMode)
	{
	default:
	case BLEND_Opaque:
		GraphicsPSOInit.BlendState = TStaticBlendState<>::GetRHI();
		break;
	case BLEND_Masked:
		GraphicsPSOInit.BlendState = TStaticBlendState<>::GetRHI();
		break;
	case BLEND_Translucent:
		GraphicsPSOInit.BlendState = TStaticBlendState<CW_RGBA, BO_Add, BF_SourceAlpha, BF_InverseSourceAlpha, BO_Add, BF_InverseDestAlpha, BF_One>::GetRHI();
		break;
	case BLEND_Additive:
		// Add to the existing scene color
		GraphicsPSOInit.BlendState = TStaticBlendState<CW_RGBA, BO_Add, BF_SourceAlpha, BF_One, BO_Add, BF_Zero, BF_InverseSourceAlpha>::GetRHI();
		//GraphicsPSOInit.BlendState = TStaticBlendState<CW_RGBA, BO_Add, BF_SourceAlpha, BF_One, BO_Add, BF_One, BF_One>::GetRHI();
		break;
	case BLEND_Modulate:
		// Modulate with the existing scene color
		GraphicsPSOInit.BlendState = TStaticBlendState<CW_RGB, BO_Add, BF_Zero, BF_SourceColor>::GetRHI();
		break;
	case BLEND_AlphaComposite:
		// Blend with existing scene color. New color is already pre-multiplied by alpha.
		GraphicsPSOInit.BlendState = TStaticBlendState<CW_RGBA, BO_Add, BF_One, BF_InverseSourceAlpha, BO_Add, BF_One, BF_InverseSourceAlpha>::GetRHI();
		break;
	case BLEND_AlphaHoldout:
		// Blend by holding out the matte shape of the source alpha
		GraphicsPSOInit.BlendState = TStaticBlendState<CW_RGBA, BO_Add, BF_Zero, BF_InverseSourceAlpha, BO_Add, BF_Zero, BF_InverseSourceAlpha>::GetRHI();
		break;
	};

	GraphicsPSOInit.DepthStencilState = TStaticDepthStencilState<false, ECompareFunction::CF_Always>::GetRHI();
	
	if (!Material->IsWireframe())
	{
		if (Material->IsTwoSided())
		{
			GraphicsPSOInit.RasterizerState = TStaticRasterizerState<FM_Solid, CM_None, false>::GetRHI();
		}
		else
		{
			GraphicsPSOInit.RasterizerState = TStaticRasterizerState<FM_Solid, CM_CW, false>::GetRHI();
		}
	}
	else
	{
		if (Material->IsTwoSided())
		{
			GraphicsPSOInit.RasterizerState = TStaticRasterizerState<FM_Wireframe, CM_None, true>::GetRHI();
		}
		else
		{
			GraphicsPSOInit.RasterizerState = TStaticRasterizerState<FM_Wireframe, CM_CW, true>::GetRHI();
		}
	}
}

DECLARE_CYCLE_STAT(TEXT("Hud RHIRender"), STAT_Hud_RHIRender, STATGROUP_LGUI);
void FLGUIHudRenderer::RenderLGUI_RenderThread(
#if ENGINE_MAJOR_VERSION >= 5
	FRDGBuilder& GraphBuilder
#else
	FRHICommandListImmediate& RHICmdList
#endif
	, FSceneView& InView)
{
	SCOPE_CYCLE_COUNTER(STAT_Hud_RHIRender);
	if (ScreenSpaceRenderParameter.HudPrimitiveArray.Num() <= 0 && WorldSpaceRenderCanvasParameterArray.Num() <= 0)return;//nothing to render

	FSceneView RenderView(InView);//use a copied view
	auto GlobalShaderMap = GetGlobalShaderMap(RenderView.GetFeatureLevel());

	//create render target
	FTextureRHIRef ScreenColorRenderTargetTexture = nullptr;
	FTextureRHIRef ScreenColorRenderTargetResolveTexture = nullptr;
	TRefCountPtr<IPooledRenderTarget> ScreenColorRenderTarget;

	auto ViewRect = RenderView.UnscaledViewRect;
#if ENGINE_MAJOR_VERSION >= 5
	FRHICommandListImmediate& RHICmdList = GraphBuilder.RHICmdList;
#endif
	if (CustomRenderTarget.IsValid())
	{
		ScreenColorRenderTargetTexture = (FTextureRHIRef)CustomRenderTarget->GetRenderTargetResource()->GetRenderTargetTexture();
		//clear render target;
		RHICmdList.BeginRenderPass(FRHIRenderPassInfo(ScreenColorRenderTargetTexture, ERenderTargetActions::Clear_DontStore), TEXT("LGUIHudRender_ClearRenderTarget"));
		RHICmdList.EndRenderPass();
	}
	else
	{
		if (bContainsPostProcess || MultiSampleCount > 1)
		{
			FPooledRenderTargetDesc desc(FPooledRenderTargetDesc::Create2DDesc(RenderView.Family->RenderTarget->GetRenderTargetTexture()->GetSizeXY(), RenderView.Family->RenderTarget->GetRenderTargetTexture()->GetFormat(), FClearValueBinding::Black, TexCreate_None, TexCreate_RenderTargetable, false));
			desc.NumSamples = MultiSampleCount;
			GRenderTargetPool.FindFreeElement(RHICmdList, desc, ScreenColorRenderTarget, TEXT("LGUISceneColorRenderTarget"));
			if (!ScreenColorRenderTarget.IsValid())
				return;
			ScreenColorRenderTargetTexture = ScreenColorRenderTarget->GetRenderTargetItem().TargetableTexture;
			ScreenColorRenderTargetResolveTexture = ScreenColorRenderTarget->GetRenderTargetItem().ShaderResourceTexture;
#if PLATFORM_WINDOWS || PLATFORM_XBOXONE || PLATFORM_MAC || PLATFORM_PS4 || PLATFORM_LINUX
			CopyRenderTarget(RHICmdList, GlobalShaderMap, (FTextureRHIRef)RenderView.Family->RenderTarget->GetRenderTargetTexture(), ScreenColorRenderTargetTexture);
#else
			RHICmdList.CopyToResolveTarget((FTextureRHIRef)RenderView.Family->RenderTarget->GetRenderTargetTexture(), ScreenColorRenderTargetTexture, FResolveParams());
#endif
		}
		else
		{
			ScreenColorRenderTargetTexture = (FTextureRHIRef)RenderView.Family->RenderTarget->GetRenderTargetTexture();
		}
	}

	auto RPInfo = FRHIRenderPassInfo(ScreenColorRenderTargetTexture, ERenderTargetActions::Load_DontStore);
#if ENGINE_MAJOR_VERSION >= 5
	const FSceneTextures& SceneTextures = FSceneTextures::Get(GraphBuilder);
#else
	FSceneRenderTargets& SceneContext = FSceneRenderTargets::Get(RHICmdList);
#endif
	FVector4 DepthTextureScaleOffset;
	FVector4 ViewTextureScaleOffset;
	switch (RenderView.StereoPass)
	{
	case EStereoscopicPass::eSSP_FULL:
	{
		DepthTextureScaleOffset = FVector4(
#if ENGINE_MAJOR_VERSION >= 5
			(float)ScreenColorRenderTargetTexture->GetSizeXYZ().X / SceneTextures.Depth.Target->Desc.GetSize().X,
			(float)ScreenColorRenderTargetTexture->GetSizeXYZ().Y / SceneTextures.Depth.Target->Desc.GetSize().Y,
#else
			(float)ScreenColorRenderTargetTexture->GetSizeXYZ().X / SceneContext.GetSceneDepthSurface()->GetSizeX(),
			(float)ScreenColorRenderTargetTexture->GetSizeXYZ().Y / SceneContext.GetSceneDepthSurface()->GetSizeY(),
#endif
			0, 0
		);
		ViewTextureScaleOffset = FVector4(1, 1, 0, 0);
	}
	break;
	case EStereoscopicPass::eSSP_LEFT_EYE:
	{
		DepthTextureScaleOffset = FVector4(
			//(float)ViewRect.Width() / SceneContext.GetSceneDepthSurface()->GetSizeX(),
			//(float)ViewRect.Height() / SceneContext.GetSceneDepthSurface()->GetSizeY(),
			0.5f, 1.0f,
			0, 0
		);
		ViewTextureScaleOffset = DepthTextureScaleOffset;
	}
	break;
	case EStereoscopicPass::eSSP_RIGHT_EYE:
	{
		DepthTextureScaleOffset = FVector4(
			//(float)ViewRect.Width() / SceneContext.GetSceneDepthSurface()->GetSizeX(),
			//(float)ViewRect.Height() / SceneContext.GetSceneDepthSurface()->GetSizeY(),
			0.5f, 1.0f,
			0.5f, 0
		);
		ViewTextureScaleOffset = DepthTextureScaleOffset;
	}
	break;
	}

	RHICmdList.BeginRenderPass(RPInfo, TEXT("LGUIHudRender"));
	RHICmdList.SetViewport(ViewRect.Min.X, ViewRect.Min.Y, 0.0f, ViewRect.Max.X, ViewRect.Max.Y, 1.0f);

	//Render world space
	if (WorldSpaceRenderCanvasParameterArray.Num() > 0)
	{
		auto ViewLocation = RenderView.ViewLocation;
		auto ViewRotationMatrix = FInverseRotationMatrix(RenderView.ViewRotation) * FMatrix(
			FPlane(0, 0, 1, 0),
			FPlane(1, 0, 0, 0),
			FPlane(0, 1, 0, 0),
			FPlane(0, 0, 0, 1));
		
		auto ProjectionMatrix = RenderView.SceneViewInitOptions.ProjectionMatrix;
		auto ViewProjectionMatrix = FTranslationMatrix(-ViewLocation) * (ViewRotationMatrix)*ProjectionMatrix;

		RenderView.SceneViewInitOptions.ViewOrigin = ViewLocation;
		RenderView.SceneViewInitOptions.ViewRotationMatrix = ViewRotationMatrix;
		RenderView.UpdateProjectionMatrix(ProjectionMatrix);

		FViewUniformShaderParameters ViewUniformShaderParameters;
		RenderView.SetupCommonViewUniformBufferParameters(
			ViewUniformShaderParameters,
			ViewRect.Size(),
			MultiSampleCount,
			ViewRect,
			RenderView.ViewMatrices,
			FViewMatrices()
		);

		RenderView.ViewUniformBuffer = TUniformBufferRef<FViewUniformShaderParameters>::CreateUniformBufferImmediate(ViewUniformShaderParameters, UniformBuffer_SingleFrame);

		FLGUIMeshElementCollector meshCollector(RenderView.GetFeatureLevel());
		FGraphicsPipelineStateInitializer GraphicsPSOInit;
		RHICmdList.ApplyCachedRenderTargets(GraphicsPSOInit);
		GraphicsPSOInit.NumSamples = MultiSampleCount;

		for (int canvasIndex = 0; canvasIndex < WorldSpaceRenderCanvasParameterArray.Num(); canvasIndex++)
		{
			auto& canvasParamItem = WorldSpaceRenderCanvasParameterArray[canvasIndex];

			for (int primitiveIndex = 0; primitiveIndex < canvasParamItem.HudPrimitiveArray.Num(); primitiveIndex++)
			{
				auto hudPrimitive = canvasParamItem.HudPrimitiveArray[primitiveIndex];
				if (hudPrimitive != nullptr)
				{
					if (hudPrimitive->CanRender())
					{
						if (hudPrimitive->GetIsPostProcess())//render post process
						{
							RHICmdList.EndRenderPass();
							hudPrimitive->OnRenderPostProcess_RenderThread(
#if ENGINE_MAJOR_VERSION >= 5
								GraphBuilder,
#else
								RHICmdList,
#endif
								this,
								(FTextureRHIRef)RenderView.Family->RenderTarget->GetRenderTargetTexture(),
								ScreenColorRenderTargetTexture,
								ScreenColorRenderTargetResolveTexture,
								GlobalShaderMap,
								ViewProjectionMatrix,
								true,
								canvasParamItem.BlendDepth,
								ViewRect,
								DepthTextureScaleOffset,
								ViewTextureScaleOffset
							);
							RHICmdList.BeginRenderPass(RPInfo, TEXT("LGUIHudRender"));
							RHICmdList.SetViewport(ViewRect.Min.X, ViewRect.Min.Y, 0.0f, ViewRect.Max.X, ViewRect.Max.Y, 1.0f);
							GraphicsPSOInit.NumSamples = MultiSampleCount;
						}
						else//render mesh
						{
							MeshBatchArray.Reset();
							hudPrimitive->GetMeshElements(*RenderView.Family, (FMeshElementCollector*)&meshCollector, MeshBatchArray);
							for (int MeshIndex = 0; MeshIndex < MeshBatchArray.Num(); MeshIndex++)
							{
								auto& MeshBatchContainer = MeshBatchArray[MeshIndex];
								const FMeshBatch& Mesh = MeshBatchContainer.Mesh;
#if ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION <= 26
								auto Material = Mesh.MaterialRenderProxy->GetMaterial(RenderView.GetFeatureLevel());
#else
								auto Material = &Mesh.MaterialRenderProxy->GetIncompleteMaterialWithFallback(RenderView.GetFeatureLevel());
#endif
								const FMaterialShaderMap* MaterialShaderMap = Material->GetRenderingThreadShaderMap();
								auto VertexShader = MaterialShaderMap->GetShader<FLGUIHudRenderVS>();

								auto PixelShader = MaterialShaderMap->GetShader<FLGUIWorldRenderPS>();
								if (VertexShader.IsValid() && PixelShader.IsValid())
								{
									SetGraphicPipelineStateFromMaterial(GraphicsPSOInit, Material);

									GraphicsPSOInit.BoundShaderState.VertexDeclarationRHI = GetLGUIHudVertexDeclaration();
									GraphicsPSOInit.BoundShaderState.VertexShaderRHI = VertexShader.GetVertexShader();
									GraphicsPSOInit.BoundShaderState.PixelShaderRHI = PixelShader.GetPixelShader();
									GraphicsPSOInit.PrimitiveType = EPrimitiveType::PT_TriangleList;
									GraphicsPSOInit.NumSamples = MultiSampleCount;
									SetGraphicsPipelineState(RHICmdList, GraphicsPSOInit);

									VertexShader->SetMaterialShaderParameters(RHICmdList, RenderView, Mesh.MaterialRenderProxy, Material, Mesh);
									PixelShader->SetMaterialShaderParameters(RHICmdList, RenderView, Mesh.MaterialRenderProxy, Material, Mesh);
#if ENGINE_MAJOR_VERSION >= 5
									PixelShader->SetDepthBlendParameter(RHICmdList, canvasParamItem.BlendDepth, DepthTextureScaleOffset, (FRHITexture2D*)(SceneTextures.Depth.Target->GetRHI()));
#else
									PixelShader->SetDepthBlendParameter(RHICmdList, canvasParamItem.BlendDepth, DepthTextureScaleOffset, SceneContext.GetSceneDepthSurface());
#endif

									RHICmdList.SetStreamSource(0, MeshBatchContainer.VertexBufferRHI, 0);
									RHICmdList.DrawIndexedPrimitive(Mesh.Elements[0].IndexBuffer->IndexBufferRHI, 0, 0, MeshBatchContainer.NumVerts, 0, Mesh.GetNumPrimitives(), 1);
								}
							}
						}
					}
				}
			}
		}
	}
	//Render screen space
	if (ScreenSpaceRenderParameter.HudPrimitiveArray.Num() > 0)
	{
#if WITH_EDITOR
		//check if simulation
		if (GEngine == nullptr)goto END_LGUI_RENDER;
		if (UEditorEngine* editor = Cast<UEditorEngine>(GEngine))
		{
			if (editor->bIsSimulatingInEditor)goto END_LGUI_RENDER;
		}

		if (ALGUIManagerActor::IsPlaying == bIsEditorPreview)goto END_LGUI_RENDER;
		if (ALGUIManagerActor::IsPlaying)
		{
			if (!InView.bIsGameView)goto END_LGUI_RENDER;
		}
		else//editor viewport preview
		{
			if (InView.GetViewKey() != EditorPreview_ViewKey)goto END_LGUI_RENDER;//only preview in specific viewport in editor
		}
#endif

		RenderView.SceneViewInitOptions.ViewOrigin = ScreenSpaceRenderParameter.ViewOrigin;
		RenderView.SceneViewInitOptions.ViewRotationMatrix = ScreenSpaceRenderParameter.ViewRotationMatrix;
		RenderView.UpdateProjectionMatrix(ScreenSpaceRenderParameter.ProjectionMatrix);

		FViewUniformShaderParameters ViewUniformShaderParameters;
		RenderView.SetupCommonViewUniformBufferParameters(
			ViewUniformShaderParameters,
			ViewRect.Size(),
			MultiSampleCount,
			ViewRect,
			RenderView.ViewMatrices,
			FViewMatrices()
		);

		RenderView.ViewUniformBuffer = TUniformBufferRef<FViewUniformShaderParameters>::CreateUniformBufferImmediate(ViewUniformShaderParameters, UniformBuffer_SingleFrame);

		FLGUIMeshElementCollector meshCollector(RenderView.GetFeatureLevel());
		FGraphicsPipelineStateInitializer GraphicsPSOInit;
		RHICmdList.ApplyCachedRenderTargets(GraphicsPSOInit);
		GraphicsPSOInit.NumSamples = MultiSampleCount;

		for (int primitiveIndex = 0; primitiveIndex < ScreenSpaceRenderParameter.HudPrimitiveArray.Num(); primitiveIndex++)
		{
			auto hudPrimitive = ScreenSpaceRenderParameter.HudPrimitiveArray[primitiveIndex];
			if (hudPrimitive != nullptr)
			{
				if (hudPrimitive->CanRender())
				{
					if (hudPrimitive->GetIsPostProcess())//render post process
					{
						RHICmdList.EndRenderPass();
						hudPrimitive->OnRenderPostProcess_RenderThread(
#if ENGINE_MAJOR_VERSION >= 5
							GraphBuilder,
#else
							RHICmdList,
#endif
							this,
							(FTextureRHIRef)RenderView.Family->RenderTarget->GetRenderTargetTexture(),
							ScreenColorRenderTargetTexture,
							ScreenColorRenderTargetResolveTexture,
							GlobalShaderMap,
							ScreenSpaceRenderParameter.ViewProjectionMatrix,
							false,
							0.0f,
							ViewRect,
							DepthTextureScaleOffset,
							ViewTextureScaleOffset
						);
						RHICmdList.BeginRenderPass(RPInfo, TEXT("LGUIHudRender"));
						RHICmdList.SetViewport(ViewRect.Min.X, ViewRect.Min.Y, 0.0f, ViewRect.Max.X, ViewRect.Max.Y, 1.0f);
						GraphicsPSOInit.NumSamples = MultiSampleCount;
					}
					else//render mesh
					{
						MeshBatchArray.Reset();
						hudPrimitive->GetMeshElements(*RenderView.Family, (FMeshElementCollector*)&meshCollector, MeshBatchArray);
						for (int MeshIndex = 0; MeshIndex < MeshBatchArray.Num(); MeshIndex++)
						{
							auto& MeshBatchContainer = MeshBatchArray[MeshIndex];
							const FMeshBatch& Mesh = MeshBatchContainer.Mesh;
#if ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION <= 26
							auto Material = Mesh.MaterialRenderProxy->GetMaterial(RenderView.GetFeatureLevel());
#else
							auto Material = &Mesh.MaterialRenderProxy->GetIncompleteMaterialWithFallback(RenderView.GetFeatureLevel());
#endif
							const FMaterialShaderMap* MaterialShaderMap = Material->GetRenderingThreadShaderMap();
							auto VertexShader = MaterialShaderMap->GetShader<FLGUIHudRenderVS>();

							auto PixelShader = MaterialShaderMap->GetShader<FLGUIHudRenderPS>();
							if (VertexShader.IsValid() && PixelShader.IsValid())
							{
								SetGraphicPipelineStateFromMaterial(GraphicsPSOInit, Material);

								GraphicsPSOInit.BoundShaderState.VertexDeclarationRHI = GetLGUIHudVertexDeclaration();
								GraphicsPSOInit.BoundShaderState.VertexShaderRHI = VertexShader.GetVertexShader();
								GraphicsPSOInit.BoundShaderState.PixelShaderRHI = PixelShader.GetPixelShader();
								GraphicsPSOInit.PrimitiveType = EPrimitiveType::PT_TriangleList;
								GraphicsPSOInit.NumSamples = MultiSampleCount;
								SetGraphicsPipelineState(RHICmdList, GraphicsPSOInit);

								VertexShader->SetMaterialShaderParameters(RHICmdList, RenderView, Mesh.MaterialRenderProxy, Material, Mesh);
								PixelShader->SetMaterialShaderParameters(RHICmdList, RenderView, Mesh.MaterialRenderProxy, Material, Mesh);

								RHICmdList.SetStreamSource(0, MeshBatchContainer.VertexBufferRHI, 0);
								RHICmdList.DrawIndexedPrimitive(Mesh.Elements[0].IndexBuffer->IndexBufferRHI, 0, 0, MeshBatchContainer.NumVerts, 0, Mesh.GetNumPrimitives(), 1);
							}
						}
					}
				}
			}
		}
	}
#if WITH_EDITOR
	END_LGUI_RENDER :
#endif

	RHICmdList.EndRenderPass();
	//copy back to screen
	if (CustomRenderTarget.IsValid())
	{
		
	}
	else
	{
		if (bContainsPostProcess || MultiSampleCount > 1)
		{
#if PLATFORM_WINDOWS || PLATFORM_XBOXONE || PLATFORM_MAC || PLATFORM_PS4 || PLATFORM_LINUX
			RHICmdList.CopyToResolveTarget(ScreenColorRenderTargetTexture, (FTextureRHIRef)RenderView.Family->RenderTarget->GetRenderTargetTexture(), FResolveParams());
#else
			CopyRenderTarget(RHICmdList, GlobalShaderMap, ScreenColorRenderTargetTexture, (FTextureRHIRef)RenderView.Family->RenderTarget->GetRenderTargetTexture());
#endif
		}
	}
	//release render target
	if (ScreenColorRenderTarget.IsValid())
	{
		ScreenColorRenderTarget.SafeRelease();
	}
}

void FLGUIHudRenderer::AddWorldSpacePrimitive_RenderThread(ULGUICanvas* InCanvas, int32 InRenderCanvasSortOrder, ILGUIHudPrimitive* InPrimitive)
{
	if (InPrimitive != nullptr)
	{
		int existIndex = WorldSpaceRenderCanvasParameterArray.IndexOfByPredicate([InCanvas](const FRenderCanvasParameter& item) {
			return item.RenderCanvas == InCanvas;
			});
		if (existIndex == INDEX_NONE)
		{
			UE_LOG(LGUI, Error, TEXT("[FLGUIHudRenderer::AddWorldSpacePrimitive_RenderThread]Canvas not exist!"));
		}
		else
		{
			auto& item = WorldSpaceRenderCanvasParameterArray[existIndex];
			item.HudPrimitiveArray.AddUnique(InPrimitive);
			item.HudPrimitiveArray.Sort([](ILGUIHudPrimitive& A, ILGUIHudPrimitive& B)
			{
				return A.GetRenderPriority() < B.GetRenderPriority();
			});
			//check if we have postprocess
			CheckContainsPostProcess_RenderThread();
		}
	}
	else
	{
		UE_LOG(LGUI, Warning, TEXT("[FLGUIHudRenderer::AddWorldSpacePrimitive_RenderThread]Add nullptr as ILGUIHudPrimitive!"));
	}
}
void FLGUIHudRenderer::RemoveWorldSpacePrimitive_RenderThread(ULGUICanvas* InCanvas, ILGUIHudPrimitive* InPrimitive)
{
	if (InPrimitive != nullptr)
	{
		int existIndex = WorldSpaceRenderCanvasParameterArray.IndexOfByPredicate([InCanvas](const FRenderCanvasParameter& item) {
			return item.RenderCanvas == InCanvas;
			});
		if (existIndex == INDEX_NONE)
		{
			UE_LOG(LGUI, Log, TEXT("[FLGUIHudRenderer::RemoveHudPrimitive]Canvas already removed."));
		}
		else
		{
			auto& item = WorldSpaceRenderCanvasParameterArray[existIndex];
			item.HudPrimitiveArray.RemoveSingle(InPrimitive);
			//check if we have postprocess
			CheckContainsPostProcess_RenderThread();
		}
	}
	else
	{
		UE_LOG(LGUI, Warning, TEXT("[FLGUIHudRenderer::RemoveWorldSpacePrimitive_RenderThread]Remove nullptr as ILGUIHudPrimitive!"));
	}
}
void FLGUIHudRenderer::AddScreenSpacePrimitive_RenderThread(ILGUIHudPrimitive* InPrimitive)
{
	if (InPrimitive != nullptr)
	{
		ScreenSpaceRenderParameter.HudPrimitiveArray.AddUnique(InPrimitive);
		SortScreenSpacePrimitiveRenderPriority_RenderThread();
		CheckContainsPostProcess_RenderThread();
	}
	else
	{
		UE_LOG(LGUI, Warning, TEXT("[FLGUIHudRenderer::AddScreenSpacePrimitive_RenderThread]Add nullptr as ILGUIHudPrimitive!"));
	}
}
void FLGUIHudRenderer::RemoveScreenSpacePrimitive_RenderThread(ILGUIHudPrimitive* InPrimitive)
{
	if (InPrimitive != nullptr)
	{
		ScreenSpaceRenderParameter.HudPrimitiveArray.RemoveSingle(InPrimitive);
		CheckContainsPostProcess_RenderThread();
	}
	else
	{
		UE_LOG(LGUI, Warning, TEXT("[FLGUIHudRenderer::RemoveScreenSpacePrimitive_RenderThread]Remove nullptr as ILGUIHudPrimitive!"));
	}
}
void FLGUIHudRenderer::SortScreenSpacePrimitiveRenderPriority_RenderThread()
{
	ScreenSpaceRenderParameter.HudPrimitiveArray.Sort([](ILGUIHudPrimitive& A, ILGUIHudPrimitive& B)
		{
			return A.GetRenderPriority() < B.GetRenderPriority();
		});
}
void FLGUIHudRenderer::SortPrimitiveRenderPriority_RenderThread()
{
	for (auto& item : WorldSpaceRenderCanvasParameterArray)
	{
		item.HudPrimitiveArray.Sort([](ILGUIHudPrimitive& A, ILGUIHudPrimitive& B) {
			return A.GetRenderPriority() < B.GetRenderPriority();
			});
	}
}
void FLGUIHudRenderer::SetRenderCanvasSortOrder(ULGUICanvas* InRenderCanvas, int32 InSortOrder)
{
	auto viewExtension = this;
	ENQUEUE_RENDER_COMMAND(FLGUICanvas_SetCanvasSortOrder)(
		[viewExtension, InRenderCanvas, InSortOrder](FRHICommandListImmediate& RHICmdList)
		{
			viewExtension->SetRenderCanvasSortOrder_RenderThread(InRenderCanvas, InSortOrder);
		});
}
void FLGUIHudRenderer::SetRenderCanvasSortOrder_RenderThread(ULGUICanvas* InRenderCanvas, int32 InSortOrder)
{
	int existIndex = WorldSpaceRenderCanvasParameterArray.IndexOfByPredicate([InRenderCanvas](const FRenderCanvasParameter& item) {
		return item.RenderCanvas == InRenderCanvas;
		});
	if (existIndex == INDEX_NONE)
	{
		UE_LOG(LGUI, Error, TEXT("[FLGUIHudRenderer::RemoveHudPrimitive]Canvas not exist!"));
	}
	else
	{
		WorldSpaceRenderCanvasParameterArray[existIndex].RenderCanvasSortOrder = InSortOrder;
		WorldSpaceRenderCanvasParameterArray.Sort([](const FRenderCanvasParameter& A, const FRenderCanvasParameter& B) {
			return A.RenderCanvasSortOrder < B.RenderCanvasSortOrder;
			});
	}
}
void FLGUIHudRenderer::SortPrimitiveRenderPriority()
{
	auto viewExtension = this;
	ENQUEUE_RENDER_COMMAND(FLGUIRender_SortPrimitiveRenderPriority)(
		[viewExtension](FRHICommandListImmediate& RHICmdList)
		{
			viewExtension->SortPrimitiveRenderPriority_RenderThread();
		}
	);
}
void FLGUIHudRenderer::SetRenderCanvasBlendDepth(ULGUICanvas* InRenderCanvas, float InBlendDepth)
{
	auto viewExtension = this;
	ENQUEUE_RENDER_COMMAND(FLGUIRender_SortRenderPriority)(
		[viewExtension, InRenderCanvas, InBlendDepth](FRHICommandListImmediate& RHICmdList)
		{
			viewExtension->SetRenderCanvasBlendDepth_RenderThread(InRenderCanvas, InBlendDepth);
		}
	);
}
void FLGUIHudRenderer::SetRenderCanvasBlendDepth_RenderThread(ULGUICanvas* InRenderCanvas, float InBlendDepth)
{
	int existIndex = WorldSpaceRenderCanvasParameterArray.IndexOfByPredicate([InRenderCanvas](const FRenderCanvasParameter& item) {
		return item.RenderCanvas == InRenderCanvas;
		});
	if (existIndex == INDEX_NONE)
	{
		UE_LOG(LGUI, Error, TEXT("[FLGUIHudRenderer::RemoveHudPrimitive]Canvas not exist!"));
	}
	else
	{
		//WorldSpaceRenderCanvasParameterArray[existIndex].BlendDepth = FMath::Pow(InBlendDepth, 2.2f);
		WorldSpaceRenderCanvasParameterArray[existIndex].BlendDepth = InBlendDepth;
	}
}
void FLGUIHudRenderer::AddWorldSpaceRenderCanvas(ULGUICanvas* InCanvas)
{
	FRenderCanvasParameter canvasItem;
	canvasItem.RenderCanvas = InCanvas;
	canvasItem.RenderCanvasSortOrder = InCanvas->GetSortOrder();
	//canvasItem.BlendDepth = FMath::Pow(InCanvas->GetActualBlendDepth(), 2.2f);
	canvasItem.BlendDepth = InCanvas->GetActualBlendDepth();

	auto viewExtension = this;
	ENQUEUE_RENDER_COMMAND(FLGUIRender_SortRenderPriority)(
		[viewExtension, canvasItem](FRHICommandListImmediate& RHICmdList)
		{
			viewExtension->AddWorldSpaceRenderCanvas_RenderThread(canvasItem);
		}
	);
}
void FLGUIHudRenderer::RemoveWorldSpaceRenderCanvas(ULGUICanvas* InCanvas)
{
	auto viewExtension = this;
	auto canvas = InCanvas;
	ENQUEUE_RENDER_COMMAND(FLGUIRender_SortRenderPriority)(
		[viewExtension, canvas](FRHICommandListImmediate& RHICmdList)
		{
			viewExtension->RemoveWorldSpaceRenderCanvas_RenderThread(canvas);
		}
	);
}

void FLGUIHudRenderer::SetScreenSpaceRenderCanvas(ULGUICanvas* InCanvas)
{
	ScreenSpaceRenderParameter.RenderCanvas = InCanvas;
}
void FLGUIHudRenderer::ClearScreenSpaceRenderCanvas()
{
	ScreenSpaceRenderParameter.RenderCanvas = nullptr;
}

void FLGUIHudRenderer::AddWorldSpaceRenderCanvas_RenderThread(FRenderCanvasParameter InCanvasParameter)
{
	int existIndex = WorldSpaceRenderCanvasParameterArray.IndexOfByPredicate([&InCanvasParameter](const FRenderCanvasParameter& item) {
		return item.RenderCanvas == InCanvasParameter.RenderCanvas;
		});
	if (existIndex != INDEX_NONE)
	{
		UE_LOG(LGUI, Error, TEXT("[FLGUIHudRenderer::AddRootCanvas_RenderThread]Canvas already exist!"));
		return;
	}
	WorldSpaceRenderCanvasParameterArray.Add(InCanvasParameter);
	WorldSpaceRenderCanvasParameterArray.Sort([](const FRenderCanvasParameter& A, const FRenderCanvasParameter& B) {
		return A.RenderCanvasSortOrder < B.RenderCanvasSortOrder;
		});
}
void FLGUIHudRenderer::RemoveWorldSpaceRenderCanvas_RenderThread(ULGUICanvas* InCanvas)
{
	int existIndex = WorldSpaceRenderCanvasParameterArray.IndexOfByPredicate([InCanvas](const FRenderCanvasParameter& item) {
		return item.RenderCanvas == InCanvas;
		});
	if (existIndex == INDEX_NONE)
	{
		UE_LOG(LGUI, Error, TEXT("[FLGUIHudRenderer::RemoveRootCanvas_RenderThread]Canvas not exist!"));
		return;
	}
	WorldSpaceRenderCanvasParameterArray.RemoveAt(existIndex);
}

void FLGUIHudRenderer::CheckContainsPostProcess_RenderThread()
{
	bContainsPostProcess = false;
	for (auto& item : ScreenSpaceRenderParameter.HudPrimitiveArray)
	{
		if (item->GetIsPostProcess())
		{
			bContainsPostProcess = true;
			return;
		}
	}
	for (auto& renderItem : WorldSpaceRenderCanvasParameterArray)
	{
		for (auto& item : renderItem.HudPrimitiveArray)
		{
			if (item->GetIsPostProcess())
			{
				bContainsPostProcess = true;
				return;
			}
		}
	}
}

void FLGUIFullScreenQuadVertexBuffer::InitRHI()
{
	TResourceArray<FLGUIPostProcessVertex, VERTEXBUFFER_ALIGNMENT> Vertices;
	Vertices.SetNumUninitialized(4);

	Vertices[0] = FLGUIPostProcessVertex(FVector(-1, -1, 0), FVector2D(0.0f, 1.0f));
	Vertices[1] = FLGUIPostProcessVertex(FVector(1, -1, 0), FVector2D(1.0f, 1.0f));
	Vertices[2] = FLGUIPostProcessVertex(FVector(-1, 1, 0), FVector2D(0.0f, 0.0f));
	Vertices[3] = FLGUIPostProcessVertex(FVector(1, 1, 0), FVector2D(1.0f, 0.0f));

#if ENGINE_MAJOR_VERSION >= 5
	FRHIResourceCreateInfo CreateInfo(TEXT("LGUIFullScreenQuadVertexBuffer"), &Vertices);
#else
	FRHIResourceCreateInfo CreateInfo(&Vertices);
#endif
	VertexBufferRHI = RHICreateVertexBuffer(Vertices.GetResourceDataSize(), BUF_Static, CreateInfo);
}
void FLGUIFullScreenQuadIndexBuffer::InitRHI()
{
	const uint16 Indices[] =
	{
		0, 2, 3,
		0, 3, 1
	};

	TResourceArray<uint16, INDEXBUFFER_ALIGNMENT> IndexBuffer;
	uint32 NumIndices = UE_ARRAY_COUNT(Indices);
	IndexBuffer.AddUninitialized(NumIndices);
	FMemory::Memcpy(IndexBuffer.GetData(), Indices, NumIndices * sizeof(uint16));

#if ENGINE_MAJOR_VERSION >= 5
	FRHIResourceCreateInfo CreateInfo(TEXT("LGUIFullScreenQuadIndexBuffer"), &IndexBuffer);
#else
	FRHIResourceCreateInfo CreateInfo(&IndexBuffer);
#endif
	IndexBufferRHI = RHICreateIndexBuffer(sizeof(uint16), IndexBuffer.GetResourceDataSize(), BUF_Static, CreateInfo);
}
