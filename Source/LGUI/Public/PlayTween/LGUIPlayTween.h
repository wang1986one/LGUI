﻿// Copyright 2019-2021 LexLiu. All Rights Reserved.

#pragma once
#include "LTweener.h"
#include "Event/LGUIEventDelegate.h"
#include "LGUIPlayTween.generated.h"

DECLARE_DYNAMIC_DELEGATE(FLGUIPlayTweenCompleteDynamicDelegate);

UCLASS(BlueprintType, Abstract, DefaultToInstanced, EditInlineNew)
class LGUI_API ULGUIPlayTween : public UObject
{
	GENERATED_BODY()

protected:
	UPROPERTY(EditAnywhere, Category = "Property")
		LTweenLoop loopType = LTweenLoop::Once;
	/** number of cycles to play (-1 for infinite) */
	UPROPERTY(EditAnywhere, Category = "Property", meta = (EditCondition = "loopType != LTweenLoop::Once"))
		int32 loopCount = -1;
	UPROPERTY(EditAnywhere, Category = "Property")
		LTweenEase easeType = LTweenEase::Linear;
	/** only valid if easeType=CurveFloat */
	UPROPERTY(EditAnywhere, Category = "Property", meta=(EditCondition = "easeType == LTweenEase::CurveFloat"))
		UCurveFloat* easeCurve;
	UPROPERTY(EditAnywhere, Category = "Property")
		float duration = 0.5f;
	UPROPERTY(EditAnywhere, Category = "Property")
		float startDelay = 0.0f;
	UPROPERTY(EditAnywhere, Category = "Event")
		FLGUIEventDelegate onStart = FLGUIEventDelegate(LGUIEventDelegateParameterType::Empty);
	/** parameter float is the progress in range 0-1, not affected by ease type (linear on time) */
	UPROPERTY(EditAnywhere, Category = "Event")
		FLGUIEventDelegate onUpdateProgress = FLGUIEventDelegate(LGUIEventDelegateParameterType::Float);
	UPROPERTY(EditAnywhere, Category = "Event")
		FLGUIEventDelegate onComplete = FLGUIEventDelegate(LGUIEventDelegateParameterType::Empty);
	UPROPERTY(Transient)
		ULTweener* tweener;
	FSimpleMulticastDelegate onComplete_Delegate;
public:
	UFUNCTION(BlueprintCallable, Category = "LGUI")
		void Start();
	UFUNCTION(BlueprintCallable, Category = "LGUI")
		void Stop();
	UFUNCTION(BlueprintCallable, Category = "LGUI")
		ULTweener* GetTweener()const { return tweener; }
	FDelegateHandle RegisterOnComplete(const FSimpleDelegate& InDelegate);
	FDelegateHandle RegisterOnComplete(const TFunction<void()>& InFunction);
	UFUNCTION(BlueprintCallable, Category = "LGUI")
		FLGUIDelegateHandleWrapper RegisterOnComplete(const FLGUIPlayTweenCompleteDynamicDelegate& InDelegate);
	void UnregisterOnComplete(const FDelegateHandle& InDelegateHandle);
	UFUNCTION(BlueprintCallable, Category = "LGUI")
		void UnregisterOnComplete(const FLGUIDelegateHandleWrapper& InDelegateHandle);
protected:
	virtual void OnUpdate(float progress)PURE_VIRTUAL(ULGUIPlayTween::OnUpdate, );
};
