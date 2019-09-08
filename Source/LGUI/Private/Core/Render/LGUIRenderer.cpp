﻿// Copyright 2019 LexLiu. All Rights Reserved.

#include "Core/Render/LGUIRenderer.h"
#include "Core/Render/LGUIShaders.h"
#include "Modules/ModuleManager.h"
#include "LGUI.h"
#include "SceneView.h"
#include "Widgets/SWindow.h"
#include "StaticMeshVertexData.h"
#include "PipelineStateCache.h"
#include "SceneRendering.h"
#include "Core/Render/ILGUIHudPrimitive.h"
#include "Core/ActorComponent/LGUICanvas.h"
#include "MeshPassProcessor.inl"
#include "ScenePrivate.h"
#if WITH_EDITOR
#include "Engine.h"
#include "Editor/EditorEngine.h"
#endif


class FLGUIMeshElementCollector : FMeshElementCollector
{
public:
	FLGUIMeshElementCollector(ERHIFeatureLevel::Type InFeatureLevel) :FMeshElementCollector(InFeatureLevel)
	{

	}
};



FLGUIViewExtension::FLGUIViewExtension(const FAutoRegister& AutoRegister, ULGUICanvas* InLGUICanvas)
	:FSceneViewExtensionBase(AutoRegister)
{
	UICanvas = InLGUICanvas;
}
FLGUIViewExtension::~FLGUIViewExtension()
{
	
}
void FLGUIViewExtension::SetupView(FSceneViewFamily& InViewFamily, FSceneView& InView)
{
	
}
void FLGUIViewExtension::SetupViewPoint(APlayerController* Player, FMinimalViewInfo& InViewInfo)
{
	if (!UICanvas.IsValid())return;
	ViewLocation = UICanvas->GetViewLocation();
	ViewRotationMatrix = UICanvas->GetViewRotationMatrix();
	ProjectionMatrix = UICanvas->GetProjectionMatrix();
	ViewProjectionMatrix = UICanvas->GetViewProjectionMatrix();
}
void FLGUIViewExtension::SetupViewProjectionMatrix(FSceneViewProjectionData& InOutProjectionData)
{
	
}

DECLARE_CYCLE_STAT(TEXT("Hud RHIRender"), STAT_Hud_RHIRender, STATGROUP_LGUI);

