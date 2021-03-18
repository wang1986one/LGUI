﻿// Copyright 2019-2021 LexLiu. All Rights Reserved.

#pragma once

#include "Event/Interface/LGUIPointerEnterExitInterface.h"
#include "Event/Interface/LGUIPointerDownUpInterface.h"
#include "Event/Interface/LGUIPointerSelectDeselectInterface.h"
#include "Components/ActorComponent.h"
#include "Core/LGUIBehaviour.h"
#include "LGUIComponentReference.h"
#include "UISelectableComponent.generated.h"

UENUM(BlueprintType, Category = LGUI)
enum class UISelectableTransitionType:uint8
{
	None				UMETA(DisplayName = "None"),
	/** In this mode, TransitionActor's color property will be override by this component. */
	ColorTint			UMETA(DisplayName = "ColorTint"),
	/** In this mode, TransitionActor's root component need to be a UISpriteBase. */
	/** Target UISprite's sprite will be override by this component. */
	SpriteSwap			UMETA(DisplayName = "SpriteSwap"),
	/** You can implement a UISelectableTransitionComponent in c++ or blueprint to do the transition, and add this component to transition actor */
	TransitionComponent			UMETA(DisplayName = "TransitionComponent"),
};
UENUM(BlueprintType, Category = LGUI)
enum class EUISelectableSelectionState :uint8
{
	/** Not hovered by pointer, just a normal state. */
	Normal,
	/** Hovered by pointer. */
	Highlighted,
	/** Pressed by pointer. */
	Pressed,
	/** Disabled, not interactable. Check the "OnUIInteractionStateChanged" function of UISelectableComponent, to see why it is disabled. */
	Disabled,
};
UENUM(BlueprintType, Category = LGUI)
enum class EUISelectableNavigationMode:uint8
{
	/** No navigation. */
	None,
	/** Navigation is controlled by LGUI. */
	Auto,
	/** Control your navigation behaviour on your own. */
	Explicit,
};

class ULGUISpriteData;

UCLASS(HideCategories = (Collision, LOD, Physics, Cooking, Rendering, Activation, Actor, Input, Lighting, Mobile), ClassGroup = (LGUI), Blueprintable, meta = (BlueprintSpawnableComponent))
class LGUI_API UUISelectableComponent : public ULGUIBehaviour, public ILGUIPointerEnterExitInterface, public ILGUIPointerDownUpInterface, public ILGUIPointerSelectDeselectInterface
{
	GENERATED_BODY()
	
protected:

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

protected:
	virtual void Awake() override;
	virtual void Start() override;
	virtual void OnDestroy()override;

	friend class FUISelectableCustomization;
	/** If not assigned, then use self. must have UIItem component */
	UPROPERTY(EditAnywhere, Category = "LGUI-Selectable")
		class AUIBaseActor* TransitionActor;
	/** inherited events of this component can bubble up? */
	UPROPERTY(EditAnywhere, Category = "LGUI-Selectable")
		bool AllowEventBubbleUp = false;

	virtual void OnUIInteractionStateChanged(bool interactableOrNot)override;

	bool CheckTarget();
	
#pragma region Transition
	UPROPERTY(EditAnywhere, Category = "LGUI-Selectable")
		UISelectableTransitionType Transition;

	UPROPERTY(Transient)class ULTweener* TransitionTweener = nullptr;
	UPROPERTY(EditAnywhere, Category = "LGUI-Selectable")
		FColor NormalColor = FColor(255, 255, 255, 255);
	UPROPERTY(EditAnywhere, Category = "LGUI-Selectable")
		FColor HighlightedColor = FColor(200, 200, 200, 255);
	UPROPERTY(EditAnywhere, Category = "LGUI-Selectable")
		FColor PressedColor = FColor(150, 150, 150, 255);
	UPROPERTY(EditAnywhere, Category = "LGUI-Selectable")
		FColor DisabledColor = FColor(150, 150, 150, 128);
	UPROPERTY(EditAnywhere, Category = "LGUI-Selectable", meta = (ClampMin = "0.0"))
		float FadeDuration = 0.2f;

