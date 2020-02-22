﻿// Copyright 2019-2020 LexLiu. All Rights Reserved.

#include "Core/ActorComponent/UIBackgroundBlur.h"
#include "LGUI.h"
#include "Core/UIGeometry.h"
#include "Core/LGUISpriteData.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Core/Render/LGUIHudShaders.h"
#include "Core/Render/LGUIHudVertex.h"
#include "PipelineStateCache.h"
#include "Core/Render/LGUIRenderer.h"
#include "Core/ActorComponent/LGUICanvas.h"

UUIBackgroundBlur::UUIBackgroundBlur(const FObjectInitializer& ObjectInitializer) :Super(ObjectInitializer)
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UUIBackgroundBlur::BeginPlay()
{
	Super::BeginPlay();

	if (copyRegionVertexArray.Num() == 0)
	{
		//full screen vertex position
		copyRegionVertexArray =
		{
			FLGUIPostProcessVertex(FVector(-1, -1, 0), FVector2D(0.0f, 0.0f)),
			FLGUIPostProcessVertex(FVector(1, -1, 0), FVector2D(1.0f, 0.0f)),
			FLGUIPostProcessVertex(FVector(-1, 1, 0), FVector2D(0.0f, 1.0f)),
			FLGUIPostProcessVertex(FVector(1, 1, 0), FVector2D(1.0f, 1.0f))
		};
	}

	inv_SampleLevelInterval = 1.0f / (MAX_BlurStrength / maxDownSampleLevel);
}

void UUIBackgroundBlur::TickComponent( float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction )
{
	Super::TickComponent( DeltaTime, TickType, ThisTickFunction );
}


#if WITH_EDITOR
void UUIBackgroundBlur::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	if (auto Property = PropertyChangedEvent.Property)
	{
		if (Property->GetFName() == GET_MEMBER_NAME_CHECKED(UUIBackgroundBlur, maxDownSampleLevel))
		{
			maxDownSampleLevel += 1;
			SetMaxDownSampleLevel(maxDownSampleLevel - 1);
		}
	}
}
#endif

void UUIBackgroundBlur::OnCreateGeometry()
{
	UIGeometry::FromUIRectSimple(widget.width, widget.height, widget.pivot, GetFinalColor(), geometry, FLGUISpriteInfo(), RenderCanvas, this);
}
void UUIBackgroundBlur::OnUpdateGeometry(bool InVertexPositionChanged, bool InVertexUVChanged, bool InVertexColorChanged)
{
	if (InVertexPositionChanged)
	{
		UIGeometry::UpdateUIRectSimpleVertex(geometry, widget.width, widget.height, widget.pivot, RenderCanvas, this);
	}
	if (InVertexUVChanged)
	{
		UIGeometry::UpdateUIRectSimpleUV(geometry, FLGUISpriteInfo());
	}
	if (InVertexColorChanged)
	{
		UIGeometry::UpdateUIColor(geometry, GetFinalColor());
	}
}

void UUIBackgroundBlur::SetBlurStrength(float newValue)
{
	if (blurStrength != newValue)
	{
		blurStrength = newValue;
	}
}

void UUIBackgroundBlur::SetMaxDownSampleLevel(int newValue)
{
	if (maxDownSampleLevel != newValue)
	{
		maxDownSampleLevel = newValue;
		inv_SampleLevelInterval = 1.0f / (MAX_BlurStrength / maxDownSampleLevel);
	}
}

float UUIBackgroundBlur::GetBlurStrengthInternal()
{
	if (applyAlphaToBlur)
	{
		return GetFinalAlpha01() * blurStrength;
	}
	return blurStrength;
}

