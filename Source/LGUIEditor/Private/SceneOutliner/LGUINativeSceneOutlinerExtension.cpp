﻿// Copyright 2019-2021 LexLiu. All Rights Reserved.

#include "SceneOutliner/LGUINativeSceneOutlinerExtension.h"
#include "LGUIEditorModule.h"
#include "Editor.h"
#include "LevelEditor.h"
#include "Widgets/Docking/SDockTab.h"
#include "Widgets/Views/STreeView.h"
#include "ISceneOutliner.h"
#include "ActorTreeItem.h"
#include "FolderTreeItem.h"
#include "WorldTreeItem.h"
#include "JsonObjectConverter.h"
#include "SceneOutlinerStandaloneTypes.h"
#include "EditorActorFolders.h"
#include "Core/LGUISettings.h"
#include "GameFramework/Actor.h"
#include "EngineUtils.h"

void ULGUINativeSceneOutlinerExtension::Init()
{
	FEditorDelegates::PreSaveWorld.AddUObject(this, &ULGUINativeSceneOutlinerExtension::OnPreSaveWorld);
	FEditorDelegates::OnMapOpened.AddUObject(this, &ULGUINativeSceneOutlinerExtension::OnMapOpened);
	FEditorDelegates::PreBeginPIE.AddUObject(this, &ULGUINativeSceneOutlinerExtension::OnPreBeginPIE);
	FEditorDelegates::BeginPIE.AddUObject(this, &ULGUINativeSceneOutlinerExtension::OnBeginPIE);
	FEditorDelegates::EndPIE.AddUObject(this, &ULGUINativeSceneOutlinerExtension::OnEndPIE);
}
void ULGUINativeSceneOutlinerExtension::Tick(float DeltaTime)
{
	if (needToRestore)
	{
		delayRestoreTime += DeltaTime;
		if (delayRestoreTime > ULGUIEditorSettings::GetDelayRestoreHierarchyTime())
		{
			RestoreSceneOutlinerState();
			delayRestoreTime = 0;
			needToRestore = false;
		}
	}
}
TStatId ULGUINativeSceneOutlinerExtension::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(ULGUIEditorManagerObject, STATGROUP_Tickables);
}

void ULGUINativeSceneOutlinerExtension::OnPreSaveWorld(uint32 SaveFlags, UWorld* World)
{
	SaveSceneOutlinerState();
}
void ULGUINativeSceneOutlinerExtension::OnMapOpened(const FString& FileName, bool AsTemplate)
{
	SetDelayRestore(true, false);
}
void ULGUINativeSceneOutlinerExtension::OnPreBeginPIE(const bool IsSimulating)
{
	SaveSceneOutlinerState();
}
void ULGUINativeSceneOutlinerExtension::OnBeginPIE(const bool IsSimulating)
{
	SetDelayRestore(false, true);
}
void ULGUINativeSceneOutlinerExtension::OnEndPIE(const bool IsSimulating)
{
	SetDelayRestore(true, false);
}
void ULGUINativeSceneOutlinerExtension::SetDelayRestore(bool RestoreTemporarilyHidden, bool RestoreUseFName)
{
	shouldRestoreTemporarilyHidden = RestoreTemporarilyHidden;
	needToRestore = true;
	delayRestoreTime = 0;
	shouldRestoreUseFNameData = RestoreUseFName;
}

