﻿// Copyright 2019-2022 LexLiu. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/ActorComponent.h"
#include "LGUIDelegateDeclaration.h"
#include "LGUIDelegateHandleWrapper.h"
#include "LGUIEventSystem.generated.h"

class ULGUIPointerEventData;
class UUISelectableComponent;
class ULGUIBaseInputModule;

DECLARE_DYNAMIC_DELEGATE_ThreeParams(FLGUIHitDynamicDelegate, bool, isHit, const FHitResult&, hitResult, USceneComponent*, hitComponent);
DECLARE_DYNAMIC_DELEGATE_OneParam(FLGUIPointerEventDynamicDelegate, ULGUIPointerEventData*, pointerEventData);
DECLARE_DYNAMIC_DELEGATE_OneParam(FLGUIBaseEventDynamicDelegate, ULGUIBaseEventData*, eventData);


/**
 * This is the place for manage LGUI's input/raycast/event.
 * InputTrigger and InputScroll need mannually setup in InputModule.
 * About event bubble: if all interface of target component and actor return true, then event will bubble up. if no interface found on target, then event will bubble up
 */
UCLASS(ClassGroup = (LGUI), Blueprintable, meta = (BlueprintSpawnableComponent))
class LGUI_API ULGUIEventSystem : public UActorComponent
{
	GENERATED_BODY()

public:
	ULGUIEventSystem();

	UFUNCTION(BlueprintPure, Category = LGUI, meta = (WorldContext = "WorldContextObject", DisplayName = "Get LGUI Event System Instance"))
		static ULGUIEventSystem* GetLGUIEventSystemInstance(UObject* WorldContextObject);
protected:
	/** a world should only have one LGUIEventSystem */
	static TMap<UWorld*, ULGUIEventSystem*> WorldToInstanceMap;
	bool existInInstanceMap = false;
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void BeginDestroy()override;

protected:

#if WITH_EDITORONLY_DATA
	UPROPERTY(EditAnywhere, Category = LGUI)
		bool outputLog = false;
#endif
	UPROPERTY(VisibleAnywhere, Category = LGUI)
		bool bRayEventEnable = true;

	void ProcessInputEvent();
public:
	/** clear event. eg when mouse is hovering a UI and highlight, and then event is disabled, we can use this to clear the hover event */
	UFUNCTION(BlueprintCallable, Category = LGUI)
		void ClearEvent();
	/** 
	 * SetRaycast enable or disable
	 * @param	clearEvent		call ClearEvent after disable Raycast
	 */
	UFUNCTION(BlueprintCallable, Category = LGUI)
		void SetRaycastEnable(bool enable, bool clearEvent = false);

	UFUNCTION(BlueprintCallable, Category = LGUI)
		void SetSelectComponent(USceneComponent* InSelectComp, ULGUIBaseEventData* eventData, ELGUIEventFireType eventFireType);
	UFUNCTION(BlueprintCallable, Category = LGUI)
		void SetSelectComponentWithDefault(USceneComponent* InSelectComp);
	UFUNCTION(BlueprintCallable, Category = LGUI)
		USceneComponent* GetCurrentSelectedComponent() { return selectedComponent; }
	UFUNCTION(BlueprintCallable, Category = LGUI)
		ULGUIBaseInputModule* GetCurrentInputModule();

	UPROPERTY(VisibleAnywhere, Category = LGUI)
		TMap<int, ULGUIPointerEventData*> pointerEventDataMap;
	/**
	 * Get PointerEventData by given pointerID.
	 * @param	pointerID	0 for mouse input, touch id for touch input
	 */
	UFUNCTION(BlueprintCallable, Category = LGUI)
		ULGUIPointerEventData* GetPointerEventData(int pointerID = 0, bool createIfNotExist = false);
protected:
	/** call back for hit event */
	FLGUIMulticastHitDelegate hitEvent;
	/** call back for all event */
	FLGUIMulticastBaseEventDelegate globalListenerPointerEvent;
public:
	void RegisterHitEvent(const FLGUIHitDelegate& InEvent);
	void UnregisterHitEvent(const FLGUIHitDelegate& InEvent);
	/**
	 * Register a global event listener, that listener will called when any LGUIEventSystem's event is executed.
	 * @param InEvent Callback delegate, you can cast LGUIBaseEventData to LGUIPointerEventData if you need.
	 */
	void RegisterGlobalListener(const FLGUIBaseEventDelegate& InEvent);
	void UnregisterGlobalListener(const FLGUIBaseEventDelegate& InEvent);

