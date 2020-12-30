﻿// Copyright 2019-2020 LexLiu. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Event/LGUIPointerEventData.h"
#include "LGUIBaseRayEmitter.generated.h"

/**
 * RayEmitter for raycaster
 */
UCLASS(Abstract)
class LGUI_API ULGUIBaseRayEmitter : public UActorComponent
{
	GENERATED_BODY()
public:
	virtual void BeginPlay()override;
protected:
	/** click/drag threshold*/
	UPROPERTY(EditAnywhere, Category = LGUI)
		float clickThreshold = 5;
	/** hold for a little while to entering drag mode */
	UPROPERTY(EditAnywhere, Category = LGUI)
		bool holdToDrag = false;
	UPROPERTY(EditAnywhere, Category = LGUI, meta = (EditCondition = "holdToDrag"))
		float holdToDragTime = 0.5f;

	float clickTresholdSquare = 0;
	float pressTime = 0;
	FVector currentRayOrigin, currentRayDirection;
	bool ShouldStartDrag_HoldToDrag(ULGUIPointerEventData* InPointerEventData);
public:
	UE_DEPRECATED(4.23, "SetClickThreshold not valid anymore, use SetInitialValue instead.")
	UFUNCTION(BlueprintCallable, Category = LGUI, meta=(DeprecatedFunction, DeprecationMessage = "SetClickThreshold not valid anymore, use SetInitialValue instead."))
		void SetClickThreshold(float value) { clickThreshold = value; clickTresholdSquare = clickThreshold * clickThreshold; }
	UFUNCTION(BlueprintCallable, Category = LGUI)
		void SetInitialValue(float InClickThreshold, bool InHoldToDrag, float InHoldToDragTime);
	UE_DEPRECATED(4.23, "GetClickThreshold not valid anymore, use GetInitialValue instead.")
	UFUNCTION(BlueprintCallable, Category = LGUI, meta = (DeprecatedFunction, DeprecationMessage = "GetClickThreshold not valid anymore, use GetInitialValue instead."))
		float GetClickThreshold() { return clickThreshold; }
	UFUNCTION(BlueprintCallable, Category = LGUI)
		void GetInitialValue(float& OutClickThreshold, bool& OutHoldToDrag, float& OutHoldToDragTime);
	virtual bool EmitRay(ULGUIPointerEventData* InPointerEventData, FVector& OutRayOrigin, FVector& OutRayDirection, TArray<AActor*>& InOutTraceOnlyActors, TArray<AActor*>& InOutTraceIgnoreActors);
	virtual bool ShouldStartDrag(ULGUIPointerEventData* InPointerEventData);
	virtual void MarkPress(ULGUIPointerEventData* InPointerEventData);
	virtual FVector GetCurrentRayOrigin() { return currentRayOrigin; }
	virtual FVector GetCurrentRayDirection() { return currentRayDirection; }
};