void ULGUINativeSceneOutlinerExtension::SaveSceneOutlinerState()
{
	if (!ULGUIEditorSettings::GetPreserveHierarchyState())return;
	auto storageActor = FindOrCreateDataStorageActor();
	if (!storageActor)return;
	FLevelEditorModule& LevelEditorModule = FModuleManager::GetModuleChecked<FLevelEditorModule>(TEXT("LevelEditor"));
	TSharedPtr<FTabManager> LevelEditorTabManager = LevelEditorModule.GetLevelEditorTabManager();
	storageActor->ExpandedFolderArray.Reset();
	TArray<AActor*> ExpandedActorArray;
	if (LevelEditorTabManager.IsValid())
	{
		TSharedPtr<SDockTab> SceneOutlinerTab = LevelEditorTabManager->FindExistingLiveTab(FTabId("LevelEditorSceneOutliner"));
		if (SceneOutlinerTab.IsValid())
		{
			auto BorderWidget = StaticCastSharedRef<SBorder>(SceneOutlinerTab->GetContent());
			auto SceneOutlinerWidget = StaticCastSharedRef<ISceneOutliner>(BorderWidget->GetContent());
			const auto& TreeView = SceneOutlinerWidget->GetTree();
			TSet<FSceneOutlinerTreeItemPtr> VisitingItems;
			TreeView.GetExpandedItems(VisitingItems);
			for (FSceneOutlinerTreeItemPtr& Item : VisitingItems)
			{
				if (auto ActorTreeItem = Item->CastTo<FActorTreeItem>())
				{
					if (ActorTreeItem->Actor.IsValid())
					{
						ExpandedActorArray.Add(ActorTreeItem->Actor.Get());
					}
				}
				else if (auto FolderTreeItem = Item->CastTo<FFolderTreeItem>())
				{
					storageActor->ExpandedFolderArray.Add(FolderTreeItem->Path);
				}
				else if (auto WorldTreeItem = Item->CastTo<FWorldTreeItem>())
				{
					if (WorldTreeItem->World.IsValid())
					{

					}
				}
			}
		}
	}
	storageActor->TemporarilyHiddenActorArray.Reset();
	storageActor->TemporarilyHiddenSoftActorArray.Reset();
	storageActor->UnexpandedActorArray.Reset();
	storageActor->UnexpandedSoftActorArray.Reset();
	UnexpandedActorArray.Reset();
	if (auto world = GEditor->GetEditorWorldContext().World())
	{
		for (TActorIterator<AActor> ActorItr(world); ActorItr; ++ActorItr)
		{
			if(!IsValid(*ActorItr))continue;
			if (!ExpandedActorArray.Contains(*ActorItr))
			{
				if (storageActor->GetLevel() == (*ActorItr)->GetLevel())
				{
					storageActor->UnexpandedActorArray.Add(*ActorItr);
				}
				else
				{
					storageActor->UnexpandedSoftActorArray.Add(*ActorItr);
				}
				UnexpandedActorArray.Add((*ActorItr)->GetFName());
			}
			if ((*ActorItr)->IsTemporarilyHiddenInEditor())
			{
				if (storageActor->GetLevel() == (*ActorItr)->GetLevel())
				{
					storageActor->TemporarilyHiddenActorArray.Add(*ActorItr);
				}
				else
				{
					storageActor->TemporarilyHiddenSoftActorArray.Add(*ActorItr);
				}
			}
		}
	}
}
ALGUIEditorLevelDataStorageActor* ULGUINativeSceneOutlinerExtension::FindOrCreateDataStorageActor()
{
	ALGUIEditorLevelDataStorageActor* result = nullptr;
	if (auto world = GEditor->GetEditorWorldContext().World())
	{
		auto baseLevel = world->GetLevel(0);
		TArray<ALGUIEditorLevelDataStorageActor*> needToDelete;
		for (TActorIterator<ALGUIEditorLevelDataStorageActor> ActorItr(world); ActorItr; ++ActorItr)
		{
			if (!IsValid(*ActorItr))continue;
			if (ActorItr->GetLevel() == baseLevel)
			{
				if (result == nullptr)
				{
					result = *ActorItr;
				}
				else
				{
					needToDelete.Add(*ActorItr);
				}
			}
		}
		if (needToDelete.Num() > 1)
		{
			auto msg = FString::Printf(TEXT("[ULGUINativeSceneOutlinerExtension::FindOrCreateDataStorageActor]There are %d count of LGUIEditorLevelDataStorageActor, this is weird..."), needToDelete.Num());
			UE_LOG(LGUIEditor, Error, TEXT("%s"), *msg);
			LGUIUtils::EditorNotification(FText::FromString(msg));
			for (auto item : needToDelete)
			{
				item->Destroy();
			}
			needToDelete.Empty();
		}
		if (!result)
		{
			world->SetCurrentLevel(baseLevel);
			result = world->SpawnActor<ALGUIEditorLevelDataStorageActor>();
		}
	}
	return result;
}