	UPROPERTY(EditAnywhere, Category = "LGUI-Selectable", meta = (DisplayThumbnail = "false"))
		ULGUISpriteData* NormalSprite;
	UPROPERTY(EditAnywhere, Category = "LGUI-Selectable", meta = (DisplayThumbnail = "false"))
		ULGUISpriteData* HighlightedSprite;
	UPROPERTY(EditAnywhere, Category = "LGUI-Selectable", meta = (DisplayThumbnail = "false"))
		ULGUISpriteData* PressedSprite;
	UPROPERTY(EditAnywhere, Category = "LGUI-Selectable", meta = (DisplayThumbnail = "false"))
		ULGUISpriteData* DisabledSprite;

	EUISelectableSelectionState CurrentSelectionState = EUISelectableSelectionState::Normal;
	void ApplySelectionState(bool immediateSet);
	bool IsPointerInsideThis = false;
	bool IsPointerDown = false;
	UPROPERTY(Transient) class UUISelectableTransitionComponent* TransitionComp = nullptr;
#pragma endregion
	UPROPERTY(EditAnywhere, Category = "LGUI-Selectable-Navigation")
		EUISelectableNavigationMode NavigationLeft = EUISelectableNavigationMode::Auto;
	UPROPERTY(EditAnywhere, Category = "LGUI-Selectable-Navigation")
		FLGUIComponentReference NavigationLeftSpecific = FLGUIComponentReference(UUISelectableComponent::StaticClass());
	UPROPERTY(EditAnywhere, Category = "LGUI-Selectable-Navigation")
		EUISelectableNavigationMode NavigationRight = EUISelectableNavigationMode::Auto;
	UPROPERTY(EditAnywhere, Category = "LGUI-Selectable-Navigation")
		FLGUIComponentReference NavigationRightSpecific = FLGUIComponentReference(UUISelectableComponent::StaticClass());
	UPROPERTY(EditAnywhere, Category = "LGUI-Selectable-Navigation")
		EUISelectableNavigationMode NavigationUp = EUISelectableNavigationMode::Auto;
	UPROPERTY(EditAnywhere, Category = "LGUI-Selectable-Navigation")
		FLGUIComponentReference NavigationUpSpecific = FLGUIComponentReference(UUISelectableComponent::StaticClass());
	UPROPERTY(EditAnywhere, Category = "LGUI-Selectable-Navigation")
		EUISelectableNavigationMode NavigationDown = EUISelectableNavigationMode::Auto;
	UPROPERTY(EditAnywhere, Category = "LGUI-Selectable-Navigation")
		FLGUIComponentReference NavigationDownSpecific = FLGUIComponentReference(UUISelectableComponent::StaticClass());
	UPROPERTY(EditAnywhere, Category = "LGUI-Selectable-Navigation")
		EUISelectableNavigationMode NavigationNext = EUISelectableNavigationMode::Auto;
	UPROPERTY(EditAnywhere, Category = "LGUI-Selectable-Navigation")
		FLGUIComponentReference NavigationNextSpecific = FLGUIComponentReference(UUISelectableComponent::StaticClass());
	UPROPERTY(EditAnywhere, Category = "LGUI-Selectable-Navigation")
		EUISelectableNavigationMode NavigationPrev = EUISelectableNavigationMode::Auto;
	UPROPERTY(EditAnywhere, Category = "LGUI-Selectable-Navigation")
		FLGUIComponentReference NavigationPrevSpecific = FLGUIComponentReference(UUISelectableComponent::StaticClass());
public:
	UFUNCTION(BlueprintCallable, Category = "LGUI-Selectable")
		class AUIBaseActor* GetTransitionTarget()const { return TransitionActor; }

