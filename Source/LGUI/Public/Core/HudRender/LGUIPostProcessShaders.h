﻿// Copyright 2019-2021 LexLiu. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Shader.h"
#include "ShaderParameterUtils.h"
#include "MaterialShaderType.h"
#include "MaterialShader.h"
#include "Engine/Texture2D.h"

class FLGUIPostProcessShader :public FGlobalShader
{
public:
	FLGUIPostProcessShader() {}
	FLGUIPostProcessShader(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		:FGlobalShader(Initializer)
	{

	}
	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return true;
	}
	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);
	}
	static bool ShouldCache(EShaderPlatform Platform)
	{
		return true;
	}
};
class FLGUISimplePostProcessVS :public FLGUIPostProcessShader
{
	DECLARE_SHADER_TYPE(FLGUISimplePostProcessVS, Global);
public:
	FLGUISimplePostProcessVS() {}
	FLGUISimplePostProcessVS(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		: FLGUIPostProcessShader(Initializer)
	{

	}
	void SetParameters(FRHICommandListImmediate& RHICmdList)
	{

	}
private:
};
class FLGUISimpleCopyTargetPS :public FLGUIPostProcessShader
{
	DECLARE_SHADER_TYPE(FLGUISimpleCopyTargetPS, Global);
public:
	FLGUISimpleCopyTargetPS() {}
	FLGUISimpleCopyTargetPS(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		: FLGUIPostProcessShader(Initializer)
	{
		MainTextureParameter.Bind(Initializer.ParameterMap, TEXT("_MainTex"));
		MainTextureSamplerParameter.Bind(Initializer.ParameterMap, TEXT("_MainTexSampler"));
	}
	void SetParameters(FRHICommandListImmediate& RHICmdList, FTextureRHIRef SceneTexture, FRHISamplerState* SceneTextureSampler = TStaticSamplerState<SF_Bilinear, AM_Clamp, AM_Clamp, AM_Clamp>::GetRHI())
	{
		SetTextureParameter(RHICmdList, RHICmdList.GetBoundPixelShader(), MainTextureParameter, MainTextureSamplerParameter, SceneTextureSampler, SceneTexture);
	}
private:
	LAYOUT_FIELD(FShaderResourceParameter, MainTextureParameter);
	LAYOUT_FIELD(FShaderResourceParameter, MainTextureSamplerParameter);
};
class FLGUIPostProcessGaussianBlurPS :public FLGUIPostProcessShader
{
	DECLARE_SHADER_TYPE(FLGUIPostProcessGaussianBlurPS, Global);
public:
	FLGUIPostProcessGaussianBlurPS() {}
	FLGUIPostProcessGaussianBlurPS(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		: FLGUIPostProcessShader(Initializer)
	{
		MainTextureParameter.Bind(Initializer.ParameterMap, TEXT("_MainTex"));
		MainTextureSamplerParameter.Bind(Initializer.ParameterMap, TEXT("_MainTexSampler"));
		HorizontalOrVerticalFilterParameter.Bind(Initializer.ParameterMap, TEXT("_HorizontalOrVerticalFilter"));
		InvSizeParameter.Bind(Initializer.ParameterMap, TEXT("_InvSize"));
		BlurStrengthParameter.Bind(Initializer.ParameterMap, TEXT("_BlurStrength"));
	}
	void SetMainTexture(FRHICommandListImmediate& RHICmdList, FTextureRHIRef MainTexture, FRHISamplerState* MainTextureSampler)
	{
		SetTextureParameter(RHICmdList, RHICmdList.GetBoundPixelShader(), MainTextureParameter, MainTextureSamplerParameter, MainTextureSampler, MainTexture);
	}
	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
	{
		FLGUIPostProcessShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);
	}
	void SetInverseTextureSize(FRHICommandListImmediate& RHICmdList, const FVector2D& InvSize)
	{
		SetShaderValue(RHICmdList, RHICmdList.GetBoundPixelShader(), InvSizeParameter, InvSize);
	}
	void SetBlurStrength(FRHICommandListImmediate& RHICmdList, float BlurStrength)
	{
		SetShaderValue(RHICmdList, RHICmdList.GetBoundPixelShader(), BlurStrengthParameter, BlurStrength);
	}
	void SetHorizontalOrVertical(FRHICommandListImmediate& RHICmdList, bool HorizontalOrVertical)
	{
		SetShaderValue(RHICmdList, RHICmdList.GetBoundPixelShader(), HorizontalOrVerticalFilterParameter, HorizontalOrVertical ? FVector2D(1.0f, 0.0f) : FVector2D(0.0f, 1.0f));
	}
private:
	LAYOUT_FIELD(FShaderResourceParameter, MainTextureParameter);
	LAYOUT_FIELD(FShaderResourceParameter, MainTextureSamplerParameter);
	LAYOUT_FIELD(FShaderParameter, HorizontalOrVerticalFilterParameter);
	LAYOUT_FIELD(FShaderParameter, InvSizeParameter);
	LAYOUT_FIELD(FShaderParameter, BlurStrengthParameter);
};
class FLGUIPostProcessGaussianBlurWithStrengthTexturePS :public FLGUIPostProcessGaussianBlurPS
{
	DECLARE_SHADER_TYPE(FLGUIPostProcessGaussianBlurWithStrengthTexturePS, Global);
public:
	FLGUIPostProcessGaussianBlurWithStrengthTexturePS() {}
	FLGUIPostProcessGaussianBlurWithStrengthTexturePS(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		: FLGUIPostProcessGaussianBlurPS(Initializer)
	{
		StrengthTextureParameter.Bind(Initializer.ParameterMap, TEXT("_StrengthTex"));
		StrengthTextureSamplerParameter.Bind(Initializer.ParameterMap, TEXT("_StrengthTexSampler"));
	}
	void SetStrengthTexture(FRHICommandListImmediate& RHICmdList, FTextureRHIRef StrengthTexture, FRHISamplerState* StrengthTextureSampler)
	{
		SetTextureParameter(RHICmdList, RHICmdList.GetBoundPixelShader(), StrengthTextureParameter, StrengthTextureSamplerParameter, StrengthTextureSampler, StrengthTexture);
	}
	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
	{
		OutEnvironment.SetDefine(TEXT("USE_STRENGTH_TEXTURE"), 1);
		FLGUIPostProcessGaussianBlurPS::ModifyCompilationEnvironment(Parameters, OutEnvironment);
	}
private:
	LAYOUT_FIELD(FShaderResourceParameter, StrengthTextureParameter);
	LAYOUT_FIELD(FShaderResourceParameter, StrengthTextureSamplerParameter);
};

class FLGUIPostProcessCustomDepthMaskPS :public FLGUIPostProcessShader
{
	DECLARE_SHADER_TYPE(FLGUIPostProcessCustomDepthMaskPS, Global);
public:
	FLGUIPostProcessCustomDepthMaskPS() {}
	FLGUIPostProcessCustomDepthMaskPS(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		: FLGUIPostProcessShader(Initializer)
	{
		_ScreenTex.Bind(Initializer.ParameterMap, TEXT("_ScreenTex"));
		_ScreenTexSampler.Bind(Initializer.ParameterMap, TEXT("_ScreenTexSampler"));
		_OriginScreenTex.Bind(Initializer.ParameterMap, TEXT("_OriginScreenTex"));
		_OriginScreenTexSampler.Bind(Initializer.ParameterMap, TEXT("_OriginScreenTexSampler"));
		_CustomDepthTex.Bind(Initializer.ParameterMap, TEXT("_CustomDepthTex"));
		_CustomDepthTexSampler.Bind(Initializer.ParameterMap, TEXT("_CustomDepthTexSampler"));
		_MaskStrength.Bind(Initializer.ParameterMap, TEXT("_MaskStrength"));
	}
	void SetParameters(FRHICommandListImmediate& RHICmdList
		, FTextureRHIRef ScreenTexture, FRHISamplerState* ScreenTextureSampler
		, FTextureRHIRef OriginScreenTexture, FRHISamplerState* OriginScreenTextureSampler
		, FTextureRHIRef CustomDepthTexture, FRHISamplerState* CustomDepthTextureSampler
		, float MaskStrength
	)
	{
		SetTextureParameter(RHICmdList, RHICmdList.GetBoundPixelShader(), _ScreenTex, _ScreenTexSampler, ScreenTextureSampler, ScreenTexture);
		SetTextureParameter(RHICmdList, RHICmdList.GetBoundPixelShader(), _OriginScreenTex, _OriginScreenTexSampler, OriginScreenTextureSampler, OriginScreenTexture);
		SetTextureParameter(RHICmdList, RHICmdList.GetBoundPixelShader(), _CustomDepthTex, _CustomDepthTexSampler, CustomDepthTextureSampler, CustomDepthTexture);
		SetShaderValue(RHICmdList, RHICmdList.GetBoundPixelShader(), _MaskStrength, MaskStrength);
	}
	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
	{
		FLGUIPostProcessShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);
	}
private:
	LAYOUT_FIELD(FShaderResourceParameter, _ScreenTex);
	LAYOUT_FIELD(FShaderResourceParameter, _ScreenTexSampler);
	LAYOUT_FIELD(FShaderResourceParameter, _OriginScreenTex);
	LAYOUT_FIELD(FShaderResourceParameter, _OriginScreenTexSampler);
	LAYOUT_FIELD(FShaderResourceParameter, _CustomDepthTex);
	LAYOUT_FIELD(FShaderResourceParameter, _CustomDepthTexSampler);
	LAYOUT_FIELD(FShaderParameter, _MaskStrength);
};
class FLGUIPostProcessCustomDepthStencilMaskPS :public FLGUIPostProcessShader
{
	DECLARE_SHADER_TYPE(FLGUIPostProcessCustomDepthStencilMaskPS, Global);
public:
	FLGUIPostProcessCustomDepthStencilMaskPS() {}
	FLGUIPostProcessCustomDepthStencilMaskPS(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		: FLGUIPostProcessShader(Initializer)
	{
		_ScreenTex.Bind(Initializer.ParameterMap, TEXT("_ScreenTex"));
		_ScreenTexSampler.Bind(Initializer.ParameterMap, TEXT("_ScreenTexSampler"));
		_OriginScreenTex.Bind(Initializer.ParameterMap, TEXT("_OriginScreenTex"));
		_OriginScreenTexSampler.Bind(Initializer.ParameterMap, TEXT("_OriginScreenTexSampler"));
		_CustomStencilTex.Bind(Initializer.ParameterMap, TEXT("_CustomStencilTex"));
		_StencilValue.Bind(Initializer.ParameterMap, TEXT("_StencilValue"));
		_TextureSize.Bind(Initializer.ParameterMap, TEXT("_TextureSize"));
	}
	void SetParameters(FRHICommandListImmediate& RHICmdList
		, FTextureRHIRef ScreenTexture, FRHISamplerState* ScreenTextureSampler
		, FTextureRHIRef OriginScreenTexture, FRHISamplerState* OriginScreenTextureSampler
		, FRHIShaderResourceView* StencilResourceView
		, int StencilValue
		, int screenWidth, int screenHeight
	)
	{
		SetTextureParameter(RHICmdList, RHICmdList.GetBoundPixelShader(), _ScreenTex, _ScreenTexSampler, ScreenTextureSampler, ScreenTexture);
		SetTextureParameter(RHICmdList, RHICmdList.GetBoundPixelShader(), _OriginScreenTex, _OriginScreenTexSampler, OriginScreenTextureSampler, OriginScreenTexture);
		SetSRVParameter(RHICmdList, RHICmdList.GetBoundPixelShader(), _CustomStencilTex, StencilResourceView);
		SetShaderValue(RHICmdList, RHICmdList.GetBoundPixelShader(), _StencilValue, StencilValue);
		SetShaderValue(RHICmdList, RHICmdList.GetBoundPixelShader(), _TextureSize, FIntVector(screenWidth, screenHeight, 0));
	}
	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
	{
		FLGUIPostProcessShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);
	}
private:
	LAYOUT_FIELD(FShaderResourceParameter, _ScreenTex);
	LAYOUT_FIELD(FShaderResourceParameter, _ScreenTexSampler);
	LAYOUT_FIELD(FShaderResourceParameter, _OriginScreenTex);
	LAYOUT_FIELD(FShaderResourceParameter, _OriginScreenTexSampler);
	LAYOUT_FIELD(FShaderResourceParameter, _CustomStencilTex);
	LAYOUT_FIELD(FShaderParameter, _StencilValue);
	LAYOUT_FIELD(FShaderParameter, _TextureSize);
};
class FLGUIPostProcessMobileCustomDepthStencilMaskPS :public FLGUIPostProcessShader
{
	DECLARE_SHADER_TYPE(FLGUIPostProcessMobileCustomDepthStencilMaskPS, Global);
public:
	FLGUIPostProcessMobileCustomDepthStencilMaskPS() {}
	FLGUIPostProcessMobileCustomDepthStencilMaskPS(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		: FLGUIPostProcessShader(Initializer)
	{
		_ScreenTex.Bind(Initializer.ParameterMap, TEXT("_ScreenTex"));
		_ScreenTexSampler.Bind(Initializer.ParameterMap, TEXT("_ScreenTexSampler"));
		_OriginScreenTex.Bind(Initializer.ParameterMap, TEXT("_OriginScreenTex"));
		_OriginScreenTexSampler.Bind(Initializer.ParameterMap, TEXT("_OriginScreenTexSampler"));
		_MobileCustomStencilTex.Bind(Initializer.ParameterMap, TEXT("_MobileCustomStencilTex"));
		_MobileCustomStencilTexSampler.Bind(Initializer.ParameterMap, TEXT("_MobileCustomStencilTexSampler"));
		_StencilValue.Bind(Initializer.ParameterMap, TEXT("_StencilValue"));
	}
	void SetParameters(FRHICommandListImmediate& RHICmdList
		, FTextureRHIRef ScreenTexture, FRHISamplerState* ScreenTextureSampler
		, FTextureRHIRef OriginScreenTexture, FRHISamplerState* OriginScreenTextureSampler
		, FTextureRHIRef CustomDepthTexture, FRHISamplerState* CustomDepthTextureSampler
		, int StencilValue
	)
	{
		SetTextureParameter(RHICmdList, RHICmdList.GetBoundPixelShader(), _ScreenTex, _ScreenTexSampler, ScreenTextureSampler, ScreenTexture);
		SetTextureParameter(RHICmdList, RHICmdList.GetBoundPixelShader(), _OriginScreenTex, _OriginScreenTexSampler, OriginScreenTextureSampler, OriginScreenTexture);
		SetTextureParameter(RHICmdList, RHICmdList.GetBoundPixelShader(), _MobileCustomStencilTex, _MobileCustomStencilTexSampler, CustomDepthTextureSampler, CustomDepthTexture);
		SetShaderValue(RHICmdList, RHICmdList.GetBoundPixelShader(), _StencilValue, StencilValue);
	}
	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
	{
		FLGUIPostProcessShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);
	}
private:
	LAYOUT_FIELD(FShaderResourceParameter, _ScreenTex);
	LAYOUT_FIELD(FShaderResourceParameter, _ScreenTexSampler);
	LAYOUT_FIELD(FShaderResourceParameter, _OriginScreenTex);
	LAYOUT_FIELD(FShaderResourceParameter, _OriginScreenTexSampler);
	LAYOUT_FIELD(FShaderResourceParameter, _MobileCustomStencilTex);
	LAYOUT_FIELD(FShaderResourceParameter, _MobileCustomStencilTexSampler);
	LAYOUT_FIELD(FShaderParameter, _StencilValue);
};





//render mesh region 
class FLGUICopyMeshRegionVS :public FLGUIPostProcessShader
{
	DECLARE_SHADER_TYPE(FLGUICopyMeshRegionVS, Global);
public:
	FLGUICopyMeshRegionVS() {}
	FLGUICopyMeshRegionVS(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		: FLGUIPostProcessShader(Initializer)
	{
		
	}
};

//render mesh pixel shader
class FLGUICopyMeshRegionPS :public FLGUIPostProcessShader
{
	DECLARE_SHADER_TYPE(FLGUICopyMeshRegionPS, Global);
public:
	FLGUICopyMeshRegionPS() {}
	FLGUICopyMeshRegionPS(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		: FLGUIPostProcessShader(Initializer)
	{
		MainTextureParameter.Bind(Initializer.ParameterMap, TEXT("_MainTex"));
		MainTextureSamplerParameter.Bind(Initializer.ParameterMap, TEXT("_MainTexSampler"));
		MVPParameter.Bind(Initializer.ParameterMap, TEXT("_MVP"));
	}
	void SetParameters(FRHICommandListImmediate& RHICmdList, const FMatrix& MVP, FTextureRHIRef MainTexture, FRHISamplerState* MainTextureSampler = TStaticSamplerState<SF_Bilinear, AM_Clamp, AM_Clamp, AM_Clamp>::GetRHI())
	{
		SetTextureParameter(RHICmdList, RHICmdList.GetBoundPixelShader(), MainTextureParameter, MainTextureSamplerParameter, MainTextureSampler, MainTexture);
		SetShaderValue(RHICmdList, RHICmdList.GetBoundPixelShader(), MVPParameter, MVP);
	}
private:
	LAYOUT_FIELD(FShaderResourceParameter, MainTextureParameter);
	LAYOUT_FIELD(FShaderResourceParameter, MainTextureSamplerParameter);
	LAYOUT_FIELD(FShaderParameter, MVPParameter);
};




//common render mesh vertex shader
class FLGUIRenderMeshVS :public FLGUIPostProcessShader
{
	DECLARE_SHADER_TYPE(FLGUIRenderMeshVS, Global);
public:
	FLGUIRenderMeshVS() {}
	FLGUIRenderMeshVS(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		: FLGUIPostProcessShader(Initializer)
	{
		MVPParameter.Bind(Initializer.ParameterMap, TEXT("_MVP"));
	}
	void SetParameters(FRHICommandListImmediate& RHICmdList, const FMatrix& MVP)
	{
		SetShaderValue(RHICmdList, RHICmdList.GetBoundVertexShader(), MVPParameter, MVP);
	}
private:
	LAYOUT_FIELD(FShaderParameter, MVPParameter);
};
//render mesh pixel shader
class FLGUIRenderMeshPS :public FLGUIPostProcessShader
{
	DECLARE_SHADER_TYPE(FLGUIRenderMeshPS, Global);
public:
	FLGUIRenderMeshPS() {}
	FLGUIRenderMeshPS(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		: FLGUIPostProcessShader(Initializer)
	{
		MainTextureParameter.Bind(Initializer.ParameterMap, TEXT("_MainTex"));
		MainTextureSamplerParameter.Bind(Initializer.ParameterMap, TEXT("_MainTexSampler"));
	}
	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
	{
		OutEnvironment.SetDefine(TEXT("LGUI_MASK"), 0);
		FLGUIPostProcessShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);
	}
	void SetParameters(FRHICommandListImmediate& RHICmdList, FTextureRHIRef MainTexture, FRHISamplerState* MainTextureSampler = TStaticSamplerState<SF_Bilinear, AM_Clamp, AM_Clamp, AM_Clamp>::GetRHI())
	{
		SetTextureParameter(RHICmdList, RHICmdList.GetBoundPixelShader(), MainTextureParameter, MainTextureSamplerParameter, MainTextureSampler, MainTexture);
	}
private:
	LAYOUT_FIELD(FShaderResourceParameter, MainTextureParameter);
	LAYOUT_FIELD(FShaderResourceParameter, MainTextureSamplerParameter);
};
//render mesh pixel shader, use a mask texture
class FLGUIRenderMeshWithMaskPS :public FLGUIPostProcessShader
{
	DECLARE_SHADER_TYPE(FLGUIRenderMeshWithMaskPS, Global);
public:
	FLGUIRenderMeshWithMaskPS() {}
	FLGUIRenderMeshWithMaskPS(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		: FLGUIPostProcessShader(Initializer)
	{
		MainTextureParameter.Bind(Initializer.ParameterMap, TEXT("_MainTex"));
		MainTextureSamplerParameter.Bind(Initializer.ParameterMap, TEXT("_MainTexSampler"));
		MaskTextureParameter.Bind(Initializer.ParameterMap, TEXT("_MaskTex"));
		MaskTextureSamplerParameter.Bind(Initializer.ParameterMap, TEXT("_MaskTexSampler"));
	}
	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
	{
		OutEnvironment.SetDefine(TEXT("LGUI_MASK"), 1);
		FLGUIPostProcessShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);
	}
	void SetParameters(FRHICommandListImmediate& RHICmdList
		, FTextureRHIRef MainTexture
		, FTextureRHIRef MaskTexture
		, FRHISamplerState* MainTextureSampler = TStaticSamplerState<SF_Bilinear, AM_Clamp, AM_Clamp, AM_Clamp>::GetRHI()
		, FRHISamplerState* MaskTextureSampler = TStaticSamplerState<SF_Bilinear, AM_Clamp, AM_Clamp, AM_Clamp>::GetRHI()
	)
	{
		SetTextureParameter(RHICmdList, RHICmdList.GetBoundPixelShader(), MainTextureParameter, MainTextureSamplerParameter, MainTextureSampler, MainTexture);
		SetTextureParameter(RHICmdList, RHICmdList.GetBoundPixelShader(), MaskTextureParameter, MaskTextureSamplerParameter, MaskTextureSampler, MaskTexture);
	}
private:
	LAYOUT_FIELD(FShaderResourceParameter, MainTextureParameter);
	LAYOUT_FIELD(FShaderResourceParameter, MainTextureSamplerParameter);
	LAYOUT_FIELD(FShaderResourceParameter, MaskTextureParameter);
	LAYOUT_FIELD(FShaderResourceParameter, MaskTextureSamplerParameter);
};

#pragma region RectClip
//render mesh pixel shader
class FLGUIRenderMeshPS_RectClip :public FLGUIRenderMeshPS
{
	DECLARE_SHADER_TYPE(FLGUIRenderMeshPS_RectClip, Global);
public:
	FLGUIRenderMeshPS_RectClip() {}
	FLGUIRenderMeshPS_RectClip(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		: FLGUIRenderMeshPS(Initializer)
	{
		OffsetAndSizeParameter.Bind(Initializer.ParameterMap, TEXT("_RectClipOffsetAndSize"));
		FeatherParameter.Bind(Initializer.ParameterMap, TEXT("_RectClipFeather"));
	}
	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
	{
		OutEnvironment.SetDefine(TEXT("LGUI_RECT_CLIP"), 1);
		FLGUIRenderMeshPS::ModifyCompilationEnvironment(Parameters, OutEnvironment);
	}
	void SetClipParameters(FRHICommandListImmediate& RHICmdList, const FVector4& OffsetAndSize, const FVector4& Feather)
	{
		SetShaderValue(RHICmdList, RHICmdList.GetBoundPixelShader(), OffsetAndSizeParameter, OffsetAndSize);
		SetShaderValue(RHICmdList, RHICmdList.GetBoundPixelShader(), FeatherParameter, Feather);
	}
private:
	LAYOUT_FIELD(FShaderParameter, OffsetAndSizeParameter);
	LAYOUT_FIELD(FShaderParameter, FeatherParameter);
};
//render mesh pixel shader, use a mask texture
class FLGUIRenderMeshWithMaskPS_RectClip :public FLGUIRenderMeshWithMaskPS
{
	DECLARE_SHADER_TYPE(FLGUIRenderMeshWithMaskPS_RectClip, Global);
public:
	FLGUIRenderMeshWithMaskPS_RectClip() {}
	FLGUIRenderMeshWithMaskPS_RectClip(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		: FLGUIRenderMeshWithMaskPS(Initializer)
	{
		OffsetAndSizeParameter.Bind(Initializer.ParameterMap, TEXT("_RectClipOffsetAndSize"));
		FeatherParameter.Bind(Initializer.ParameterMap, TEXT("_RectClipFeather"));
	}
	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
	{
		OutEnvironment.SetDefine(TEXT("LGUI_RECT_CLIP"), 1);
		FLGUIRenderMeshWithMaskPS::ModifyCompilationEnvironment(Parameters, OutEnvironment);
	}
	void SetClipParameters(FRHICommandListImmediate& RHICmdList, const FVector4& OffsetAndSize, const FVector4& Feather)
	{
		SetShaderValue(RHICmdList, RHICmdList.GetBoundPixelShader(), OffsetAndSizeParameter, OffsetAndSize);
		SetShaderValue(RHICmdList, RHICmdList.GetBoundPixelShader(), FeatherParameter, Feather);
	}
private:
	LAYOUT_FIELD(FShaderParameter, OffsetAndSizeParameter);
	LAYOUT_FIELD(FShaderParameter, FeatherParameter);
};
#pragma endregion

#pragma region TextureClip
//render mesh pixel shader
class FLGUIRenderMeshPS_TextureClip :public FLGUIRenderMeshPS
{
	DECLARE_SHADER_TYPE(FLGUIRenderMeshPS_TextureClip, Global);
public:
	FLGUIRenderMeshPS_TextureClip() {}
	FLGUIRenderMeshPS_TextureClip(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		: FLGUIRenderMeshPS(Initializer)
	{
		OffsetAndSizeParameter.Bind(Initializer.ParameterMap, TEXT("_TextureClipOffsetAndSize"));
		ClipTextureParameter.Bind(Initializer.ParameterMap, TEXT("_ClipTex"));
		ClipTextureSamplerParameter.Bind(Initializer.ParameterMap, TEXT("_ClipTexSampler"));
	}
	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
	{
		OutEnvironment.SetDefine(TEXT("LGUI_TEXTURE_CLIP"), 1);
		FLGUIRenderMeshPS::ModifyCompilationEnvironment(Parameters, OutEnvironment);
	}
	void SetClipParameters(FRHICommandListImmediate& RHICmdList, const FVector4& OffsetAndSize
		, FTextureRHIRef ClipTexture
		, FRHISamplerState* ClipTextureSampler = TStaticSamplerState<SF_Bilinear, AM_Clamp, AM_Clamp, AM_Clamp>::GetRHI())
	{
		SetShaderValue(RHICmdList, RHICmdList.GetBoundPixelShader(), OffsetAndSizeParameter, OffsetAndSize);
		SetTextureParameter(RHICmdList, RHICmdList.GetBoundPixelShader(), ClipTextureParameter, ClipTextureSamplerParameter, ClipTextureSampler, ClipTexture);
	}
private:
	LAYOUT_FIELD(FShaderParameter, OffsetAndSizeParameter);
	LAYOUT_FIELD(FShaderResourceParameter, ClipTextureParameter);
	LAYOUT_FIELD(FShaderResourceParameter, ClipTextureSamplerParameter);
};
//render mesh pixel shader, use a mask texture
class FLGUIRenderMeshWithMaskPS_TextureClip :public FLGUIRenderMeshWithMaskPS
{
	DECLARE_SHADER_TYPE(FLGUIRenderMeshWithMaskPS_TextureClip, Global);
public:
	FLGUIRenderMeshWithMaskPS_TextureClip() {}
	FLGUIRenderMeshWithMaskPS_TextureClip(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		: FLGUIRenderMeshWithMaskPS(Initializer)
	{
		OffsetAndSizeParameter.Bind(Initializer.ParameterMap, TEXT("_TextureClipOffsetAndSize"));
		ClipTextureParameter.Bind(Initializer.ParameterMap, TEXT("_ClipTex"));
		ClipTextureSamplerParameter.Bind(Initializer.ParameterMap, TEXT("_ClipTexSampler"));
	}
	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters & Parameters, FShaderCompilerEnvironment & OutEnvironment)
	{
		OutEnvironment.SetDefine(TEXT("LGUI_TEXTURE_CLIP"), 1);
		FLGUIRenderMeshWithMaskPS::ModifyCompilationEnvironment(Parameters, OutEnvironment);
	}
	void SetClipParameters(FRHICommandListImmediate & RHICmdList, const FVector4 & OffsetAndSize
		, FTextureRHIRef ClipTexture
		, FRHISamplerState * ClipTextureSampler = TStaticSamplerState<SF_Bilinear, AM_Clamp, AM_Clamp, AM_Clamp>::GetRHI())
	{
		SetShaderValue(RHICmdList, RHICmdList.GetBoundPixelShader(), OffsetAndSizeParameter, OffsetAndSize);
		SetTextureParameter(RHICmdList, RHICmdList.GetBoundPixelShader(), ClipTextureParameter, ClipTextureSamplerParameter, ClipTextureSampler, ClipTexture);
	}
private:
	LAYOUT_FIELD(FShaderParameter, OffsetAndSizeParameter);
	LAYOUT_FIELD(FShaderResourceParameter, ClipTextureParameter);
	LAYOUT_FIELD(FShaderResourceParameter, ClipTextureSamplerParameter);
};
#pragma endregion