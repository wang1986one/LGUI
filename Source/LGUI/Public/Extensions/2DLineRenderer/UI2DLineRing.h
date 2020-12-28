﻿// Copyright 2019-2020 LexLiu. All Rights Reserved.

#pragma once

#include "Core/Actor/UIBaseActor.h"
#include "UI2DLineRendererBase.h"
#include "LTweenHeaders.h"
#include "UI2DLineRing.generated.h"


UCLASS(ClassGroup = (LGUI), Blueprintable, meta = (BlueprintSpawnableComponent))
class LGUI_API UUI2DLineRing : public UUI2DLineRendererBase
{
	GENERATED_BODY()

public:	
	UUI2DLineRing(const FObjectInitializer& ObjectInitializer);

protected:
	virtual void BeginPlay()override;

	UPROPERTY(EditAnywhere, Category = LGUI)float AngleBegin = 0.0f;
	UPROPERTY(EditAnywhere, Category = LGUI)float AngleEnd = 90.0f;
	//line segment
	UPROPERTY(EditAnywhere, Category = LGUI, meta = (ClampMin = "0", ClampMax = "200"))int Segment = 12;

	UPROPERTY(VisibleAnywhere, Transient, Category = LGUI)TArray<FVector2D> CurrentPointArray;

	//Begin UI2DLineRendererBase interface
	virtual const TArray<FVector2D>& GetCalcaultedPointArray()override
	{
		return CurrentPointArray;
	}
	virtual void CalculatePoints()override;
	virtual bool OverrideStartPointTangentDirection()override { return true; }
	virtual bool OverrideEndPointTangentDirection()override { return true; }
	virtual FVector2D GetStartPointTangentDirection()override;
	virtual FVector2D GetEndPointTangentDirection()override;
	//End UI2DLineRendererBase interface
public:
	UFUNCTION(BlueprintCallable, Category = LGUI)float GetAngleBegin() { return AngleBegin; }
	UFUNCTION(BlueprintCallable, Category = LGUI)float GetAngleEnd() { return AngleEnd; }
	UFUNCTION(BlueprintCallable, Category = LGUI)int GetSegment() { return Segment; }

	UFUNCTION(BlueprintCallable, Category = LGUI)void SetAngleBegin(float newValue);
	UFUNCTION(BlueprintCallable, Category = LGUI)void SetAngleEnd(float newValue);
	UFUNCTION(BlueprintCallable, Category = LGUI)void SetSegment(float newValue);

	UFUNCTION(BlueprintCallable, Category = LGUI)ULTweener* AngleBeginTo(float endValue, float duration = 0.5f, float delay = 0.0f, LTweenEase easeType = LTweenEase::OutCubic);
	UFUNCTION(BlueprintCallable, Category = LGUI)ULTweener* AngleEndTo(float endValue, float duration = 0.5f, float delay = 0.0f, LTweenEase easeType = LTweenEase::OutCubic);
};


UCLASS()
class LGUI_API AUI2DLineRingActor : public AUIBaseActor
{
	GENERATED_BODY()

public:
	AUI2DLineRingActor();

	FORCEINLINE virtual UUIItem* GetUIItem()const override { return UIElement; }
	FORCEINLINE UUI2DLineRing* Get2DLineRing()const { return UIElement; }
private:
	UPROPERTY(Category = "LGUI", VisibleAnywhere, BlueprintReadOnly, Transient, meta = (AllowPrivateAccess = "true"))
		class UUI2DLineRing* UIElement;

};