	UFUNCTION(BlueprintCallable, Category = "LGUI-Selectable") 
		ULGUISpriteData* GetNormalSprite()const { return NormalSprite; }
	UFUNCTION(BlueprintCallable, Category = "LGUI-Selectable") 
		FColor GetNormalColor()const { return NormalColor; }
	UFUNCTION(BlueprintCallable, Category = "LGUI-Selectable") 
		ULGUISpriteData* GetHighlightedSprite()const { return HighlightedSprite; }
	UFUNCTION(BlueprintCallable, Category = "LGUI-Selectable") 
		FColor GetHighlightedColor()const { return HighlightedColor; }
	UFUNCTION(BlueprintCallable, Category = "LGUI-Selectable") 
		ULGUISpriteData* GetPressedSprite()const { return PressedSprite; }
	UFUNCTION(BlueprintCallable, Category = "LGUI-Selectable") 
		FColor GetPressedColor()const { return PressedColor; }
	UFUNCTION(BlueprintCallable, Category = "LGUI-Selectable") 
		EUISelectableSelectionState GetSelectionState()const;

	UFUNCTION(BlueprintCallable, Category = "LGUI-Selectable")
		void SetTransitionTarget(class AUIBaseActor* value);
	UFUNCTION(BlueprintCallable, Category = "LGUI-Selectable")
		void SetNormalSprite(ULGUISpriteData* NewSprite);
	UFUNCTION(BlueprintCallable, Category = "LGUI-Selectable")
		void SetNormalColor(FColor NewColor);
	UFUNCTION(BlueprintCallable, Category = "LGUI-Selectable")
		void SetHighlightedSprite(ULGUISpriteData* NewSprite);
	UFUNCTION(BlueprintCallable, Category = "LGUI-Selectable")
		void SetHighlightedColor(FColor NewColor);
	UFUNCTION(BlueprintCallable, Category = "LGUI-Selectable")
		void SetPressedSprite(ULGUISpriteData* NewSprite);
	UFUNCTION(BlueprintCallable, Category = "LGUI-Selectable")
		void SetPressedColor(FColor NewColor);
	UFUNCTION(BlueprintCallable, Category = "LGUI-Selectable")
		void SetSelectionState(EUISelectableSelectionState NewState);

	UFUNCTION(BlueprintCallable, Category = "LGUI-Selectable")
		bool IsInteractable()const;

#pragma region Navigation
	UFUNCTION(BlueprintCallable, Category = "LGUI-Selectable-Navigation")
		EUISelectableNavigationMode GetNavigationLeft()const { return NavigationLeft; }
	UFUNCTION(BlueprintCallable, Category = "LGUI-Selectable-Navigation")
		EUISelectableNavigationMode GetNavigationRight()const { return NavigationRight; }
	UFUNCTION(BlueprintCallable, Category = "LGUI-Selectable-Navigation")
		EUISelectableNavigationMode GetNavigationUp()const { return NavigationUp; }
	UFUNCTION(BlueprintCallable, Category = "LGUI-Selectable-Navigation")
		EUISelectableNavigationMode GetNavigationDown()const { return NavigationDown; }
	UFUNCTION(BlueprintCallable, Category = "LGUI-Selectable-Navigation")
		EUISelectableNavigationMode GetNavigationPrev()const { return NavigationPrev; }
	UFUNCTION(BlueprintCallable, Category = "LGUI-Selectable-Navigation")
		EUISelectableNavigationMode GetNavigationNext()const { return NavigationNext; }

	UFUNCTION(BlueprintCallable, Category = "LGUI-Selectable-Navigation")
		UUISelectableComponent* GetNavigationLeftExplicit()const { return NavigationLeftSpecific.GetComponent<UUISelectableComponent>(); }
	UFUNCTION(BlueprintCallable, Category = "LGUI-Selectable-Navigation")
		UUISelectableComponent* GetNavigationRightExplicit()const { return NavigationRightSpecific.GetComponent<UUISelectableComponent>(); }
	UFUNCTION(BlueprintCallable, Category = "LGUI-Selectable-Navigation")
		UUISelectableComponent* GetNavigationUpExplicit()const { return NavigationUpSpecific.GetComponent<UUISelectableComponent>(); }
	UFUNCTION(BlueprintCallable, Category = "LGUI-Selectable-Navigation")
		UUISelectableComponent* GetNavigationDownExplicit()const { return NavigationDownSpecific.GetComponent<UUISelectableComponent>(); }
	UFUNCTION(BlueprintCallable, Category = "LGUI-Selectable-Navigation")
		UUISelectableComponent* GetNavigationPrevExplicit()const { return NavigationPrevSpecific.GetComponent<UUISelectableComponent>(); }
	UFUNCTION(BlueprintCallable, Category = "LGUI-Selectable-Navigation")
		UUISelectableComponent* GetNavigationNextExplicit()const { return NavigationNextSpecific.GetComponent<UUISelectableComponent>(); }