void FLGUIViewExtension::PostRenderView_RenderThread(FRHICommandListImmediate& RHICmdList, FSceneView& InView)
{
	SCOPE_CYCLE_COUNTER(STAT_Hud_RHIRender);
	check(IsInRenderingThread());
	if (!InView.bIsGameView)return;
#if WITH_EDITOR
	if (GEngine == nullptr)return;
	if (UEditorEngine* editor = CastChecked<UEditorEngine>(GEngine))
	{
		if (editor->bIsSimulatingInEditor)return;
	}
#endif
	if (NeedToSortPrimitive)
	{
		HudPrimitiveArray.Sort([](ILGUIHudPrimitive& A, ILGUIHudPrimitive& B)
		{
			return A.GetRenderPriority() < B.GetRenderPriority();
		});
		NeedToSortPrimitive = false;
	}

	FTexture2DRHIRef RenderTarget = InView.Family->RenderTarget->GetRenderTargetTexture();
	FRHIRenderPassInfo RPInfo(RenderTarget, ERenderTargetActions::Load_Store);
	RHICmdList.BeginRenderPass(RPInfo, TEXT("LGUIHudRender"));
	const FSceneViewFamily* ViewFamily = InView.Family;
	const FScene* Scene = nullptr;
	if (ViewFamily->Scene)
	{
		Scene = ViewFamily->Scene->GetRenderScene();
	}

	InView.SceneViewInitOptions.ViewOrigin = ViewLocation;
	InView.SceneViewInitOptions.ViewRotationMatrix = ViewRotationMatrix;
	InView.UpdateProjectionMatrix(ProjectionMatrix);

	FViewUniformShaderParameters viewUniformShaderParameters;
	InView.SetupCommonViewUniformBufferParameters(
		viewUniformShaderParameters,
		InView.UnscaledViewRect.Size(),
		1,
		InView.UnscaledViewRect,
		InView.ViewMatrices,
		FViewMatrices()
	);

	InView.ViewUniformBuffer = TUniformBufferRef<FViewUniformShaderParameters>::CreateUniformBufferImmediate(viewUniformShaderParameters, UniformBuffer_SingleFrame);


	FLGUIMeshElementCollector meshCollector(InView.GetFeatureLevel());
	FGraphicsPipelineStateInitializer GraphicsPSOInit;
	RHICmdList.ApplyCachedRenderTargets(GraphicsPSOInit);
	GraphicsPSOInit.DepthStencilState = TStaticDepthStencilState<false, ECompareFunction::CF_Always>::GetRHI();
	GraphicsPSOInit.RasterizerState = TStaticRasterizerState<FM_Solid, CM_None, false>::GetRHI();
	for (int i = 0; i < HudPrimitiveArray.Num(); i++)
	{
		auto hudPrimitive = HudPrimitiveArray[i];
		if (hudPrimitive != nullptr && hudPrimitive->CanRender())
		{
			const FMeshBatch& Mesh = hudPrimitive->GetMeshElement((FMeshElementCollector*)&meshCollector);
			if (Mesh.VertexFactory != nullptr && Mesh.VertexFactory->IsInitialized())
			{
				auto Material = Mesh.MaterialRenderProxy->GetMaterial(InView.GetFeatureLevel());
				const FMaterialShaderMap* MaterialShaderMap = Material->GetRenderingThreadShaderMap();
				FLGUIHudRenderVS* VertexShader = MaterialShaderMap->GetShader<FLGUIHudRenderVS>(0);
				FLGUIHudRenderPS* PixelShader = MaterialShaderMap->GetShader<FLGUIHudRenderPS>(0);
				VertexShader->SetMatrix(RHICmdList, ViewProjectionMatrix, hudPrimitive->GetObject2WorldMatrix());
				VertexShader->SetMaterialShaderParameters(RHICmdList, InView, Mesh.MaterialRenderProxy, Material);
				PixelShader->SetBlendState(GraphicsPSOInit, Material);
				PixelShader->SetParameters(RHICmdList, InView, Mesh.MaterialRenderProxy, Material);
				GraphicsPSOInit.BoundShaderState.VertexDeclarationRHI = GLGUIVertexDeclaration.VertexDeclarationRHI;
				GraphicsPSOInit.BoundShaderState.VertexShaderRHI = GETSAFERHISHADER_VERTEX(VertexShader);
				GraphicsPSOInit.BoundShaderState.PixelShaderRHI = GETSAFERHISHADER_PIXEL(PixelShader);
				GraphicsPSOInit.PrimitiveType = EPrimitiveType::PT_TriangleList;

				SetGraphicsPipelineState(RHICmdList, GraphicsPSOInit);
				RHICmdList.SetStreamSource(0, hudPrimitive->GetVertexBufferRHI(), 0);
				RHICmdList.DrawIndexedPrimitive(Mesh.Elements[0].IndexBuffer->IndexBufferRHI, 0, 0, hudPrimitive->GetNumVerts(), 0, Mesh.GetNumPrimitives(), 1);
			}
		}
	}
	/*FMeshPassProcessorRenderState drawRenderState(InView);
	drawRenderState.SetDepthStencilState(TStaticDepthStencilState<false, ECompareFunction::CF_Always>::GetRHI());
	drawRenderState.SetViewUniformBuffer(InView.ViewUniformBuffer);
	FLGUIMeshElementCollector meshCollector(InView.GetFeatureLevel());
	for (int i = 0; i < HudPrimitiveArray.Num(); i++)
	{
		auto hudPrimitive = HudPrimitiveArray[i];
		if (hudPrimitive != nullptr && hudPrimitive->CanRender())
		{
			const FMeshBatch& Mesh = hudPrimitive->GetMeshElement((FMeshElementCollector*)&meshCollector);
			if (Mesh.VertexFactory != nullptr && Mesh.VertexFactory->IsInitialized())
			{
				auto primitiveSceneProxy = hudPrimitive->GetPrimitiveSceneProxy();
				DrawDynamicMeshPass(InView, RHICmdList,
					[Scene, &InView, &drawRenderState, &Mesh, primitiveSceneProxy](FMeshPassDrawListContext* InDrawListContext)
				{
					FLGUIHudMeshProcessor meshProcessor(
						Scene,
						InView.GetFeatureLevel(),
						&InView,
						drawRenderState,
						InDrawListContext
					);
					const uint64 DefaultBatchElementMask = ~0ull;
					meshProcessor.AddMeshBatch(Mesh, DefaultBatchElementMask, primitiveSceneProxy);
				});
			}
		}
	}*/
	RHICmdList.EndRenderPass();
}
void FLGUIViewExtension::PreRenderView_RenderThread(FRHICommandListImmediate& RHICmdList, FSceneView& InView)
{

}

void FLGUIViewExtension::AddHudPrimitive(ILGUIHudPrimitive* InPrimitive)
{
	if (InPrimitive == nullptr)
	{
		UE_LOG(LGUI, Error, TEXT("[FLGUIViewExtension::AddHudPrimitive]Add nullptr as ILGUIHudPrimitive!"));
		return;
	}
	HudPrimitiveArray.Add(InPrimitive);
}
void FLGUIViewExtension::RemoveHudPrimitive(ILGUIHudPrimitive* InPrimitive)
{
	if (InPrimitive != nullptr)
	{
		HudPrimitiveArray.Remove(InPrimitive);
	}
}

void FLGUIViewExtension::MarkSortRenderPriority_RenderThread()
{
	NeedToSortPrimitive = true;
	HudPrimitiveArray.Sort([](ILGUIHudPrimitive& A, ILGUIHudPrimitive& B)
	{
		return A.GetRenderPriority() < B.GetRenderPriority();
	});
}
void FLGUIViewExtension::SortRenderPriority()
{
	FLGUIViewExtension* viewExtension = this;
	ENQUEUE_RENDER_COMMAND(FLGUIRender_SortRenderPriority)(
		[viewExtension](FRHICommandListImmediate& RHICmdList)
	{
		viewExtension->MarkSortRenderPriority_RenderThread();
	}
	);
}