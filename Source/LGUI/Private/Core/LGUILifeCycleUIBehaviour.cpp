﻿// Copyright 2019-2021 LexLiu. All Rights Reserved.

#include "Core/LGUILifeCycleUIBehaviour.h"
#include "LGUI.h"
#include "Core/Actor/LGUIManagerActor.h"

ULGUILifeCycleUIBehaviour::ULGUILifeCycleUIBehaviour()
{
	PrimaryComponentTick.bCanEverTick = false;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

void ULGUILifeCycleUIBehaviour::BeginPlay()
{
	UActorComponent::BeginPlay();//skip parent's OnComponentRenderStateDirty
	ALGUIManagerActor::AddLGUILifeCycleBehaviourForLifecycleEvent(this);
}

void ULGUILifeCycleUIBehaviour::OnRegister()
{
	Super::OnRegister();
	if (CheckRootUIComponent())
	{
		RootUIComp->AddLGUILifeCycleUIBehaviourComponent(this);
	}
}
void ULGUILifeCycleUIBehaviour::OnUnregister()
{
	Super::OnUnregister();
	if (RootUIComp.IsValid())
	{
		RootUIComp->RemoveLGUILifeCycleUIBehaviourComponent(this);
	}
}
#if WITH_EDITOR
void ULGUILifeCycleUIBehaviour::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	RootUIComp = nullptr;
	CheckRootUIComponent();
}
#endif

bool ULGUILifeCycleUIBehaviour::CheckRootUIComponent() const
{
	if (this->GetWorld() == nullptr)return false;
	if (RootUIComp.IsValid())return true;
	if (auto Owner = GetOwner())
	{
		RootUIComp = Cast<UUIItem>(Owner->GetRootComponent());
		if(RootUIComp.IsValid())return true;
	}
	UE_LOG(LGUI, Error, TEXT("[ULGUILifeCycleUIBehaviour::CheckRootUIComponent]LGUILifeCycleUIBehaviour must attach to a UI actor!"));
	return false;
}

bool ULGUILifeCycleUIBehaviour::GetIsActiveAndEnable()const
{
	if (CheckRootUIComponent())
	{
		return RootUIComp->GetIsUIActiveInHierarchy() && enable;
	}
	else
	{
		return enable;
	}
}
bool ULGUILifeCycleUIBehaviour::IsAllowedToCallOnEnable()const
{
	return GetIsActiveAndEnable();
}

bool ULGUILifeCycleUIBehaviour::IsAllowedToCallAwake()const
{
	if (CheckRootUIComponent())
	{
		return RootUIComp->GetIsUIActiveInHierarchy();
	}
	else
	{
		return true;
	}
}

UUIItem* ULGUILifeCycleUIBehaviour::GetRootUIComponent() const
{
	if (CheckRootUIComponent())
	{
		return RootUIComp.Get();
	}
	return nullptr;
}

void ULGUILifeCycleUIBehaviour::OnUIActiveInHierachy(bool activeOrInactive) 
{ 
	if (activeOrInactive)
	{
		if (!bIsAwakeCalled)
		{
#if WITH_EDITOR
			if (!this->GetWorld()->IsGameWorld())//edit mode
			{

			}
			else
#endif
			{
				Awake();
			}
		}
	}
	SetActiveStateForEnableAndDisable(activeOrInactive);
	if (GetClass()->HasAnyClassFlags(CLASS_CompiledFromBlueprint) || !GetClass()->HasAnyClassFlags(CLASS_Native))
	{
		OnUIActiveInHierarchyBP(activeOrInactive);
	}
}

void ULGUILifeCycleUIBehaviour::Awake()
{
	bIsAwakeCalled = true;
	for (auto CallbackFunc : CallbacksBeforeAwake)
	{
		CallbackFunc();
	}
	Super::Awake();
}