	UFUNCTION(BlueprintCallable, Category = LGUI)
		FLGUIDelegateHandleWrapper RegisterHitEvent(const FLGUIHitDynamicDelegate& InDelegate);
	UFUNCTION(BlueprintCallable, Category = LGUI)
		void UnregisterHitEvent(const FLGUIDelegateHandleWrapper& InHandle);
	/**
	 * Register a global event listener, that listener will called when any LGUIEventSystem's event is executed.
	 * @param InDelegate Callback delegate, you can cast LGUIBaseEventData to LGUIPointerEventData if you need.
	 */
	UFUNCTION(BlueprintCallable, Category = LGUI)
		FLGUIDelegateHandleWrapper RegisterGlobalListener(const FLGUIBaseEventDynamicDelegate& InDelegate);
	UFUNCTION(BlueprintCallable, Category = LGUI)
		void UnregisterGlobalListener(const FLGUIDelegateHandleWrapper& InHandle);
	void RaiseHitEvent(bool hitOrNot, const FHitResult& hitResult, USceneComponent* hitComponent);

	UFUNCTION(BlueprintCallable, Category = LGUI)
		void SetHighlightedComponentForNavigation(USceneComponent* InComp);
	UFUNCTION(BlueprintCallable, Category = LGUI)
		USceneComponent* GetHighlightedComponentForNavigation()const { return highlightedComponent; }

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = LGUI)
		ELGUIPointerInputType defaultInputType = ELGUIPointerInputType::Pointer;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = LGUI)
		ELGUIEventFireType eventFireTypeForNavigation = ELGUIEventFireType::TargetActorAndAllItsComponents;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = LGUI)
		float navigateInputInterval = 0.2f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = LGUI)
		float navigateInputIntervalForFirstTime = 0.5f;
protected:
	UPROPERTY(Transient)USceneComponent* selectedComponent = nullptr;
	UPROPERTY(Transient)USceneComponent* highlightedComponent = nullptr;
public:
	void CallOnPointerEnter(USceneComponent* component, ULGUIPointerEventData* eventData, ELGUIEventFireType eventFireType);
	void CallOnPointerExit(USceneComponent* component, ULGUIPointerEventData* eventData, ELGUIEventFireType eventFireType);
	void CallOnPointerDown(USceneComponent* component, ULGUIPointerEventData* eventData, ELGUIEventFireType eventFireType);
	void CallOnPointerUp(USceneComponent* component, ULGUIPointerEventData* eventData, ELGUIEventFireType eventFireType);
	void CallOnPointerClick(USceneComponent* component, ULGUIPointerEventData* eventData, ELGUIEventFireType eventFireType);
	void CallOnPointerBeginDrag(USceneComponent* component, ULGUIPointerEventData* eventData, ELGUIEventFireType eventFireType);
	void CallOnPointerDrag(USceneComponent* component, ULGUIPointerEventData* eventData, ELGUIEventFireType eventFireType);
	void CallOnPointerEndDrag(USceneComponent* component, ULGUIPointerEventData* eventData, ELGUIEventFireType eventFireType);
	void CallOnPointerScroll(USceneComponent* component, ULGUIPointerEventData* eventData, ELGUIEventFireType eventFireType);
	void CallOnPointerDragDrop(USceneComponent* component, ULGUIPointerEventData* eventData, ELGUIEventFireType eventFireType);
	void CallOnPointerSelect(USceneComponent* component, ULGUIBaseEventData* eventData, ELGUIEventFireType eventFireType);
	void CallOnPointerDeselect(USceneComponent* component, ULGUIBaseEventData* eventData, ELGUIEventFireType eventFireType);

	void LogEventData(ULGUIBaseEventData* eventData);


	void BubbleOnPointerEnter(AActor* actor, ULGUIPointerEventData* eventData);
	void BubbleOnPointerExit(AActor* actor, ULGUIPointerEventData* eventData);
	void BubbleOnPointerDown(AActor* actor, ULGUIPointerEventData* eventData);
	void BubbleOnPointerUp(AActor* actor, ULGUIPointerEventData* eventData);
	void BubbleOnPointerClick(AActor* actor, ULGUIPointerEventData* eventData);
	void BubbleOnPointerBeginDrag(AActor* actor, ULGUIPointerEventData* eventData);
	void BubbleOnPointerDrag(AActor* actor, ULGUIPointerEventData* eventData);
	void BubbleOnPointerEndDrag(AActor* actor, ULGUIPointerEventData* eventData);
	void BubbleOnPointerScroll(AActor* actor, ULGUIPointerEventData* eventData);
	void BubbleOnPointerDragDrop(AActor* actor, ULGUIPointerEventData* eventData);
	void BubbleOnPointerSelect(AActor* actor, ULGUIBaseEventData* eventData);
	void BubbleOnPointerDeselect(AActor* actor, ULGUIBaseEventData* eventData);
};

/*
 * This is a preset actor that contans a LGUIEventSystem component
 */
UCLASS(ClassGroup = LGUI, NotPlaceable)
class LGUI_API ALGUIEventSystemActor : public AActor
{
	GENERATED_BODY()

public:
	ALGUIEventSystemActor();
protected:
	UPROPERTY(Category = "LGUI", VisibleAnywhere, BlueprintReadOnly, Transient, meta = (AllowPrivateAccess = "true"))
		class ULGUIEventSystem* EventSystem;
};
