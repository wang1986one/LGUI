﻿// Copyright 2019 LexLiu. All Rights Reserved.

#include "Core/LGUIAtlasData.h"
#include "LGUI.h"
#include "Core/LGUISettings.h"
#include "Core/Actor/LGUIManagerActor.h"
#include "Core/ActorComponent/UISpriteBase.h"
#include "Core/LGUISpriteData.h"


void FLGUIAtlasData::EnsureAtlasTexture(const FName& packingTag)
{
#if WITH_EDITOR
	int32 defaultAtlasTextureSize = ULGUISettings::GetAtlasTextureInitialSize(packingTag);
#else
	static int32 defaultAtlasTextureSize = ULGUISettings::GetAtlasTextureInitialSize(packingTag);
#endif
	if (!IsValid(atlasTexture))
	{
		atlasBinPack.Init(defaultAtlasTextureSize, defaultAtlasTextureSize, 0, 0);
		CreateAtlasTexture(packingTag, 0, defaultAtlasTextureSize);
	}
}
void FLGUIAtlasData::CreateAtlasTexture(const FName& packingTag, int oldTextureSize, int newTextureSize)
{
#if WITH_EDITOR
	bool atlasSRGB = ULGUISettings::GetAtlasTextureSRGB(packingTag);
	auto filter = ULGUISettings::GetAtlasTextureFilter(packingTag);
#else
	static bool atlasSRGB = ULGUISettings::GetAtlasTextureSRGB(packingTag);
	static auto filter = ULGUISettings::GetAtlasTextureFilter(packingTag);
#endif
	auto texture = UTexture2D::CreateTransient(newTextureSize, newTextureSize, PF_B8G8R8A8);
	texture->CompressionSettings = TextureCompressionSettings::TC_EditorIcon;
	texture->LODGroup = TextureGroup::TEXTUREGROUP_UI;
	texture->SRGB = atlasSRGB;
	texture->Filter = filter;
	texture->UpdateResource();
	texture->AddToRoot();
	auto oldTexture = this->atlasTexture;
	this->atlasTexture = texture;

	//copy texture
	if (oldTextureSize != 0 && oldTexture != nullptr)
	{
		ENQUEUE_RENDER_COMMAND(FLGUISpriteCopyAtlasTexture)(
			[oldTexture, texture, oldTextureSize](FRHICommandListImmediate& RHICmdList)
		{
			FRHICopyTextureInfo CopyInfo;
			CopyInfo.SourcePosition = FIntVector(0, 0, 0);
			CopyInfo.Size = FIntVector(oldTextureSize, oldTextureSize, 0);
			CopyInfo.DestPosition = FIntVector(0, 0, 0);
			RHICmdList.CopyTexture(
				((FTexture2DResource*)oldTexture->Resource)->GetTexture2DRHI(),
				((FTexture2DResource*)texture->Resource)->GetTexture2DRHI(),
				CopyInfo
			);
			oldTexture->RemoveFromRoot();//ready for gc
		});
	}
}
int32 FLGUIAtlasData::ExpendTextureSize(const FName& packingTag)
{
	int32 oldTextureSize = this->atlasBinPack.GetBinWidth();
	int32 newTextureSize = oldTextureSize * 2;

	this->atlasBinPack.ExpendSize(newTextureSize, newTextureSize);
	//create new texture
	this->CreateAtlasTexture(packingTag, oldTextureSize, newTextureSize);
	//scale down sprite uv
	for (ULGUISpriteData* spriteItem : this->spriteDataArray)
	{
		spriteItem->atlasTexture = this->atlasTexture;
		spriteItem->spriteInfo.ScaleUV(0.5f);
	}
	//tell UISprite to scale down uv
	for (auto itemSprite : this->renderSpriteArray)
	{
		if (itemSprite.IsValid())
		{
			itemSprite->ApplyAtlasTextureScaleUp();
		}
	}
	//callback function
	if (expendTextureSizeCallback.IsBound())
	{
		expendTextureSizeCallback.Broadcast(this->atlasTexture);
	}

	return newTextureSize;
}


ULGUIAtlasManager* ULGUIAtlasManager::Instance = nullptr;
bool ULGUIAtlasManager::InitCheck()
{
	if (Instance == nullptr)
	{
		Instance = NewObject<ULGUIAtlasManager>();
		Instance->AddToRoot();
	}
	return true;
}
void ULGUIAtlasManager::BeginDestroy()
{
	ResetAtlasMap();
#if WITH_EDITOR
	ULGUISpriteData::MarkAllSpritesNeedToReinitialize();
#endif
	Instance = nullptr;
	Super::BeginDestroy();
}

const TMap<FName, FLGUIAtlasData>& ULGUIAtlasManager::GetAtlasMap()
{
	return atlasMap;
}
FLGUIAtlasData* ULGUIAtlasManager::FindOrAdd(const FName& packingTag)
{
	if (InitCheck())
	{
		return &(Instance->atlasMap.FindOrAdd(packingTag));
	}
	return nullptr;
}
FLGUIAtlasData* ULGUIAtlasManager::Find(const FName& packingTag)
{
	if (Instance != nullptr)
	{
		return Instance->atlasMap.Find(packingTag);
	}
	return nullptr;
}
void ULGUIAtlasManager::ResetAtlasMap()
{
	if (Instance != nullptr)
	{
		Instance->atlasMap.Reset();
		for (auto item : Instance->atlasMap)
		{
			item.Value.atlasTexture->RemoveFromRoot();
			item.Value.atlasTexture->ConditionalBeginDestroy();
		}
	}
}