void ULGUINativeSceneOutlinerExtension::RestoreSceneOutlinerStateForTreeItem(FSceneOutlinerTreeItemPtr& Item, ALGUIEditorLevelDataStorageActor* storageActor)
{
	if (auto ActorTreeItem = Item->CastTo<FActorTreeItem>())
	{
		if (ActorTreeItem->Actor.IsValid())
		{
			//expend
			if (shouldRestoreUseFNameData)
			{
				bool needToExpand = !UnexpandedActorArray.Contains(ActorTreeItem->Actor->GetFName());
				ActorTreeItem->Flags.bIsExpanded = needToExpand;
			}
			else
			{
				if (storageActor->GetLevel() == ActorTreeItem->Actor->GetLevel())
				{
					bool needToExpand = !storageActor->UnexpandedActorArray.Contains(ActorTreeItem->Actor);
					ActorTreeItem->Flags.bIsExpanded = needToExpand;
				}
				else
				{
					bool needToExpand = !storageActor->UnexpandedSoftActorArray.Contains(ActorTreeItem->Actor.Get());
					ActorTreeItem->Flags.bIsExpanded = needToExpand;
				}
			}
		}
	}
	else if (auto FolderTreeItem = Item->CastTo<FFolderTreeItem>())
	{
		//expend
		bool needToExpand = storageActor->ExpandedFolderArray.Contains(FolderTreeItem->Path);
		FolderTreeItem->Flags.bIsExpanded = needToExpand;
	}
	else if (auto WorldTreeItem = Item->CastTo<FWorldTreeItem>())
	{
		if (WorldTreeItem->World.IsValid())
		{

		}
	}
}

void ULGUINativeSceneOutlinerExtension::RestoreSceneOutlinerState()
{
	if (!ULGUIEditorSettings::GetPreserveHierarchyState())return;
	auto storageActor = FindOrCreateDataStorageActor();
	if (!storageActor)return;
	FLevelEditorModule& LevelEditorModule = FModuleManager::GetModuleChecked<FLevelEditorModule>(TEXT("LevelEditor"));
	TSharedPtr<FTabManager> LevelEditorTabManager = LevelEditorModule.GetLevelEditorTabManager();
	if (LevelEditorTabManager.IsValid())
	{
		TSharedPtr<SDockTab> SceneOutlinerTab = LevelEditorTabManager->FindExistingLiveTab(FTabId("LevelEditorSceneOutliner"));
		if (SceneOutlinerTab.IsValid())
		{
			auto BorderWidget = StaticCastSharedRef<SBorder>(SceneOutlinerTab->GetContent());
			auto SceneOutlinerWidget = StaticCastSharedRef<ISceneOutliner>(BorderWidget->GetContent());
			auto& TreeView = SceneOutlinerWidget->GetTree();
			TSet<FSceneOutlinerTreeItemPtr> VisitingItems;
			TreeView.GetExpandedItems(VisitingItems);
			for (FSceneOutlinerTreeItemPtr& Item : VisitingItems)
			{
				RestoreSceneOutlinerStateForTreeItem(Item, storageActor);
			}
			GEditor->BroadcastLevelActorListChanged();
		}
	}
	if (shouldRestoreTemporarilyHidden)
	{
		//hidden
		if (auto world = GEditor->GetEditorWorldContext().World())
		{
			for (TActorIterator<AActor> ActorItr(world); ActorItr; ++ActorItr)
			{
				if (!IsValid(*ActorItr))continue;
				if (storageActor->GetLevel() == (ActorItr->GetLevel()))
				{
					if (storageActor->TemporarilyHiddenActorArray.Contains((*ActorItr)))
					{
						(*ActorItr)->SetIsTemporarilyHiddenInEditor(true);
					}
				}
				else
				{
					if (storageActor->TemporarilyHiddenSoftActorArray.Contains((*ActorItr)))
					{
						(*ActorItr)->SetIsTemporarilyHiddenInEditor(true);
					}
				}
			}
		}
	}
}
