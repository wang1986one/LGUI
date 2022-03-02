﻿// Copyright 2019-2022 LexLiu. All Rights Reserved.

#pragma once

#include "UIBaseRenderable.h"
#include "UIBatchGeometryRenderable.generated.h"

class UIGeometry;
class UMaterialInterface;
class ULGUICanvas;
class ULGUIMeshComponent;


USTRUCT(BlueprintType)
struct LGUI_API FLGUIGeometryVertex
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LGUI")
		FVector position = FVector::ZeroVector;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LGUI")
		FColor color = FColor::White;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LGUI")
		FVector2D uv0 = FVector2D::ZeroVector;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LGUI", AdvancedDisplay)
		FVector2D uv1 = FVector2D::ZeroVector;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LGUI", AdvancedDisplay)
		FVector2D uv2 = FVector2D::ZeroVector;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LGUI", AdvancedDisplay)
		FVector2D uv3 = FVector2D::ZeroVector;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LGUI", AdvancedDisplay)
		FVector normal = FVector(1, 0, 0);
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LGUI", AdvancedDisplay)
		FVector tangent = FVector(0, 1, 0);
};
/** a helper class for make LGUI geometry */
UCLASS(BlueprintType)
class LGUI_API ULGUIGeometryHelper : public UObject
{
	GENERATED_BODY()
public:
	UIGeometry* UIGeo = nullptr;
	UFUNCTION(BlueprintCallable, Category = "LGUI")
		void AddVertexSimple(FVector position, FColor color, FVector2D uv0);
	UFUNCTION(BlueprintCallable, Category = "LGUI")
		void AddVertexFull(FVector position, FColor color, FVector2D uv0, FVector2D uv1, FVector2D uv2, FVector2D uv3, FVector normal, FVector tangent);
	UFUNCTION(BlueprintCallable, Category = "LGUI")
		void AddVertexStruct(FLGUIGeometryVertex vertex);
	UFUNCTION(BlueprintCallable, Category = "LGUI")
		void AddTriangle(int index0, int index1, int index2);

	UFUNCTION(BlueprintCallable, Category = "LGUI")
		void SetGeometry(const TArray<FLGUIGeometryVertex>& InVertices, const TArray<int>& InIndices);
};


/** UI element which have render geometry, and can be batched and renderred by LGUICanvas */
UCLASS(Abstract, Blueprintable, ClassGroup=(LGUI))
class LGUI_API UUIBatchGeometryRenderable : public UUIBaseRenderable
{
	GENERATED_BODY()

public:	
	UUIBatchGeometryRenderable(const FObjectInitializer& ObjectInitializer);

protected:
	virtual void BeginPlay() override;
	virtual void TickComponent( float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction ) override;
#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
	virtual void OnRegister()override;
	virtual void OnUnregister()override;

	TSharedPtr<UIGeometry> geometry = nullptr;

	/** if have GeometryModifier component */
	bool HaveGeometryModifier(bool includeDisabled = true);
	/** Will any geometry modifier change these data? */
	void GeometryModifierWillChangeVertexData(bool& OutTriangleIndices, bool& OutVertexPosition, bool& OutUV, bool& OutColor);
	/** 
	 * use GeometryModifier to modify geometry 
	 */
	void ApplyGeometryModifier(bool triangleChanged, bool uvChanged, bool colorChanged, bool vertexPositionChanged);
	TInlineComponentArray<class UUIGeometryModifierBase*> GeometryModifierComponentArray;
public:
	UFUNCTION(BlueprintCallable, Category = "LGUI")
		UMaterialInterface* GetCustomUIMaterial()const { return CustomUIMaterial; }
	/** 
	 * if inMat is a UMaterialInstanceDynamic, then it will directly use for render.
	 * if not, then a new MaterialInstanceDynamic will be created to render this UI item, and the created MaterialInstanceDynamic may shared with others UI items.
	 */
	UFUNCTION(BlueprintCallable, Category = "LGUI")
		void SetCustomUIMaterial(UMaterialInterface* inMat);
	/** 
	 * If CustomUIMaterial is a UMaterialInstanceDynamic, then will return it directly.
	 * If not, then return a created MaterialInstanceDynamic that renderring this UI item, may shared by other UI item. if this UI item is not renderred yet, then return nullptr.
	 * LGUI only create MaterialInstanceDynamic when specified material have one of these LGUI material parameter: [MainTexture, RectClipOffsetAndSize, RectClipFeather, ClipTexture, TextureClipOffsetAndSize].
	 */
	UFUNCTION(BlueprintCallable, Category = "LGUI")
		UMaterialInstanceDynamic* GetMaterialInstanceDynamic()const;
#if 0//@todo: get this done
	/**
	 * Get pixel color at given 2d point.
	 * NOTE!!! This method will convert InPoint to uv coordinate, and get main texture's color at that uv coordinate, so main texture is required (main texture: UISprite's atlas texture, UITexture's texture, UIText's font texture).
	 * If you use custom material, then the result could be incorrect.
	 * @param	InPoint		Input 2d point in UI's local space
	 * @return	Pixel color value
	 */
	UFUNCTION(BlueprintCallable, Category = "LGUI")
		FColor GetPixelAtPoint(const FVector2D& InPoint)const PURE_VIRTUAL(UUIBatchGeometryRenderable::GetPixelAtPoint, return FColor::White;);
#endif
protected:
	virtual void OnRenderCanvasChanged(ULGUICanvas* OldCanvas, ULGUICanvas* NewCanvas)override;
	virtual void OnAnchorChange(bool InPivotChange, bool InSizeChange, bool InDiscardCache = true)override;
public:
	//@todo: these MarkXXXDirty function could be combined into a single function
	virtual void MarkVertexPositionDirty();
	virtual void MarkUVDirty();
	virtual void MarkTriangleDirty();
	virtual void MarkTextureDirty();
	virtual void MarkMaterialDirty();

