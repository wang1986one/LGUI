﻿// Copyright 2019-2020 LexLiu. All Rights Reserved.

#pragma once

#include "UIGeometryModifierBase.h"
#include "LTweener.h"
#include "UIEffectTextAnimation.generated.h"

struct FUIEffectTextAnimation_SelectResult
{
public:
	int startCharIndex = 0;
	int endCharIndex = 0;
	TArray<float> lerpValueArray;
};

UCLASS(ClassGroup = (LGUI), Abstract, BlueprintType, DefaultToInstanced, EditInlineNew)
class LGUI_API UUIEffectTextAnimation_Selector : public UObject
{
	GENERATED_BODY()
protected:
	UPROPERTY(EditAnywhere, Category = "Property", meta = (ClampMin = "0.0", ClampMax = "1.0"))
		float start = 0.0f;
	UPROPERTY(EditAnywhere, Category = "Property", meta = (ClampMin = "0.0", ClampMax = "1.0"))
		float end = 1.0f;
	//editor clamp this value between 0-1, but you can assign it more than 0-1 for PositionWave/RotationWave/ScaleWave/XXXWave
	UPROPERTY(EditAnywhere, Category = "Property", meta = (ClampMin = "0.0", ClampMax = "1.0"))
		float offset = 0.5f;
	class UUIText* GetUIText();
public:
	virtual bool Select(class UUIText* InUIText, FUIEffectTextAnimation_SelectResult& OutSelection) PURE_VIRTUAL(UUIEffectTextAnimation_Selector::Select, return false;);
	
	UFUNCTION(BlueprintCallable, Category = "LGUI")
		float GetStart()const { return start; }
	UFUNCTION(BlueprintCallable, Category = "LGUI")
		float GetEnd()const { return end; }
	UFUNCTION(BlueprintCallable, Category = "LGUI")
		float GetOffset()const { return offset; }

	UFUNCTION(BlueprintCallable, Category = "LGUI")
		void SetStart(float value);
	UFUNCTION(BlueprintCallable, Category = "LGUI")
		void SetEnd(float value);
	UFUNCTION(BlueprintCallable, Category = "LGUI")
		void SetOffset(float value);
};

UCLASS(ClassGroup = (LGUI), Abstract, BlueprintType, DefaultToInstanced, EditInlineNew)
class LGUI_API UUIEffectTextAnimation_Property : public UObject
{
	GENERATED_BODY()
protected:
	class UUIText* GetUIText();
	FORCEINLINE void MarkUITextPositionDirty();
public:
	virtual void ApplyProperty(class UUIText* InUIText, const FUIEffectTextAnimation_SelectResult& InSelection, TSharedPtr<UIGeometry> OutGeometry) PURE_VIRTUAL(UUIEffectTextAnimation_Property::ApplyEffect, );
};

//per character animation control for UIText
UCLASS(ClassGroup = (LGUI), Blueprintable, meta = (BlueprintSpawnableComponent))
class LGUI_API UUIEffectTextAnimation : public UUIGeometryModifierBase
{
	GENERATED_BODY()

public:	
	UUIEffectTextAnimation();
protected:
	UPROPERTY(EditAnywhere, Category = "LGUI", Instanced)
		UUIEffectTextAnimation_Selector* selector;
	UPROPERTY(EditAnywhere, Category = "LGUI", Instanced)
		TArray<UUIEffectTextAnimation_Property*> properties;
	UPROPERTY(Transient)class UUIText* uiText;
	FUIEffectTextAnimation_SelectResult selection;
	bool CheckUIText();
public:
	virtual void ModifyUIGeometry(TSharedPtr<UIGeometry>& InGeometry, int32& InOutOriginVerticesCount, int32& InOutOriginTriangleIndicesCount, bool& OutTriangleChanged)override;
	class UUIText* GetUIText();

	UFUNCTION(BlueprintCallable, Category = "LGUI")
		UUIEffectTextAnimation_Selector* GetSelector()const { return selector; }
	UFUNCTION(BlueprintCallable, Category = "LGUI")
		const TArray<UUIEffectTextAnimation_Property*> GetProperties()const { return properties; }
	UFUNCTION(BlueprintCallable, Category = "LGUI")
		UUIEffectTextAnimation_Property* GetProperty(int index)const;
	UFUNCTION(BlueprintCallable, Category = "LGUI")
		float GetSelectorOffset()const;
	UFUNCTION(BlueprintCallable, Category = "LGUI")
		float GetSelectorStart()const;
	UFUNCTION(BlueprintCallable, Category = "LGUI")
		float GetSelectorEnd()const;

	UFUNCTION(BlueprintCallable, Category = "LGUI")
		void SetSelector(UUIEffectTextAnimation_Selector* value);
	UFUNCTION(BlueprintCallable, Category = "LGUI")
		void SetProperties(const TArray<UUIEffectTextAnimation_Property*>& value);
	UFUNCTION(BlueprintCallable, Category = "LGUI")
		void SetProperty(int index, UUIEffectTextAnimation_Property* value);
	UFUNCTION(BlueprintCallable, Category = "LGUI")
		void SetSelectorOffset(float value);
	UFUNCTION(BlueprintCallable, Category = "LGUI")
		void SetSelectorStart(float value);
	UFUNCTION(BlueprintCallable, Category = "LGUI")
		void SetSelectorEnd(float value);
};