void ULGUILifeCycleUIBehaviour::OnUIDimensionsChanged(bool positionChanged, bool sizeChanged)
{
	if (GetClass()->HasAnyClassFlags(CLASS_CompiledFromBlueprint) || !GetClass()->HasAnyClassFlags(CLASS_Native))
	{
		OnUIDimensionsChangedBP(positionChanged, sizeChanged);
	}
}
void ULGUILifeCycleUIBehaviour::OnUIChildDimensionsChanged(UUIItem* child, bool positionChanged, bool sizeChanged)
{
	if (GetClass()->HasAnyClassFlags(CLASS_CompiledFromBlueprint) || !GetClass()->HasAnyClassFlags(CLASS_Native))
	{
		OnUIChildDimensionsChangedBP(child, positionChanged, sizeChanged);
	}
}
void ULGUILifeCycleUIBehaviour::OnUIChildAcitveInHierarchy(UUIItem* child, bool ativeOrInactive)
{
	if (GetClass()->HasAnyClassFlags(CLASS_CompiledFromBlueprint) || !GetClass()->HasAnyClassFlags(CLASS_Native))
	{
		OnUIChildAcitveInHierarchyBP(child, ativeOrInactive);
	}
}
void ULGUILifeCycleUIBehaviour::OnUIAttachmentChanged()
{ 
	if (GetClass()->HasAnyClassFlags(CLASS_CompiledFromBlueprint) || !GetClass()->HasAnyClassFlags(CLASS_Native))
	{
		OnUIAttachmentChangedBP();
	}
}
void ULGUILifeCycleUIBehaviour::OnUIChildAttachmentChanged(UUIItem* child, bool attachOrDetach) 
{ 
	if (GetClass()->HasAnyClassFlags(CLASS_CompiledFromBlueprint) || !GetClass()->HasAnyClassFlags(CLASS_Native))
	{
		OnUIChildAttachmentChangedBP(child, attachOrDetach);
	}
}
void ULGUILifeCycleUIBehaviour::OnUIInteractionStateChanged(bool interactableOrNot)
{ 
	if (GetClass()->HasAnyClassFlags(CLASS_CompiledFromBlueprint) || !GetClass()->HasAnyClassFlags(CLASS_Native))
	{
		OnUIInteractionStateChangedBP(interactableOrNot);
	}
}
void ULGUILifeCycleUIBehaviour::OnUIChildHierarchyIndexChanged(UUIItem* child)
{ 
	if (GetClass()->HasAnyClassFlags(CLASS_CompiledFromBlueprint) || !GetClass()->HasAnyClassFlags(CLASS_Native))
	{
		OnUIChildHierarchyIndexChangedBP(child);
	}
}