	/** 
	 * Mark vertext dirty, so LGUI will trigger UpdateGeometry process, and OnUpdateGeometry will executed in next render update.
	 * Call this if you want to update vertex data. 
	 * For blueprint easily use.
	 */
	UFUNCTION(BlueprintCallable, Category = "LGUI")
		void MarkVertexDirty();

	void AddGeometryModifier(class UUIGeometryModifierBase* InModifier);
	void RemoveGeometryModifier(class UUIGeometryModifierBase* InModifier);
	void SortGeometryModifier();

	virtual void MarkAllDirtyRecursive()override;
	TSharedPtr<UIGeometry> GetGeometry()const { return geometry; }

	virtual bool LineTraceUI(FHitResult& OutHit, const FVector& Start, const FVector& End)override;
protected:
	friend class FUIGeometryRenderableCustomization;
	/** Use custom material to render this element */
	UPROPERTY(EditAnywhere, Category = "LGUI", meta = (DisplayThumbnail = "false"))
		UMaterialInterface* CustomUIMaterial = nullptr;

	/** if NeedTextureToCreateGeometry() is true, then we should provide this texture */
	virtual UTexture* GetTextureToCreateGeometry();

	/** do anything before acturally create or update geometry */
	virtual void OnBeforeCreateOrUpdateGeometry();
	/** fill and update ui geometry */
	virtual void OnUpdateGeometry(UIGeometry& InGeo, bool InTriangleChanged, bool InVertexPositionChanged, bool InVertexUVChanged, bool InVertexColorChanged);

	virtual void UpdateGeometry()override final;
	virtual void MarkFlattenHierarchyIndexDirty()override;
	virtual void GetGeometryBoundsInLocalSpace(FVector2D& OutMinPoint, FVector2D& OutMaxPoint)const override;
#if WITH_EDITOR
	virtual void GetGeometryBounds3DInLocalSpace(FVector& OutMinPoint, FVector& OutMaxPoint)const override;
#endif

	UFUNCTION(BlueprintImplementableEvent, Category = "LGUI", meta = (DisplayName = "GetTextureToCreateGeometry"))
		UTexture* ReceiveGetTextureToCreateGeometry();
	UFUNCTION(BlueprintImplementableEvent, Category = "LGUI", meta = (DisplayName = "OnBeforeCreateOrUpdateGeometry"))
		void ReceiveOnBeforeCreateOrUpdateGeometry();
	/** create or update geometry data */
	UFUNCTION(BlueprintImplementableEvent, Category = "LGUI", meta = (DisplayName = "OnUpdateGeometry"))
		void ReceiveOnUpdateGeometry(ULGUIGeometryHelper* InGeometryHelper, bool InTriangleChanged, bool InVertexPositionChanged, bool InVertexUVChanged, bool InVertexColorChanged);

private:
	/** local space vertex position changed */
	uint8 bLocalVertexPositionChanged : 1;
	/** vertex's uv change */
	uint8 bUVChanged:1;
	/** triangle index change */
	uint8 bTriangleChanged:1;
	FVector2D LocalMinPoint = FVector2D(0, 0), LocalMaxPoint = FVector2D(0, 0);
#if WITH_EDITORONLY_DATA
	FVector LocalMinPoint3D = FVector::ZeroVector, LocalMaxPoint3D = FVector::ZeroVector;
#endif
	void CalculateLocalBounds();
	UPROPERTY(Transient)ULGUIGeometryHelper* GeometryHelper = nullptr;
};