void UUIBackgroundBlur::OnBeforeRenderPostProcess_GameThread(FSceneViewFamily& InViewFamily, FSceneView& InView)
{
	auto blurStrengthWithAlpha = GetBlurStrengthInternal();
	if (blurStrengthWithAlpha <= 0.0f)return;
	if (!IsValid(blurEffectRenderTarget1))
	{
		float width = widget.width;
		float height = widget.height;

		blurEffectRenderTarget1 = NewObject<UTextureRenderTarget2D>(this);
		blurEffectRenderTarget1->InitAutoFormat((int)width, (int)height);

		blurEffectRenderTarget2 = NewObject<UTextureRenderTarget2D>(this);
		blurEffectRenderTarget2->InitAutoFormat((int)width, (int)height);

		inv_TextureSize.X = 1.0f / width;
		inv_TextureSize.Y = 1.0f / height;
	}
	else
	{
		if (blurEffectRenderTarget1->SizeX != widget.width || blurEffectRenderTarget1->SizeY != widget.height)
		{
			float width = widget.width;
			float height = widget.height;

			blurEffectRenderTarget1->ResizeTarget((int)width, (int)height);
			blurEffectRenderTarget2->ResizeTarget((int)width, (int)height);

			inv_TextureSize.X = 1.0f / width;
			inv_TextureSize.Y = 1.0f / height;
		}
	}

	auto modelViewPrjectionMatrix = RenderCanvas->GetRootCanvas()->GetViewProjectionMatrix();
	auto& vertices = geometry->vertices;
	{
		FScopeLock scopeLock(&mutex);

		for (int i = 0; i < 4; i++)
		{
			auto& copyVert = copyRegionVertexArray[i];
			//convert vertex postition to screen, and use as texture coordinate
			auto clipSpacePos = modelViewPrjectionMatrix.TransformPosition(vertices[i].Position);
			copyVert.TextureCoordinate0 = FVector2D(clipSpacePos.X / clipSpacePos.W, clipSpacePos.Y / clipSpacePos.W) * 0.5f + FVector2D(0.5f, 0.5f);
		}
	}
}

DECLARE_CYCLE_STAT(TEXT("PostProcess_BackgroundBlur"), STAT_BackgroundBlur, STATGROUP_LGUI);