void ULGUILifeCycleUIBehaviour::Call_OnUIDimensionsChanged(bool positionChanged, bool sizeChanged)
{
#if WITH_EDITOR
	if (!this->GetWorld()->IsGameWorld())//edit mode
	{
		OnUIDimensionsChanged(positionChanged, sizeChanged);
	}
	else
#endif
	{
		if (bIsAwakeCalled)
		{
			OnUIDimensionsChanged(positionChanged, sizeChanged);
		}
		else
		{
			auto ThisPtr = MakeWeakObjectPtr(this);
			CallbacksBeforeAwake.Add([=]() {
				if (ThisPtr.IsValid())
				{
					ThisPtr->OnUIDimensionsChanged(positionChanged, sizeChanged);
				}});
		}
	}
}
void ULGUILifeCycleUIBehaviour::Call_OnUIChildDimensionsChanged(UUIItem* child, bool positionChanged, bool sizeChanged)
{
#if WITH_EDITOR
	if (!this->GetWorld()->IsGameWorld())//edit mode
	{
		OnUIChildDimensionsChanged(child, positionChanged, sizeChanged);
	}
	else
#endif
	{
		if (bIsAwakeCalled)
		{
			OnUIChildDimensionsChanged(child, positionChanged, sizeChanged);
		}
		else
		{
			auto ThisPtr = MakeWeakObjectPtr(this);
			CallbacksBeforeAwake.Add([=]() {
				if (ThisPtr.IsValid())
				{
					ThisPtr->OnUIChildDimensionsChanged(child, positionChanged, sizeChanged);
				}});
		}
	}
}
void ULGUILifeCycleUIBehaviour::Call_OnUIChildAcitveInHierarchy(UUIItem* child, bool ativeOrInactive)
{
#if WITH_EDITOR
	if (!this->GetWorld()->IsGameWorld())//edit mode
	{
		OnUIChildAcitveInHierarchy(child, ativeOrInactive);
	}
	else
#endif
	{
		if (bIsAwakeCalled)
		{
			OnUIChildAcitveInHierarchy(child, ativeOrInactive);
		}
		else
		{
			auto ThisPtr = MakeWeakObjectPtr(this);
			CallbacksBeforeAwake.Add([=]() {
				if (ThisPtr.IsValid())
				{
					ThisPtr->OnUIChildAcitveInHierarchy(child, ativeOrInactive);
				}});
		}
	}
}
void ULGUILifeCycleUIBehaviour::Call_OnUIAttachmentChanged()
{
#if WITH_EDITOR
	if (!this->GetWorld()->IsGameWorld())//edit mode
	{
		OnUIAttachmentChanged();
	}
	else
#endif
	{
		if (bIsAwakeCalled)
		{
			OnUIAttachmentChanged();
		}
		else
		{
			auto ThisPtr = MakeWeakObjectPtr(this);
			CallbacksBeforeAwake.Add([=]() {
				if (ThisPtr.IsValid())
				{
					ThisPtr->OnUIAttachmentChanged();
				}});
		}
	}
}
void ULGUILifeCycleUIBehaviour::Call_OnUIChildAttachmentChanged(UUIItem* child, bool attachOrDetach)
{
#if WITH_EDITOR
	if (!this->GetWorld()->IsGameWorld())//edit mode
	{
		OnUIChildAttachmentChanged(child, attachOrDetach);
	}
	else
#endif
	{
		if (bIsAwakeCalled)
		{
			OnUIChildAttachmentChanged(child, attachOrDetach);
		}
		else
		{
			auto ThisPtr = MakeWeakObjectPtr(this);
			CallbacksBeforeAwake.Add([=]() {
				if (ThisPtr.IsValid())
				{
					ThisPtr->OnUIChildAttachmentChanged(child, attachOrDetach);
				}});
		}
	}
}
void ULGUILifeCycleUIBehaviour::Call_OnUIInteractionStateChanged(bool interactableOrNot)
{
#if WITH_EDITOR
	if (!this->GetWorld()->IsGameWorld())//edit mode
	{
		OnUIInteractionStateChanged(interactableOrNot);
	}
	else
#endif
	{
		if (bIsAwakeCalled)
		{
			OnUIInteractionStateChanged(interactableOrNot);
		}
		else
		{
			auto ThisPtr = MakeWeakObjectPtr(this);
			CallbacksBeforeAwake.Add([=]() {
				if (ThisPtr.IsValid())
				{
					ThisPtr->OnUIInteractionStateChanged(interactableOrNot);
				}});
		}
	}
}
void ULGUILifeCycleUIBehaviour::Call_OnUIChildHierarchyIndexChanged(UUIItem* child)
{
#if WITH_EDITOR
	if (!this->GetWorld()->IsGameWorld())//edit mode
	{
		OnUIChildHierarchyIndexChanged(child);
	}
	else
#endif
	{
		if (bIsAwakeCalled)
		{
			OnUIChildHierarchyIndexChanged(child);
		}
		else
		{
			auto ThisPtr = MakeWeakObjectPtr(this);
			CallbacksBeforeAwake.Add([=]() {
				if (ThisPtr.IsValid())
				{
					ThisPtr->OnUIChildHierarchyIndexChanged(child);
				}});
		}
	}
}