	UFUNCTION(BlueprintCallable, Category = "LGUI-Selectable-Navigation")
		void SetNavigationLeft(EUISelectableNavigationMode value);
	UFUNCTION(BlueprintCallable, Category = "LGUI-Selectable-Navigation")
		void SetNavigationRight(EUISelectableNavigationMode value);
	UFUNCTION(BlueprintCallable, Category = "LGUI-Selectable-Navigation")
		void SetNavigationUp(EUISelectableNavigationMode value);
	UFUNCTION(BlueprintCallable, Category = "LGUI-Selectable-Navigation")
		void SetNavigationDown(EUISelectableNavigationMode value);
	UFUNCTION(BlueprintCallable, Category = "LGUI-Selectable-Navigation")
		void SetNavigationPrev(EUISelectableNavigationMode value);
	UFUNCTION(BlueprintCallable, Category = "LGUI-Selectable-Navigation")
		void SetNavigationNext(EUISelectableNavigationMode value);

	UFUNCTION(BlueprintCallable, Category = "LGUI-Selectable-Navigation")
		void SetNavigationLeftExplicit(UUISelectableComponent* value);
	UFUNCTION(BlueprintCallable, Category = "LGUI-Selectable-Navigation")
		void SetNavigationRightExplicit(UUISelectableComponent* value);
	UFUNCTION(BlueprintCallable, Category = "LGUI-Selectable-Navigation")
		void SetNavigationUpExplicit(UUISelectableComponent* value);
	UFUNCTION(BlueprintCallable, Category = "LGUI-Selectable-Navigation")
		void SetNavigationDownExplicit(UUISelectableComponent* value);
	UFUNCTION(BlueprintCallable, Category = "LGUI-Selectable-Navigation")
		void SetNavigationPrevExplicit(UUISelectableComponent* value);
	UFUNCTION(BlueprintCallable, Category = "LGUI-Selectable-Navigation")
		void SetNavigationNextExplicit(UUISelectableComponent* value);
	/**
	 * Called by input module to navigate on selectables.
	 * @return true if can navigate to other selectable.
	 */
	virtual bool OnNavigate(ELGUINavigationDirection InDirection);
	/**
	 * Find UISelectable component on specific direction.
	 */
	virtual UUISelectableComponent* FindSelectable(FVector InDirection);
	/**
	 * Find UISelectable component inside InParent on specific direction.
	 */
	virtual UUISelectableComponent* FindSelectable(FVector InDirection, USceneComponent* InParent);
	static UUISelectableComponent* FindDefaultSelectable(UObject* WorldContextObject);
	virtual UUISelectableComponent* FindSelectableOnLeft();
	virtual UUISelectableComponent* FindSelectableOnRight();
	virtual UUISelectableComponent* FindSelectableOnUp();
	virtual UUISelectableComponent* FindSelectableOnDown();
	virtual UUISelectableComponent* FindSelectableOnNext();
	virtual UUISelectableComponent* FindSelectableOnPrev();
#pragma endregion
public:
	virtual bool OnPointerEnter_Implementation(ULGUIPointerEventData* eventData)override;
	virtual bool OnPointerExit_Implementation(ULGUIPointerEventData* eventData)override;
	virtual bool OnPointerDown_Implementation(ULGUIPointerEventData* eventData)override;
	virtual bool OnPointerUp_Implementation(ULGUIPointerEventData* eventData)override;
	virtual bool OnPointerSelect_Implementation(ULGUIBaseEventData* eventData)override;
	virtual bool OnPointerDeselect_Implementation(ULGUIBaseEventData* eventData)override;
};