void UUIBackgroundBlur::OnRenderPostProcess_RenderThread(
	FRHICommandListImmediate& RHICmdList, 
	FTexture2DRHIRef ScreenImage, 
	TShaderMap<FGlobalShaderType>* GlobalShaderMap, 
	const FMatrix& ViewProjectionMatrix,  
	const TFunction<void()>& DrawPrimitive
)
{
	SCOPE_CYCLE_COUNTER(STAT_BackgroundBlur);
	auto blurStrengthWithAlpha = GetBlurStrengthInternal();
	if (blurStrengthWithAlpha <= 0.0f)return;
	if (!IsValid(blurEffectRenderTarget1))return;
	if (!IsValid(blurEffectRenderTarget2))return;

	auto Resource1 = blurEffectRenderTarget1->GetRenderTargetResource();
	auto Resource2 = blurEffectRenderTarget2->GetRenderTargetResource();
	if (Resource1 == nullptr || Resource2 == nullptr)return;
	FTexture2DRHIRef BlurEffectRenderTexture1 = Resource1->GetRenderTargetTexture();
	FTexture2DRHIRef BlurEffectRenderTexture2 = Resource2->GetRenderTargetTexture();
	//copy rect area from screen image to a render target, so we can just process this area
	{
		TArray<FLGUIPostProcessVertex> tempCopyRegion;
		{
			FScopeLock scopeLock(&mutex);
			tempCopyRegion = copyRegionVertexArray;
		}
		FLGUIViewExtension::CopyRenderTargetOnMeshRegion(RHICmdList, GlobalShaderMap, ScreenImage, BlurEffectRenderTexture1, true, tempCopyRegion);//mesh's uv.y is flipped
	}
	//do the blur process on the area
	{
		TShaderMapRef<FLGUISimplePostProcessVS> VertexShader(GlobalShaderMap);
		TShaderMapRef<FLGUIPostProcessGaussianBlurPS> PixelShader(GlobalShaderMap);
		FGraphicsPipelineStateInitializer GraphicsPSOInit;
		RHICmdList.ApplyCachedRenderTargets(GraphicsPSOInit);
		GraphicsPSOInit.DepthStencilState = TStaticDepthStencilState<false, ECompareFunction::CF_Always>::GetRHI();
		GraphicsPSOInit.RasterizerState = TStaticRasterizerState<FM_Solid, CM_None, false>::GetRHI();
		GraphicsPSOInit.BlendState = TStaticBlendState<>::GetRHI();
		GraphicsPSOInit.BoundShaderState.VertexDeclarationRHI = GetLGUIPostProcessVertexDeclaration();
		GraphicsPSOInit.BoundShaderState.VertexShaderRHI = GETSAFERHISHADER_VERTEX(*VertexShader);
		GraphicsPSOInit.BoundShaderState.PixelShaderRHI = GETSAFERHISHADER_PIXEL(*PixelShader);
		GraphicsPSOInit.PrimitiveType = EPrimitiveType::PT_TriangleList;
		SetGraphicsPipelineState(RHICmdList, GraphicsPSOInit);
		VertexShader->SetParameters(RHICmdList, false);

		auto samplerState = TStaticSamplerState<SF_Bilinear, AM_Clamp, AM_Clamp, AM_Clamp>::GetRHI();
		PixelShader->SetInverseTextureSize(RHICmdList, inv_TextureSize);
		blurStrengthWithAlpha = FMath::Pow(blurStrengthWithAlpha * INV_MAX_BlurStrength, 0.5f) * MAX_BlurStrength;//this can make the blur effect transition feel more linear
		float calculatedBlurStrength = blurStrengthWithAlpha * inv_SampleLevelInterval;
		float calculatedBlurStrength2 = 1.0f;
		int sampleCount = (int)calculatedBlurStrength + 1;
		for (int i = 0; i < sampleCount; i++)
		{
			if (i + 1 == sampleCount)
			{
				float fracValue = (calculatedBlurStrength - (int)calculatedBlurStrength);
				fracValue = FMath::FastAsin(fracValue * 2.0f - 1.0f) / PI + 0.5f;//another thing to make the blur transition feel more linear
				PixelShader->SetBlurStrength(RHICmdList, calculatedBlurStrength2 * fracValue);
			}
			else
			{
				PixelShader->SetBlurStrength(RHICmdList, calculatedBlurStrength2);
			}
			//render horizontal
			RHICmdList.BeginRenderPass(FRHIRenderPassInfo(BlurEffectRenderTexture2, ERenderTargetActions::Load_Store), TEXT("Vertical"));
			PixelShader->SetMainTexture(RHICmdList, BlurEffectRenderTexture1, samplerState);
			PixelShader->SetHorizontalOrVertical(RHICmdList, true);
			FLGUIViewExtension::DrawFullScreenQuad(RHICmdList);
			RHICmdList.EndRenderPass();
			//render horizontal
			RHICmdList.BeginRenderPass(FRHIRenderPassInfo(BlurEffectRenderTexture1, ERenderTargetActions::Load_Store), TEXT("Horizontal"));
			PixelShader->SetMainTexture(RHICmdList, BlurEffectRenderTexture2, samplerState);
			PixelShader->SetHorizontalOrVertical(RHICmdList, false);
			FLGUIViewExtension::DrawFullScreenQuad(RHICmdList);
			RHICmdList.EndRenderPass();
			calculatedBlurStrength2 *= 2;
		}
	}
	//after blur process, copy the area back to screen image
	{
		RHICmdList.BeginRenderPass(FRHIRenderPassInfo(ScreenImage, ERenderTargetActions::Load_Store), TEXT("CopyAreaToScreen"));
		TShaderMapRef<FLGUIMeshPostProcessVS> VertexShader(GlobalShaderMap);
		TShaderMapRef<FLGUIMeshCopyTargetPS> PixelShader(GlobalShaderMap);
		FGraphicsPipelineStateInitializer GraphicsPSOInit;
		RHICmdList.ApplyCachedRenderTargets(GraphicsPSOInit);
		GraphicsPSOInit.DepthStencilState = TStaticDepthStencilState<false, ECompareFunction::CF_Always>::GetRHI();
		GraphicsPSOInit.RasterizerState = TStaticRasterizerState<FM_Solid, CM_None, false>::GetRHI();
		GraphicsPSOInit.BlendState = TStaticBlendState<>::GetRHI();
		GraphicsPSOInit.BoundShaderState.VertexDeclarationRHI = GetLGUIVertexDeclaration();
		GraphicsPSOInit.BoundShaderState.VertexShaderRHI = GETSAFERHISHADER_VERTEX(*VertexShader);
		GraphicsPSOInit.BoundShaderState.PixelShaderRHI = GETSAFERHISHADER_PIXEL(*PixelShader);
		GraphicsPSOInit.PrimitiveType = EPrimitiveType::PT_TriangleList;
		SetGraphicsPipelineState(RHICmdList, GraphicsPSOInit);
		VertexShader->SetParameters(RHICmdList, ViewProjectionMatrix);
		PixelShader->SetParameters(RHICmdList, BlurEffectRenderTexture1);
		DrawPrimitive();
		RHICmdList.EndRenderPass();
	}
}