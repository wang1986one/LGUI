﻿// Copyright 2019-2020 LexLiu. All Rights Reserved.

#pragma once

#pragma warning(disable:4668)//error C4668: '__SUNPRO_C' is not defined as a preprocessor macro, replacing with '0' for '#if/#elif'

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "RHI.h"
#include <ft2build.h>
#include FT_FREETYPE_H
#include "LGUIEditHelper.h"
#include "Utils/MaxRectsBinPack/MaxRectsBinPack.h"
#include "LGUIFontData.generated.h"


struct FLGUIFontKeyData
{
public:
	FLGUIFontKeyData() {}
	FLGUIFontKeyData(const TCHAR& inCharIndex, const uint16& inCharSize)
	{
		this->charIndex = inCharIndex;
		this->charSize = inCharSize;
	}
	TCHAR charIndex = 0;
	uint16 charSize = 0;
	bool operator==(const FLGUIFontKeyData& other)const
	{
		return this->charIndex == other.charIndex && this->charSize == other.charSize;
	}
	friend FORCEINLINE uint32 GetTypeHash(const FLGUIFontKeyData& other)
	{
		return HashCombine(GetTypeHash(other.charIndex), GetTypeHash(other.charSize));
	}
};

struct FLGUICharData
{
public:
	uint16 width = 0;
	uint16 height = 0;
	int16 xoffset = 0;
	int16 yoffset = 0;
	float xadvance = 0;
	float uv0X = 0;
	float uv0Y = 0;
	float uv3X = 0;
	float uv3Y = 0;

	FVector2D GetUV0()
	{
		return FVector2D(uv0X, uv0Y);
	}
	FVector2D GetUV3()
	{
		return FVector2D(uv3X, uv3Y);
	}
	FVector2D GetUV2()
	{
		return FVector2D(uv0X, uv3Y);
	}
	FVector2D GetUV1()
	{
		return FVector2D(uv3X, uv0Y);
	}
};

class UUIText;
UCLASS(BlueprintType)
class LGUI_API ULGUIFontData : public UObject
{
	GENERATED_BODY()
public:
	ULGUIFontData();
	~ULGUIFontData();

	//Font file path, absolute path or relative to ProjectDir
	UPROPERTY(EditAnywhere, Category = "LGUI")
		FString fontFilePath;
	//Font file use relative path(relative to ProjectDir) or absolute path. After build your game, remember to copy your font file to target path, unless "useExternalFileOrEmbedInToUAsset" is false
	UPROPERTY(EditAnywhere, Category = "LGUI")
		bool useRelativeFilePath = true;
	//When in build, use external file or embed into uasset. But in editor, will always load from fontFilePath.
	UPROPERTY(EditAnywhere, Category = "LGUI")
		bool useExternalFileOrEmbedInToUAsset = false;
	//Some font text may not renderred at vertical center, use this to offset
	UPROPERTY(EditAnywhere, Category = "LGUI")
		float fixedVerticalOffset = 0.0f;
	//angle of italic style in degree
	UPROPERTY(EditAnywhere, Category = "LGUI")
		float italicAngle = 15.0f;
	//bold size radio for bold style, large number create more bold effect
	UPROPERTY(EditAnywhere, Category = "LGUI")
		float boldRatio = 0.015f;
	//Packing tag of this font. If packingTag is not none, then LGUI will search UISprite's atlas packingTag, and pack font texture into sprite atlas's texture.
	//This can be very useful to reduce drawcall.
	UPROPERTY(EditAnywhere, Category = "LGUI")
		FName packingTag;
	//Texture of this font
	UPROPERTY(VisibleAnywhere, Category = "LGUI")
		UTexture2D* texture;

	void InitFreeType();
	void DeinitFreeType();
	virtual void FinishDestroy()override;

	const float MAX_UINT16_RECEPROCAL = 1.0f / 65536.0f;
	const int32 SPACE_NEED_EXPEND = 1;//when draw a rectangle, need to expend 1 pixel to avoid too sharp edge
	const int32 SPACE_NEED_EXPENDx2 = SPACE_NEED_EXPEND * 2;
	const int32 SPACE_BETWEEN_GLYPH = SPACE_NEED_EXPEND + 1;//space between char in texture
	const int32 SPACE_BETWEEN_GLYPHx2 = SPACE_BETWEEN_GLYPH * 2;

	FORCEINLINE void AddUIText(UUIText* InText);
	FORCEINLINE void RemoveUIText(UUIText* InText);
private:
	//Collection of UIText which use this font to render.
	UPROPERTY(VisibleAnywhere, Transient, Category = "LGUI")
		TArray<TWeakObjectPtr<UUIText>> renderTextArray;

	friend class FLGUIFontDataCustomization;
	UPROPERTY()
		TArray<uint8> fontBinaryArray;
	UPROPERTY(Transient)
		TArray<uint8> tempFontBinaryArray;//temp array for storing data, because freetype need to load font to memory and keep alive
	TMap<FLGUIFontKeyData, FLGUICharData> charDataMap;
	struct FLGUIAtlasData* packingAtlasData = nullptr;
	FDelegateHandle packingAtlasTextureExpandDelegateHandle;

	rbp::MaxRectsBinPack binPack;//for rect packing
	int32 textureSize;//current texture size
	float fullTextureSizeReciprocal;//1.0 / textureSize

	FLGUIFontKeyData cacheFontKey;
	FLGUICharData cacheCharData;

	FT_Library library = nullptr;
	FT_Face face = nullptr;
	bool alreadyInitialized = false;
	bool usePackingTag = false;
	FLGUICharData* PushCharIntoFont(const TCHAR& charIndex, const uint16& charSize);
	/*Insert rect into area, assign pixel if succeed
	 return: if can fit in rect area return true, else false
	*/
	bool PackRectAndInsertChar(int32 InExtraSpace, const FT_Bitmap& InCharBitmap, const FT_GlyphSlot& InSlot, rbp::MaxRectsBinPack& InOutBinpack, UTexture2D* InTexture);
	void UpdateTextureRegions(UTexture2D* Texture, int32 MipIndex, uint32 NumRegions, FUpdateTextureRegion2D* Regions, uint32 SrcPitch, uint32 SrcBpp, uint8* SrcData, bool bFreeData);
	void UpdateFontTextureRegion(UTexture2D* Texture, FUpdateTextureRegion2D* Region, uint32 SrcPitch, uint32 SrcBpp, uint8* SrcData);

	void CreateFontTexture(int oldTextureSize, int newTextureSize);
	void ApplyPackingAtlasTextureExpand(UTexture2D* newTexture, int newTextureSize);
public:
	FLGUICharData* GetCharData(const TCHAR& charIndex, const uint16& charSize);
#if WITH_EDITOR
	void ReloadFont();
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
	static ULGUIFontData* GetDefaultFont();
};
