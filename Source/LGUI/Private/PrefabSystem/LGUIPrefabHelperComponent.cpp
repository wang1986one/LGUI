﻿// Copyright 2019-2021 LexLiu. All Rights Reserved.

#include "PrefabSystem/LGUIPrefabHelperComponent.h"
#include "LGUI.h"
#include "PrefabSystem/ActorSerializer.h"
#include "Utils/LGUIUtils.h"
#include "PrefabSystem/LGUIPrefabActor.h"
#include "Core/ActorComponent/UIItem.h"
#if WITH_EDITOR
#include "Editor.h"
#include "EditorActorFolders.h"
#include "Core/Actor/LGUIManagerActor.h"
#endif

#if WITH_EDITORONLY_DATA
FName ULGUIPrefabHelperComponent::PrefabFolderName(TEXT("--LGUIPrefabActor--"));
#endif

ULGUIPrefabHelperComponent::ULGUIPrefabHelperComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
#if WITH_EDITORONLY_DATA
	IdentityColor = FColor::MakeRandomColor();
#endif
}

void ULGUIPrefabHelperComponent::OnRegister()
{
	Super::OnRegister();
}
void ULGUIPrefabHelperComponent::OnUnregister()
{
	Super::OnUnregister();
}

#if WITH_EDITOR
void ULGUIPrefabHelperComponent::MoveActorToPrefabFolder()
{
	FActorFolders::Get().CreateFolder(*this->GetWorld(), PrefabFolderName);
	this->GetOwner()->SetFolderPath(PrefabFolderName);
}
void ULGUIPrefabHelperComponent::LoadPrefab()
{
	if (!LoadedRootActor)
	{
		ULGUIEditorManagerObject::CanExecuteSelectionConvert = false;

		LoadedRootActor = LGUIPrefabSystem::ActorSerializer::LoadPrefabForEdit(this->GetWorld(), PrefabAsset
			, IsValid(ParentActorForEditor) ? ParentActorForEditor->GetRootComponent() 
			: nullptr, AllLoadedActorArray);
		ParentActorForEditor = nullptr;

		FActorFolders::Get().CreateFolder(*this->GetWorld(), PrefabFolderName);
		this->GetOwner()->SetFolderPath(PrefabFolderName);
		GEditor->SelectNone(false, true);
		GEditor->SelectActor(LoadedRootActor, true, false);

		ULGUIEditorManagerObject::CanExecuteSelectionConvert = true;
	}
}
void ULGUIPrefabHelperComponent::SavePrefab()
{
	if (PrefabAsset)
	{
		AActor* Target = nullptr;
		if (GetOwner()->GetClass()->IsChildOf(ALGUIPrefabActor::StaticClass()) && this == GetOwner()->GetRootComponent())//if this component is created from LGUIPrefabActor
		{
			Target = LoadedRootActor;
		}
		else//if this component is attached to Actor, then consider that actor as prefab root
		{
			Target = GetOwner();
		}
		LGUIPrefabSystem::ActorSerializer serializer(this->GetWorld());
		serializer.SerializeActor(Target, PrefabAsset);

		AllLoadedActorArray.Empty();
		LGUIUtils::CollectChildrenActors(Target, AllLoadedActorArray);
	}
	else
	{
		UE_LOG(LGUI, Error, TEXT("PrefabAsset is null, please create a LGUIPrefab asset and assign to PrefabAsset"));
	}
}

void ULGUIPrefabHelperComponent::RevertPrefab()
{
	if (PrefabAsset)
	{
		AActor* OldParentActor = nullptr;
		bool haveRootTransform = true;
		FTransform OldTransform;
		FUIWidget OldWidget;
		//delete loaded actor
		if (IsValid(LoadedRootActor))
		{
			OldParentActor = LoadedRootActor->GetAttachParentActor();
			if(auto RootComp = LoadedRootActor->GetRootComponent())
			{
				haveRootTransform = true;
				OldTransform = LoadedRootActor->GetRootComponent()->GetRelativeTransform();
				if (auto RootUIComp = Cast<UUIItem>(RootComp))
				{
					OldWidget = RootUIComp->GetWidget();
				}
			}
			LGUIUtils::DestroyActorWithHierarchy(LoadedRootActor, true);
			LoadedRootActor = nullptr;
		}
		//create new actor
		LoadPrefab();
		if (IsValid(LoadedRootActor))
		{
			LoadedRootActor->AttachToActor(OldParentActor, FAttachmentTransformRules::KeepRelativeTransform);
			if (haveRootTransform)
			{
				if (auto RootComp = LoadedRootActor->GetRootComponent())
				{
					RootComp->SetRelativeTransform(OldTransform);
					if (auto RootUIItem = Cast<UUIItem>(RootComp))
					{
						auto newWidget = RootUIItem->GetWidget();
						OldWidget.color = newWidget.color;
						OldWidget.depth = newWidget.depth;
						RootUIItem->SetWidget(OldWidget);
					}
				}
			}
		}
	}
	else
	{
		UE_LOG(LGUI, Error, TEXT("PrefabAsset is null, please create a LGUIPrefab asset and assign to PrefabAsset"));
	}
}

void ULGUIPrefabHelperComponent::DeleteThisInstance()
{
	if (LoadedRootActor)
	{
#if WITH_EDITOR
		GEditor->RedrawAllViewports();
#endif
		LGUIUtils::DestroyActorWithHierarchy(LoadedRootActor, true);
		LGUIUtils::DestroyActorWithHierarchy(GetOwner(), false);
	}
	else
	{
		UE_LOG(LGUI, Error, TEXT("This actor is not a prefab, ignore this action"));
	}
}
#endif