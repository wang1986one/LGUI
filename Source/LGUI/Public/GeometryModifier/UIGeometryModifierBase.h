﻿// Copyright 2019-2022 LexLiu. All Rights Reserved.

#pragma once

#include "Core/UIGeometry.h"
#include "Core/ActorComponent/UIBatchGeometryRenderable.h"
#include "Core/ActorComponent/UIBatchGeometryRenderable_BP.h"
#include "Components/ActorComponent.h"
#include "UIGeometryModifierBase.generated.h"

class UUIBatchGeometryRenderable;
class UUIText;

UENUM(BlueprintType)
enum class ELGUIGeometryModifierHelper_UITextModifyPositionType:uint8
{
	//Relative to character's origin position
	Relative,
	//Direct set character's position
	Absolute,
};
/** a helper class for UIGeometryModifierBase to easily modify ui geometry */
UCLASS(BlueprintType)
class LGUI_API ULGUIGeometryModifierHelper : public UObject
{
	GENERATED_BODY()
public:
	UPROPERTY(Transient, BlueprintReadOnly, Category = "LGUI")
		TArray<FLGUIGeometryVertex> cacheVertices;
	UPROPERTY(Transient, BlueprintReadOnly, Category = "LGUI")
		TArray<int32> cacheTriangleIndices;
	void BeginModify(TSharedPtr<UIGeometry> InUIGeometry, int32 InOriginVerticesCount, int32 InOriginTriangleIndicesCount);
	void EndModify(TSharedPtr<UIGeometry> InUIGeometry, int32& OutOriginVerticesCount, int32& OutOriginTriangleIndicesCount, bool& OutTriangleChanged);

	/** Get character's center position in UIText's rect range, and convert to 0-1 range (left is 0 and right is 1) */
	UFUNCTION(BlueprintCallable, Category = "LGUI")
		float UITextHelperFunction_GetCharHorizontalPositionRatio01(UUIText* InUIText, int InCharIndex)const;
	/**
	 * Modify character's position & rotation & scale
	 * @param	InPositionType		Set position type, relative to origin position or absolute position
	 */
	UFUNCTION(BlueprintCallable, Category = "LGUI")
		void UITextHelperFunction_ModifyCharGeometry_Transform(UUIText* InUIText, int InCharIndex, const FVector& InPosition, ELGUIGeometryModifierHelper_UITextModifyPositionType InPositionType, const FRotator& InRotator, const FVector& InScale);
	/**
	 * Modify character's position
	 * @param	InPositionType		Set position type, relative to origin position or absolute position
	 */
	UFUNCTION(BlueprintCallable, Category = "LGUI")
		void UITextHelperFunction_ModifyCharGeometry_Position(UUIText* InUIText, int InCharIndex, const FVector& InPosition, ELGUIGeometryModifierHelper_UITextModifyPositionType InPositionType);
	UFUNCTION(BlueprintCallable, Category = "LGUI")
		void UITextHelperFunction_ModifyCharGeometry_Rotate(UUIText* InUIText, int InCharIndex, const FRotator& InRotator);
	UFUNCTION(BlueprintCallable, Category = "LGUI")
		void UITextHelperFunction_ModifyCharGeometry_Scale(UUIText* InUIText, int InCharIndex, const FVector& InScale);
	UFUNCTION(BlueprintCallable, Category = "LGUI")
		void UITextHelperFunction_ModifyCharGeometry_Color(UUIText* InUIText, int InCharIndex, const FColor& InColor);
	UFUNCTION(BlueprintCallable, Category = "LGUI")
		void UITextHelperFunction_ModifyCharGeometry_Alpha(UUIText* InUIText, int InCharIndex, const float& InAlpha);
};

/** 
 * For modify ui geometry, act like a filter.
 * Need UIBatchGeometryRenderable component.
 */
UCLASS(Abstract, Blueprintable, meta = (BlueprintSpawnableComponent))
class LGUI_API UUIGeometryModifierBase : public UActorComponent
{
	GENERATED_BODY()

public:	
	UUIGeometryModifierBase();

protected:
	virtual void BeginPlay()override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason)override;
#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
	virtual void OnRegister()override;
	virtual void OnUnregister()override;

	/** Enable this geometry modifier */
	UPROPERTY(EditAnywhere, Category = "LGUI")
		bool bEnable = true;
	/** Execute order of this effect in actor. Smaller executeOrder will execute eailer */
	UPROPERTY(EditAnywhere, Category = "LGUI")
		int executeOrder = 0;
	/** 
	 * If there are multiple UIBatchGeometryRenderable components, then select one of them by name.
	 * Leave it empty if only one UIBatchGeometryRenderable component.
	 */
	UPROPERTY(EditAnywhere, Category = "LGUI")
		FName componentName;

private:
	TWeakObjectPtr<UUIBatchGeometryRenderable> renderableUIItem;
	void RemoveFromUIBatchGeometry();
	void AddToUIBatchGeometry();
public:
	UFUNCTION(BlueprintCallable, Category = "LGUI")
		UUIBatchGeometryRenderable* GetRenderableUIItem();
	UFUNCTION(BlueprintCallable, Category = "LGUI")
		bool GetEnable()const { return bEnable; }

	UFUNCTION(BlueprintCallable, Category = "LGUI")
		int GetExecuteOrder()const { return executeOrder; }
	UFUNCTION(BlueprintCallable, Category = "LGUI")
		void SetEnable(bool value);
	UFUNCTION(BlueprintCallable, Category = "LGUI")
		void SetExecuteOrder();
	/**
	 * Modify UI geometry's vertex and triangle.
	 * @param	InOutOriginVerticesCount	orign vertex count; after modify, new vertex count must be set to this
	 * @param	InOutOriginTriangleIndicesCount		orign triangle indices count; after modify, new triangle indices count must be set to this
	 * @param	OutTriangleChanged		if this modifier affect triangle, then set this to true
	 * @param	InUVChanged			vertex uv changed
	 * @param	InColorChanged			vertex color changed
	 * @param	InVertexPositionChanged			vertex position changed
	 * @param	InTransformChanged			object's transform changed
	 */
	virtual void ModifyUIGeometry(
		TSharedPtr<UIGeometry>& InGeometry, int32& InOutOriginVerticesCount, int32& InOutOriginTriangleIndicesCount, bool& OutTriangleChanged,
		bool InUVChanged, bool InColorChanged, bool InVertexPositionChanged, bool InTransformChanged
	);
protected:
	UPROPERTY(Transient) ULGUIGeometryModifierHelper* GeometryModifierHelper = nullptr;
	/**
	 * Modify UI geometry's vertex and triangle.
	 * @param	InUVChanged			vertex uv changed
	 * @param	InColorChanged			vertex color changed
	 * @param	InVertexPositionChanged			vertex position changed
	 * @param	InTransformChanged			object's transform changed
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "LGUI", meta = (DisplayName = "ModifyUIGeometry"))
		void ReceiveModifyUIGeometry(ULGUIGeometryModifierHelper* InGeometryModifierHelper
			, bool InUVChanged, bool InColorChanged, bool InVertexPositionChanged, bool InTransformChanged
		);
};
