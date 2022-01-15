// Copyright 2019-2022 LexLiu. All Rights Reserved.

#include "Core/Actor/LGUIManagerActor.h"
#include "LGUI.h"
#include "Utils/LGUIUtils.h"
#include "Core/ActorComponent/UIItem.h"
#include "Core/ActorComponent/UIText.h"
#include "Core/ActorComponent/LGUICanvas.h"
#include "Event/LGUIBaseInteractionComponent.h"
#include "Engine/World.h"
#include "Interaction/UISelectableComponent.h"
#include "Core/LGUISettings.h"
#include "Event/InputModule/LGUIBaseInputModule.h"
#include "Core/Actor/UIBaseActor.h"
#include "Core/ActorComponent/UIBatchGeometryRenderable.h"
#include "Core/ActorComponent/UIBaseRenderable.h"
#include "Core/ActorComponent/UIPostProcessRenderable.h"
#include "Engine/Engine.h"
#include "Core/HudRender/LGUIRenderer.h"
#include "Core/ILGUICultureChangedInterface.h"
#include "Core/LGUILifeCycleBehaviour.h"
#include "Layout/ILGUILayoutInterface.h"
#if WITH_EDITOR
#include "Editor.h"
#include "DrawDebugHelpers.h"
#include "Engine/Selection.h"
#include "EditorViewportClient.h"
#include "PrefabSystem/LGUIPrefabHelperActor.h"
#include "PrefabSystem/LGUIPrefabHelperObject.h"
#include "PrefabSystem/LGUIPrefab.h"
#include "EngineUtils.h"
#include "Layout/LGUICanvasScaler.h"
#endif

#define LOCTEXT_NAMESPACE "LGUIManagerObject"

PRAGMA_DISABLE_OPTIMIZATION

ULGUIEditorManagerObject* ULGUIEditorManagerObject::Instance = nullptr;
ULGUIEditorManagerObject::ULGUIEditorManagerObject()
{

}
void ULGUIEditorManagerObject::BeginDestroy()
{
#if WITH_EDITORONLY_DATA
	if (OnAssetReimportDelegateHandle.IsValid())
	{
		GEditor->GetEditorSubsystem<UImportSubsystem>()->OnAssetReimport.Remove(OnAssetReimportDelegateHandle);
	}
	if (OnActorLabelChangedDelegateHandle.IsValid())
	{
		FCoreDelegates::OnActorLabelChanged.Remove(OnActorLabelChangedDelegateHandle);
	}
	if (OnActorDeletedDelegateHandle.IsValid())
	{
		FEditorDelegates::OnDeleteActorsEnd.Remove(OnActorDeletedDelegateHandle);
	}
	if (OnMapOpenedDelegateHandle.IsValid())
	{
		FEditorDelegates::OnMapOpened.Remove(OnMapOpenedDelegateHandle);
	}
	if (OnBlueprintCompiledDelegateHandle.IsValid())
	{
		GEditor->OnBlueprintCompiled().Remove(OnBlueprintCompiledDelegateHandle);
	}
#endif
	Instance = nullptr;
	Super::BeginDestroy();
}

void ULGUIEditorManagerObject::Tick(float DeltaTime)
{
#if WITH_EDITORONLY_DATA
	if (OneShotFunctionsToExecuteInTick.Num() > 0)
	{
		for (int i = 0; i < OneShotFunctionsToExecuteInTick.Num(); i++)
		{
			auto& Item = OneShotFunctionsToExecuteInTick[i];
			if (Item.Key <= 0)
			{
				Item.Value();
				OneShotFunctionsToExecuteInTick.RemoveAt(i);
				i--;
			}
			else
			{
				Item.Key--;
			}
		}
	}

	//draw frame
	for (auto& item : AllUIItemArray)
	{
		if (!item.IsValid())continue;
		if (!IsValid(item->GetWorld()))continue;
		if (
			item->GetWorld()->WorldType != EWorldType::Editor//actually, ULGUIEditorManagerObject only collect editor mode UIItem, so only this Editor condition will trigger.
															//so only Editor mode will draw frame. the modes below will not work, just leave it as a reference.
			&& item->GetWorld()->WorldType != EWorldType::Game
			&& item->GetWorld()->WorldType != EWorldType::PIE
			&& item->GetWorld()->WorldType != EWorldType::EditorPreview
			)continue;

		ULGUIEditorManagerObject::DrawFrameOnUIItem(item.Get());
	}

	bool canUpdateLayout = true;
	if (!GetDefault<ULGUIEditorSettings>()->AnchorControlPosition)
	{
		canUpdateLayout = false;
	}

	if (canUpdateLayout)
	{
		for (auto& item : AllLayoutArray)
		{
			ILGUILayoutInterface::Execute_OnUpdateLayout(item.GetObject());
		}
	}
	
	int ScreenSpaceOverlayCanvasCount = 0;
	for (auto& item : AllCanvasArray)
	{
		check(item.IsValid());
		if (item.IsValid())
		{
			if (item->IsRootCanvas())
			{
				if (item->GetWorld() == GWorld)
				{
					if (item->GetActualRenderMode() == ELGUIRenderMode::ScreenSpaceOverlay)
					{
						ScreenSpaceOverlayCanvasCount++;
					}
				}
			}
		}
	}
	for (auto& item : AllCanvasArray)
	{
		if (item.IsValid())
		{
			item->UpdateCanvas();
		}
	}

	if (ScreenSpaceOverlayCanvasCount > 1)
	{
		if (PrevScreenSpaceOverlayCanvasCount != ScreenSpaceOverlayCanvasCount)//only show message when change
		{
			PrevScreenSpaceOverlayCanvasCount = ScreenSpaceOverlayCanvasCount;
			auto errMsg = LOCTEXT("MultipleLGUICanvasRenderScreenSpaceOverlay", "Detect multiply LGUICanvas renderred with ScreenSpaceOverlay mode, this is not allowed! There should be only one ScreenSpace UI in a world!");
			UE_LOG(LGUI, Error, TEXT("%s"), *errMsg.ToString());
			LGUIUtils::EditorNotification(errMsg, 10.0f);
		}
	}
	else
	{
		PrevScreenSpaceOverlayCanvasCount = 0;
	}

	if (EditorTick.IsBound())
	{
		EditorTick.Broadcast(DeltaTime);
	}

	if (AllCanvasArray.Num() > 0)
	{
		if (bShouldSortLGUIRenderer || bShouldSortWorldSpaceCanvas || bShouldSortRenderTargetSpaceCanvas)
		{
			AllCanvasArray.Sort([](const TWeakObjectPtr<ULGUICanvas>& A, const TWeakObjectPtr<ULGUICanvas>& B)
				{
					return A->GetActualSortOrder() < B->GetActualSortOrder();
				});
		}
		if (bShouldSortLGUIRenderer)
		{
			SortDrawcallOnRenderMode(ELGUIRenderMode::ScreenSpaceOverlay);
			SortDrawcallOnRenderMode(ELGUIRenderMode::WorldSpace_LGUI);
			if (ScreenSpaceOverlayViewExtension.IsValid())
			{
				ScreenSpaceOverlayViewExtension->SortPrimitiveRenderPriority();
			}
		}
		if (bShouldSortWorldSpaceCanvas)
		{
			SortDrawcallOnRenderMode(ELGUIRenderMode::WorldSpace);
		}
		if (bShouldSortRenderTargetSpaceCanvas)
		{
			SortDrawcallOnRenderMode(ELGUIRenderMode::RenderTarget);
		}

		bShouldSortLGUIRenderer = false;
		bShouldSortWorldSpaceCanvas = false;
		bShouldSortRenderTargetSpaceCanvas = false;
	}
#endif
#if WITH_EDITOR
	CheckEditorViewportIndexAndKey();
#endif
}
TStatId ULGUIEditorManagerObject::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(ULGUIEditorManagerObject, STATGROUP_Tickables);
}

#if WITH_EDITOR
TArray<TTuple<int, TFunction<void()>>> ULGUIEditorManagerObject::OneShotFunctionsToExecuteInTick;
void ULGUIEditorManagerObject::AddPrefabForGenerateAgent(ULGUIPrefab* InPrefab)
{
	TTuple<int, TFunction<void()>> Item;
	Item.Key = 0;
	Item.Value = [Prefab = MakeWeakObjectPtr(InPrefab)]() {
		if (Prefab.IsValid())
		{
			Prefab->RefreshAgentObjectsInPreviewWorld();
		}
	};
	OneShotFunctionsToExecuteInTick.Add(Item);
}

void ULGUIEditorManagerObject::AddOneShotTickFunction(TFunction<void()> InFunction, int InDelayFrameCount)
{
	TTuple<int, TFunction<void()>> Item;
	Item.Key = InDelayFrameCount;
	Item.Value = InFunction;
	OneShotFunctionsToExecuteInTick.Add(Item);
}

ULGUIEditorManagerObject* ULGUIEditorManagerObject::GetInstance(UWorld* InWorld, bool CreateIfNotValid)
{
	if (CreateIfNotValid)
	{
		InitCheck(InWorld);
	}
	return Instance;
}
bool ULGUIEditorManagerObject::InitCheck(UWorld* InWorld)
{
	if (Instance == nullptr)
	{
		if (IsValid(InWorld))
		{
			if (InWorld->IsGameWorld())
			{
				auto msg = LOCTEXT("CreateLGUIEditorManagerObjectInGameMode", "[ULGUIEditorManagerObject::InitCheck]Trying to create a LGUIEditorManagerObject in game mode, this is not allowed!");
				UE_LOG(LGUI, Error, TEXT("%s"), *msg.ToString());
				LGUIUtils::EditorNotification(msg);
				return nullptr;
			}
			
			Instance = NewObject<ULGUIEditorManagerObject>();
			Instance->AddToRoot();
			UE_LOG(LGUI, Log, TEXT("[ULGUIManagerObject::InitCheck]No Instance for LGUIManagerObject, create!"));
			Instance->OnActorLabelChangedDelegateHandle = FCoreDelegates::OnActorLabelChanged.AddUObject(Instance, &ULGUIEditorManagerObject::OnActorLabelChanged);
			//reimport asset
			Instance->OnAssetReimportDelegateHandle = GEditor->GetEditorSubsystem<UImportSubsystem>()->OnAssetReimport.AddUObject(Instance, &ULGUIEditorManagerObject::OnAssetReimport);
			//delete actor
			Instance->OnActorDeletedDelegateHandle = FEditorDelegates::OnDeleteActorsEnd.AddUObject(Instance, &ULGUIEditorManagerObject::OnActorDeleted);
			//open map
			Instance->OnMapOpenedDelegateHandle = FEditorDelegates::OnMapOpened.AddUObject(Instance, &ULGUIEditorManagerObject::OnMapOpened);
			//blueprint recompile
			Instance->OnBlueprintCompiledDelegateHandle = GEditor->OnBlueprintCompiled().AddUObject(Instance, &ULGUIEditorManagerObject::RefreshOnBlueprintCompiled);

			GeneratePrefabAgentInPreviewWorld();
		}
		else
		{
			return false;
		}
	}
	return true;
}

void ULGUIEditorManagerObject::RefreshOnBlueprintCompiled()
{
	
}

void ULGUIEditorManagerObject::SortDrawcallOnRenderMode(ELGUIRenderMode InRenderMode)
{
	int32 RenderPriority = 0;
	int32 prevSortOrder = INT_MIN;
	int32 prevCanvasDrawcallCount = 0;//prev Canvas's drawcall count
	for (int i = 0; i < AllCanvasArray.Num(); i++)
	{
		auto canvasItem = this->AllCanvasArray[i];
		if (canvasItem.IsValid() && canvasItem->GetIsUIActive() && !canvasItem->IsRenderByOtherCanvas())
		{
			if (canvasItem->GetActualRenderMode() == InRenderMode)
			{
				canvasItem->SortDrawcall(RenderPriority);
			}
		}
	}
}
void ULGUIEditorManagerObject::MarkSortLGUIRenderer()
{
	bShouldSortLGUIRenderer = true;
}
void ULGUIEditorManagerObject::MarkSortWorldSpaceCanvas()
{
	bShouldSortWorldSpaceCanvas = true;
}
void ULGUIEditorManagerObject::MarkSortRenderTargetSpaceCanvas()
{
	bShouldSortRenderTargetSpaceCanvas = true;
}

TSharedPtr<class FLGUIHudRenderer, ESPMode::ThreadSafe> ULGUIEditorManagerObject::GetViewExtension(UWorld* InWorld, bool InCreateIfNotExist)
{
	if (Instance != nullptr)
	{
		if (!Instance->ScreenSpaceOverlayViewExtension.IsValid())
		{
			if (InCreateIfNotExist)
			{
				Instance->ScreenSpaceOverlayViewExtension = FSceneViewExtensions::NewExtension<FLGUIHudRenderer>(InWorld, nullptr);
			}
		}
		else
		{
			if (!Instance->ScreenSpaceOverlayViewExtension->GetWorld().IsValid())
			{
				Instance->ScreenSpaceOverlayViewExtension.Reset();
				if (InCreateIfNotExist)
				{
					Instance->ScreenSpaceOverlayViewExtension = FSceneViewExtensions::NewExtension<FLGUIHudRenderer>(InWorld, nullptr);
				}
			}
		}
		return Instance->ScreenSpaceOverlayViewExtension;
	}
	return nullptr;
}

void ULGUIEditorManagerObject::OnAssetReimport(UObject* asset)
{
	if (IsValid(asset))
	{
		auto textureAsset = Cast<UTexture2D>(asset);
		if (IsValid(textureAsset))
		{
			bool needToRebuildUI = false;
			//find sprite data that reference this texture
			for (TObjectIterator<ULGUISpriteData> Itr; Itr; ++Itr)
			{
				ULGUISpriteData* spriteData = *Itr;
				if (IsValid(spriteData))
				{
					if (spriteData->GetSpriteTexture() == textureAsset)
					{
						spriteData->ReloadTexture();
						spriteData->MarkPackageDirty();
						needToRebuildUI = true;
					}
				}
			}
			//Refresh ui
			if (needToRebuildUI)
			{
				RefreshAllUI();
			}
		}
	}
}

void ULGUIEditorManagerObject::OnActorDeleted()
{
	if (GWorld == nullptr)return;
	for (TActorIterator<ALGUIPrefabHelperActor> ActorItr(GWorld); ActorItr; ++ActorItr)
	{
		auto prefabActor = *ActorItr;
		if (IsValid(prefabActor))
		{
			if (!IsValid(prefabActor->PrefabHelperObject->LoadedRootActor))
			{
				LGUIUtils::DestroyActorWithHierarchy(prefabActor, false);
			}
		}
	}
}
void ULGUIEditorManagerObject::OnMapOpened(const FString& FileName, bool AsTemplate)
{

}

UWorld* ULGUIEditorManagerObject::PreviewWorldForPrefabPackage = nullptr;
UWorld* ULGUIEditorManagerObject::GetPreviewWorldForPrefabPackage()
{
	if (PreviewWorldForPrefabPackage == nullptr)
	{
		FName UniqueWorldName = MakeUniqueObjectName(GetTransientPackage(), UWorld::StaticClass(), FName("LGUI_PreviewWorldForPrefabPackage"));
		PreviewWorldForPrefabPackage = NewObject<UWorld>(GetTransientPackage(), UniqueWorldName);
		PreviewWorldForPrefabPackage->AddToRoot();
		PreviewWorldForPrefabPackage->WorldType = EWorldType::EditorPreview;

		FWorldContext& WorldContext = GEngine->CreateNewWorldContext(PreviewWorldForPrefabPackage->WorldType);
		WorldContext.SetCurrentWorld(PreviewWorldForPrefabPackage);

		PreviewWorldForPrefabPackage->InitializeNewWorld(UWorld::InitializationValues()
			.AllowAudioPlayback(false)
			.CreatePhysicsScene(false)
			.RequiresHitProxies(false)
			.CreateNavigation(false)
			.CreateAISystem(false)
			.ShouldSimulatePhysics(false)
			.SetTransactional(false));
	}
	return PreviewWorldForPrefabPackage;
}

#include "AssetRegistryModule.h"
void ULGUIEditorManagerObject::GeneratePrefabAgentInPreviewWorld()
{
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(FName("AssetRegistry"));
	IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

	// Need to do this if running in the editor with -game to make sure that the assets in the following path are available
	TArray<FString> PathsToScan;
	PathsToScan.Add(TEXT("/Game/"));
	AssetRegistry.ScanPathsSynchronous(PathsToScan);

	// Get asset in path
	TArray<FAssetData> ScriptAssetList;
	AssetRegistry.GetAssetsByPath(FName("/Game/"), ScriptAssetList, /*bRecursive=*/true);

	// Ensure all assets are loaded
	for (const FAssetData& Asset : ScriptAssetList)
	{
		// Gets the loaded asset, loads it if necessary
		if (Asset.AssetClass == TEXT("LGUIPrefab"))
		{
			auto AssetObject = Asset.GetAsset();
			if (auto Prefab = Cast<ULGUIPrefab>(AssetObject))
			{
				Prefab->MakeAgentObjectsInPreviewWorld();
			}
		}
	}
}

void ULGUIEditorManagerObject::OnActorLabelChanged(AActor* actor)
{
	if (auto rootComp = actor->GetRootComponent())
	{
		if (auto rootUIComp = Cast<UUIItem>(rootComp))
		{
			auto actorLabel = actor->GetActorLabel();
			if (actorLabel.StartsWith("//"))
			{
				actorLabel = actorLabel.Right(actorLabel.Len() - 2);
			}
			rootUIComp->SetDisplayName(actorLabel);

			LGUIUtils::NotifyPropertyChanged(rootUIComp, FName(TEXT("displayName")));
		}
	}
}

void ULGUIEditorManagerObject::RefreshAllUI()
{
	struct Local
	{
		static void UpdateComponentToWorldRecursive(UUIItem* UIItem)
		{
			UIItem->CalculateTransformFromAnchor();
			UIItem->UpdateComponentToWorld();
			auto& Children = UIItem->GetAttachUIChildren();
			for (auto& Child : Children)
			{
				UpdateComponentToWorldRecursive(Child);
			}
		}
	};

	if (Instance != nullptr)
	{
		for (auto& Layout : Instance->AllLayoutArray)
		{
			if (Layout.GetObject() != nullptr)
			{
				ILGUILayoutInterface::Execute_MarkRebuildLayout(Layout.GetObject());
			}
		}
		for (auto& RootUIItem : Instance->AllRootUIItemArray)
		{
			if (RootUIItem.IsValid())
			{
				RootUIItem->MarkAllDirtyRecursive();
				Local::UpdateComponentToWorldRecursive(RootUIItem.Get());
				RootUIItem->EditorForceUpdate();
			}
		}
		for (auto& CanvasItem : Instance->AllCanvasArray)
		{
			CanvasItem->EnsureDrawcallObjectReference();
		}
	}
}

bool ULGUIEditorManagerObject::IsSelected(AActor* InObject)
{
	if (!IsValid(GEditor))return false;
	for (FSelectionIterator itr(GEditor->GetSelectedActorIterator()); itr; ++itr)
	{
		if (*itr != nullptr)
		{
			auto itrActor = Cast<AActor>(*itr);
			if (itrActor == InObject)
			{
				return true;
			}
		}
	}
	return false;
}

bool ULGUIEditorManagerObject::AnySelectedIsChildOf(AActor* InObject)
{
	if (!IsValid(GEditor))return false;
	for (FSelectionIterator itr(GEditor->GetSelectedActorIterator()); itr; ++itr)
	{
		auto itrActor = Cast<AActor>(*itr);
		if (itrActor->IsAttachedTo(InObject))
		{
			return true;
		}
	}
	return false;
}

void ULGUIEditorManagerObject::AddUIItem(UUIItem* InItem)
{
	if (InitCheck(InItem->GetWorld()))
	{
#if !UE_BUILD_SHIPPING
		check(!Instance->AllUIItemArray.Contains(InItem));
#endif
		Instance->AllUIItemArray.Add(InItem);
	}
}
void ULGUIEditorManagerObject::RemoveUIItem(UUIItem* InItem)
{
	if (Instance != nullptr)
	{
#if !UE_BUILD_SHIPPING
		check(Instance->AllUIItemArray.Contains(InItem));
#endif
		Instance->AllUIItemArray.RemoveSingle(InItem);
	}
}

void ULGUIEditorManagerObject::RemoveCanvas(ULGUICanvas* InCanvas)
{
	if (Instance != nullptr)
	{
#if !UE_BUILD_SHIPPING
		check(Instance->AllCanvasArray.Contains(InCanvas));
#endif
		Instance->AllCanvasArray.RemoveSingle(InCanvas);
	}
}
void ULGUIEditorManagerObject::AddCanvas(ULGUICanvas* InCanvas)
{
	if (GetInstance(InCanvas->GetWorld(), true))
	{
#if !UE_BUILD_SHIPPING
		check(!Instance->AllCanvasArray.Contains(InCanvas));
#endif
		Instance->AllCanvasArray.AddUnique(InCanvas);
	}
}

void ULGUIEditorManagerObject::AddRootUIItem(UUIItem* InItem)
{
	if (GetInstance(InItem->GetWorld(), true))
	{
#if !UE_BUILD_SHIPPING
		check(!Instance->AllRootUIItemArray.Contains(InItem));
#endif
		Instance->AllRootUIItemArray.AddUnique(InItem);
	}
}
void ULGUIEditorManagerObject::RemoveRootUIItem(UUIItem* InItem)
{
	if (Instance != nullptr)
	{
#if !UE_BUILD_SHIPPING
		check(Instance->AllRootUIItemArray.Contains(InItem));
#endif
		Instance->AllRootUIItemArray.RemoveSingle(InItem);
	}
}

void ULGUIEditorManagerObject::RegisterLGUILayout(TScriptInterface<ILGUILayoutInterface> InItem)
{
	if (InitCheck(InItem.GetObject()->GetWorld()))
	{
#if !UE_BUILD_SHIPPING
		check(!Instance->AllLayoutArray.Contains(InItem));
#endif
		Instance->AllLayoutArray.AddUnique(InItem);
	}
}
void ULGUIEditorManagerObject::UnregisterLGUILayout(TScriptInterface<ILGUILayoutInterface> InItem)
{
	if (Instance != nullptr)
	{
#if !UE_BUILD_SHIPPING
		check(Instance->AllLayoutArray.Contains(InItem));
#endif
		Instance->AllLayoutArray.RemoveSingle(InItem);
	}
}

void ULGUIEditorManagerObject::DrawFrameOnUIItem(UUIItem* item)
{
	auto RectExtends = FVector(0.1f, item->GetWidth(), item->GetHeight()) * 0.5f;
	auto GeometryBoundsExtends = FVector(0, 0, 0);
	bool bCanDrawRect = false;
	auto RectDrawColor = FColor(128, 128, 128, 128);//gray means normal object
	if (ULGUIEditorManagerObject::IsSelected(item->GetOwner()))//select self
	{
		RectDrawColor = FColor(0, 255, 0, 255);//green means selected object
		RectExtends.X = 1;
		bCanDrawRect = true;

		if (auto UIRenderable = Cast<UUIBaseRenderable>(item))
		{
			FVector2D Min, Max;
			UIRenderable->GetGeometryBoundsInLocalSpace(Min, Max);
			auto WorldTransform = item->GetComponentTransform();
			FVector Center = FVector(0, (Max.X + Min.X) * 0.5f, (Max.Y + Min.Y) * 0.5f);
			auto WorldLocation = WorldTransform.TransformPosition(Center);

			auto GeometryBoundsDrawColor = FColor(255, 255, 0, 255);//yellow for geometry bounds
			GeometryBoundsExtends = FVector(0.1f, (Max.X - Min.X) * 0.5f, (Max.Y - Min.Y) * 0.5f);
			DrawDebugBox(item->GetWorld(), WorldLocation, GeometryBoundsExtends * WorldTransform.GetScale3D(), WorldTransform.GetRotation(), GeometryBoundsDrawColor);
		}
	}
	else
	{
		//parent selected
		if (IsValid(item->GetParentUIItem()))
		{
			if (ULGUIEditorManagerObject::IsSelected(item->GetParentUIItem()->GetOwner()))
			{
				bCanDrawRect = true;
			}
		}
		//child selected
		auto& childrenCompArray = item->GetAttachUIChildren();
		for (auto& uiComp : childrenCompArray)
		{
			if (IsValid(uiComp) && IsValid(uiComp->GetOwner()) && ULGUIEditorManagerObject::IsSelected(uiComp->GetOwner()))
			{
				bCanDrawRect = true;
				break;
			}
		}
		//other object of same hierarchy is selected
		if (IsValid(item->GetParentUIItem()))
		{
			const auto& sameLevelCompArray = item->GetParentUIItem()->GetAttachUIChildren();
			for (auto& uiComp : sameLevelCompArray)
			{
				if (IsValid(uiComp) && IsValid(uiComp->GetOwner()) && ULGUIEditorManagerObject::IsSelected(uiComp->GetOwner()))
				{
					bCanDrawRect = true;
					break;
				}
			}
		}
	}
	//canvas scaler
	if (!bCanDrawRect)
	{
		if (item->IsCanvasUIItem())
		{
			if (auto canvasScaler = item->GetOwner()->FindComponentByClass<ULGUICanvasScaler>())
			{
				if (ULGUIEditorManagerObject::AnySelectedIsChildOf(item->GetOwner()))
				{
					bCanDrawRect = true;
					RectDrawColor = FColor(255, 227, 124);
				}
			}
		}
	}

	if (bCanDrawRect)
	{
		auto WorldTransform = item->GetComponentTransform();
		FVector RelativeOffset(0, 0, 0);
		RelativeOffset.Y = (0.5f - item->GetPivot().X) * item->GetWidth();
		RelativeOffset.Z = (0.5f - item->GetPivot().Y) * item->GetHeight();
		auto WorldLocation = WorldTransform.TransformPosition(RelativeOffset);

		DrawDebugBox(item->GetWorld(), WorldLocation, RectExtends * WorldTransform.GetScale3D(), WorldTransform.GetRotation(), RectDrawColor);//@todo: screen-space UI should draw on screen-space, but no clue to achieve that
	}
}


void ULGUIEditorManagerObject::CheckEditorViewportIndexAndKey()
{
	if (!IsValid(GEditor))return;
	auto& viewportClients = GEditor->GetAllViewportClients();
	if (PrevEditorViewportCount != viewportClients.Num())
	{
		PrevEditorViewportCount = viewportClients.Num();
		EditorViewportIndexToKeyMap.Reset();
		for (FEditorViewportClient* viewportClient : viewportClients)
		{
			auto viewKey = viewportClient->ViewState.GetReference()->GetViewKey();
			EditorViewportIndexToKeyMap.Add(viewportClient->ViewIndex, viewKey);
		}

		if (EditorViewportIndexAndKeyChange.IsBound())
		{
			EditorViewportIndexAndKeyChange.Broadcast();
		}
	}

	if (auto viewport = GEditor->GetActiveViewport())
	{
		if (auto viewportClient = viewport->GetClient())
		{
			if (ULGUIEditorManagerObject::Instance != nullptr)
			{
				auto editorViewportClient = (FEditorViewportClient*)viewportClient;
				CurrentActiveViewportIndex = editorViewportClient->ViewIndex;
				CurrentActiveViewportKey = ULGUIEditorManagerObject::Instance->GetViewportKeyFromIndex(editorViewportClient->ViewIndex);
			}
		}
	}
}
uint32 ULGUIEditorManagerObject::GetViewportKeyFromIndex(int32 InViewportIndex)
{
	if (auto key = EditorViewportIndexToKeyMap.Find(InViewportIndex))
	{
		return *key;
	}
	return 0;
}



void ULGUIEditorManagerObject::BeginPrefabSystemProcessingActor(UWorld* InWorld)
{
	if (InitCheck(InWorld))
	{
		//Instance->AllActors_PrefabSystemProcessing.Reset();//@todo: better to use a stack 
	}
}
void ULGUIEditorManagerObject::EndPrefabSystemProcessingActor()
{
	if (Instance != nullptr)
	{
		//Instance->AllActors_PrefabSystemProcessing.Reset();//@todo: better to use a stack 
	}
}
void ULGUIEditorManagerObject::AddActorForPrefabSystem(AActor* InActor)
{
	if (Instance != nullptr)
	{
		Instance->AllActors_PrefabSystemProcessing.AddUnique(InActor);
	}
}
void ULGUIEditorManagerObject::RemoveActorForPrefabSystem(AActor* InActor)
{
	if (Instance != nullptr)
	{
		Instance->AllActors_PrefabSystemProcessing.RemoveSingle(InActor);
	}
}
bool ULGUIEditorManagerObject::IsPrefabSystemProcessingActor(AActor* InActor)
{
	if (Instance != nullptr)
	{
		return Instance->AllActors_PrefabSystemProcessing.Contains(InActor);
	}
	return false;
}

bool ULGUIEditorManagerObject::RaycastHitUI(UWorld* InWorld, const TArray<TWeakObjectPtr<UUIItem>>& InUIItems, const FVector& LineStart, const FVector& LineEnd
	, UUIBaseRenderable*& ResultSelectTarget
)
{
	TArray<FHitResult> HitResultArray;
	for (auto& uiItem : InUIItems)
	{
		if (uiItem->GetWorld() == InWorld)
		{
			if (auto uiRenderable = Cast<UUIBaseRenderable>(uiItem))
			{
				FHitResult hitInfo;
				auto OriginRaycastType = uiRenderable->GetRaycastType();
				auto OriginRaycastTarget = uiRenderable->IsRaycastTarget();
				uiRenderable->SetRaycastType(EUIRenderableRaycastType::Geometry);//in editor selection, make the ray hit actural triangle
				uiRenderable->SetRaycastTarget(true);
				if (uiRenderable->LineTraceUI(hitInfo, LineStart, LineEnd))
				{
					if (uiRenderable->GetRenderCanvas()->IsPointVisible(hitInfo.Location))
					{
						HitResultArray.Add(hitInfo);
					}
				}
				uiRenderable->SetRaycastType(OriginRaycastType);
				uiRenderable->SetRaycastTarget(OriginRaycastTarget);
			}
		}
	}
	if (HitResultArray.Num() > 0)//hit something
	{
		HitResultArray.Sort([](const FHitResult& A, const FHitResult& B)
			{
				auto AUIRenderable = (UUIBaseRenderable*)(A.Component.Get());
				auto BUIRenderable = (UUIBaseRenderable*)(B.Component.Get());
				if (AUIRenderable->GetRenderCanvas() == BUIRenderable->GetRenderCanvas())//if Canvas's depth is equal then sort on item's depth
				{
					return AUIRenderable->GetFlattenHierarchyIndex() > BUIRenderable->GetFlattenHierarchyIndex();
				}
				else//if Canvas's depth not equal then sort on Canvas's SortOrder
				{
					return AUIRenderable->GetRenderCanvas()->GetSortOrder() > BUIRenderable->GetRenderCanvas()->GetSortOrder();
				}
			});
		if (auto uiRenderableComp = Cast<UUIBaseRenderable>(HitResultArray[0].Component.Get()))//target need to select
		{
			ResultSelectTarget = uiRenderableComp;
			return true;
		}
	}
	return false;
}

#endif



TMap<UWorld*, ALGUIManagerActor*> ALGUIManagerActor::WorldToInstanceMap ;
ALGUIManagerActor::ALGUIManagerActor()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickGroup = ETickingGroup::TG_DuringPhysics;
	SpawnCollisionHandlingMethod = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
#if WITH_EDITOR
	FEditorDelegates::BeginPIE.AddLambda([=](const bool isSimulating) {
		for (auto& keyValue : WorldToInstanceMap)
		{
			if (IsValid(keyValue.Value))
			{
				LGUIUtils::DestroyActorWithHierarchy(keyValue.Value);//delete any instance before begin play
			}
		}
	});
#endif
}
ALGUIManagerActor* ALGUIManagerActor::GetLGUIManagerActorInstance(UObject* WorldContextObject)
{
	auto world = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);
	if (auto result = WorldToInstanceMap.Find(world))
	{
		return *result;
	}
	else
	{
		return nullptr;
	}
}
#if WITH_EDITORONLY_DATA
bool ALGUIManagerActor::IsPlaying = false;
#endif
void ALGUIManagerActor::BeginPlay()
{
	Super::BeginPlay();
#if WITH_EDITORONLY_DATA
	IsPlaying = true;
#endif
	//localization
	onCultureChangedDelegateHandle = FInternationalization::Get().OnCultureChanged().AddUObject(this, &ALGUIManagerActor::OnCultureChanged);
}
void ALGUIManagerActor::BeginDestroy()
{
	if (WorldToInstanceMap.Num() > 0 && existInInstanceMap)
	{
		bool removed = false;
		if (auto world = this->GetWorld())
		{
			WorldToInstanceMap.Remove(world);
			removed = true;
		}
		else
		{
			world = nullptr;
			for (auto& keyValue : WorldToInstanceMap)
			{
				if (keyValue.Value == this)
				{
					world = keyValue.Key;
				}
			}
			if (world != nullptr)
			{
				WorldToInstanceMap.Remove(world);
				removed = true;
			}
		}
		if (removed)
		{
			existInInstanceMap = false;
		}
		else
		{
			UE_LOG(LGUI, Warning, TEXT("[ALGUIManagerActor::BeginDestroy]Cannot remove instance!"));
		}
	}
	if (WorldToInstanceMap.Num() <= 0)
	{
		UE_LOG(LGUI, Log, TEXT("[ALGUIManagerActor::BeginDestroy]All instance removed."));
	}
	if (ScreenSpaceOverlayViewExtension.IsValid())
	{
		ScreenSpaceOverlayViewExtension.Reset();
	}
	Super::BeginDestroy();
#if WITH_EDITORONLY_DATA
	IsPlaying = false;
#endif
	if (onCultureChangedDelegateHandle.IsValid())
	{
		FInternationalization::Get().OnCultureChanged().Remove(onCultureChangedDelegateHandle);
	}
}

void ALGUIManagerActor::OnCultureChanged()
{
	bShouldUpdateOnCultureChanged = true;
}

ALGUIManagerActor* ALGUIManagerActor::GetInstance(UWorld* InWorld, bool CreateIfNotValid)
{
	if (IsValid(InWorld))
	{
		if (auto instance = WorldToInstanceMap.Find(InWorld))
		{
			return *instance;
		}
		else
		{
			if (CreateIfNotValid)
			{
#if WITH_EDITOR
				if (!InWorld->IsGameWorld())
				{
					auto msg = LOCTEXT("TryToCreateLGUIManagerActorInEditMode", "[ALGUIManagerActor::GetInstance]Trying to create a LGUIManagerActor in edit mode, this is not allowed!");
					UE_LOG(LGUI, Error, TEXT("%s"), *msg.ToString());
					LGUIUtils::EditorNotification(msg);
					return nullptr;
				}
#endif
				FActorSpawnParameters param = FActorSpawnParameters();
				param.ObjectFlags = RF_Transient;
				auto newInstance = InWorld->SpawnActor<ALGUIManagerActor>(param);
				WorldToInstanceMap.Add(InWorld, newInstance);
				UE_LOG(LGUI, Log, TEXT("[ALGUIManagerActor::GetInstance]No Instance for LGUIManagerActor, create!"));
				newInstance->existInInstanceMap = true;
				return newInstance;
			}
			else
			{
				return nullptr;
			}
		}
	}
	else
	{
		return nullptr;
	}
}

DECLARE_CYCLE_STAT(TEXT("LGUILifeCycleBehaviour Update"), STAT_LGUILifeCycleBehaviourUpdate, STATGROUP_LGUI);
DECLARE_CYCLE_STAT(TEXT("LGUILifeCycleBehaviour Start"), STAT_LGUILifeCycleBehaviourStart, STATGROUP_LGUI);
DECLARE_CYCLE_STAT(TEXT("UIItem UpdateLayout"), STAT_UIItemUpdateLayout, STATGROUP_LGUI);
DECLARE_CYCLE_STAT(TEXT("Canvas UpdateGeometryAndDrawcall"), STAT_UpdateGeometryAndDrawcall, STATGROUP_LGUI);
void ALGUIManagerActor::Tick(float DeltaTime)
{
	//editor draw helper frame
#if WITH_EDITOR
	if (this->GetWorld())
	{
		if (this->GetWorld()->WorldType == EWorldType::Game
			|| this->GetWorld()->WorldType == EWorldType::PIE
			)
		{
			for (auto& item : AllUIItemArray)
			{
				if (!item.IsValid())continue;
				if (!IsValid(item->GetWorld()))continue;

				ULGUIEditorManagerObject::DrawFrameOnUIItem(item.Get());
			}
		}
	}
#endif

	//Update culture
	{
		if (bShouldUpdateOnCultureChanged)
		{
			bShouldUpdateOnCultureChanged = false;
			for (auto& item : AllCultureChangedArray)
			{
				ILGUICultureChangedInterface::Execute_OnCultureChanged(item.GetObject());
			}
		}
	}

	//LGUILifeCycleBehaviour start
	{
		if (LGUILifeCycleBehavioursForStart.Num() > 0)
		{
			bIsExecutingStart = true;
			SCOPE_CYCLE_COUNTER(STAT_LGUILifeCycleBehaviourStart);
			for (int i = 0; i < LGUILifeCycleBehavioursForStart.Num(); i++)
			{
				auto item = LGUILifeCycleBehavioursForStart[i];
				if (item.IsValid())
				{
					item->Start();
					if (item->bCanExecuteUpdate && !item->bIsAddedToUpdate)
					{
						item->bIsAddedToUpdate = true;
						LGUILifeCycleBehavioursForUpdate.Add(item);
					}
				}
			}
			LGUILifeCycleBehavioursForStart.Reset();
			bIsExecutingStart = false;
		}
	}

	//LGUILifeCycleBehaviour update
	{
		bIsExecutingUpdate = true;
		SCOPE_CYCLE_COUNTER(STAT_LGUILifeCycleBehaviourUpdate);
		for (int i = 0; i < LGUILifeCycleBehavioursForUpdate.Num(); i++)
		{
			CurrentExecutingUpdateIndex = i;
			auto item = LGUILifeCycleBehavioursForUpdate[i];
			if (item.IsValid())
			{
				item->Update(DeltaTime);
			}
		}
		bIsExecutingUpdate = false;
		CurrentExecutingUpdateIndex = -1;
		//remove these padding things
		if (LGUILifeCycleBehavioursNeedToRemoveFromUpdate.Num() > 0)
		{
			for (auto& item : LGUILifeCycleBehavioursNeedToRemoveFromUpdate)
			{
				LGUILifeCycleBehavioursForUpdate.Remove(item);
			}
			LGUILifeCycleBehavioursNeedToRemoveFromUpdate.Reset();
		}
	}

#if WITH_EDITOR
	int ScreenSpaceOverlayCanvasCount = 0;
	for (auto& item : AllCanvasArray)
	{
		if (item.IsValid())
		{
			if (item->IsRootCanvas())
			{
				if (item->GetActualRenderMode() == ELGUIRenderMode::ScreenSpaceOverlay)
				{
					ScreenSpaceOverlayCanvasCount++;
				}
			}
		}
	}
	if (ScreenSpaceOverlayCanvasCount > 1)
	{
		if (PrevScreenSpaceOverlayCanvasCount != ScreenSpaceOverlayCanvasCount)//only show message when change
		{
			PrevScreenSpaceOverlayCanvasCount = ScreenSpaceOverlayCanvasCount;
			auto errMsg = LOCTEXT("MultipleLGUICanvasRenderScreenSpaceOverlay", "Detect multiply LGUICanvas renderred with ScreenSpaceOverlay mode, this is not allowed! There should be only one ScreenSpace UI in a world!");
			UE_LOG(LGUI, Error, TEXT("%s"), *errMsg.ToString());
			LGUIUtils::EditorNotification(errMsg, 10.0f);
		}
	}
	else
	{
		PrevScreenSpaceOverlayCanvasCount = 0;
	}
#endif

	UpdateLayout();

	//update drawcall
	{
		SCOPE_CYCLE_COUNTER(STAT_UpdateGeometryAndDrawcall);
		for (auto& item : AllCanvasArray)
		{
			if (item.IsValid())
			{
				item->UpdateCanvas();
			}
		}
	}

	//sort render order
	if (AllCanvasArray.Num() > 0)
	{
		if (bShouldSortLGUIRenderer || bShouldSortWorldSpaceCanvas || bShouldSortRenderTargetSpaceCanvas)
		{
			//@todo: no need to sort all canvas
			AllCanvasArray.Sort([](const TWeakObjectPtr<ULGUICanvas>& A, const TWeakObjectPtr<ULGUICanvas>& B)
				{
					return A->GetActualSortOrder() < B->GetActualSortOrder();
				});
		}
		if (bShouldSortLGUIRenderer)
		{
			SortDrawcallOnRenderMode(ELGUIRenderMode::ScreenSpaceOverlay);
			SortDrawcallOnRenderMode(ELGUIRenderMode::WorldSpace_LGUI);
			if (ScreenSpaceOverlayViewExtension.IsValid())
			{
				ScreenSpaceOverlayViewExtension->SortPrimitiveRenderPriority();
			}
			bShouldSortLGUIRenderer = false;
		}
		if (bShouldSortWorldSpaceCanvas)
		{
			SortDrawcallOnRenderMode(ELGUIRenderMode::WorldSpace);
			bShouldSortWorldSpaceCanvas = false;
		}
		if (bShouldSortRenderTargetSpaceCanvas)
		{
			SortDrawcallOnRenderMode(ELGUIRenderMode::RenderTarget);
			bShouldSortRenderTargetSpaceCanvas = false;
		}
	}
}

void ALGUIManagerActor::SortDrawcallOnRenderMode(ELGUIRenderMode InRenderMode)//@todo: cleanup this function
{
	int32 RenderPriority = 0;
	int32 prevSortOrder = INT_MIN;
	int32 prevCanvasDrawcallCount = 0;//prev Canvas's drawcall count
	for (int i = 0; i < AllCanvasArray.Num(); i++)
	{
		auto canvasItem = this->AllCanvasArray[i];
		if (canvasItem.IsValid() && canvasItem->GetIsUIActive() && !canvasItem->IsRenderByOtherCanvas())
		{
			if (canvasItem->GetActualRenderMode() == InRenderMode)
			{
				canvasItem->SortDrawcall(RenderPriority);
			}
		}
	}
}

void ALGUIManagerActor::AddLGUILifeCycleBehaviourForLifecycleEvent(ULGUILifeCycleBehaviour* InComp)
{
	if (IsValid(InComp))
	{
		if (auto Instance = GetInstance(InComp->GetWorld(), true))
		{
			if (IsPrefabSystemProcessingActor(InComp->GetOwner()))
			{
				if (Instance->PrefabSystemProcessing_CurrentArrayIndex < 0 || Instance->PrefabSystemProcessing_CurrentArrayIndex >= Instance->LGUILifeCycleBehaviours_PrefabSystemProcessing.Num())
				{
					UE_LOG(LGUI, Warning, TEXT("[ALGUIManagerActor::AddLGUILifeCycleBehaviourForLifecycleEvent]array out of range, index:%d, arrayCount:%d"), Instance->PrefabSystemProcessing_CurrentArrayIndex, Instance->LGUILifeCycleBehaviours_PrefabSystemProcessing.Num());
					return;
				}
				auto& compArray = Instance->LGUILifeCycleBehaviours_PrefabSystemProcessing[Instance->PrefabSystemProcessing_CurrentArrayIndex].LGUILifeCycleBehaviourArray;
				if (compArray.Contains(InComp))
				{
					UE_LOG(LGUI, Warning, TEXT("[ALGUIManagerActor::AddLGUILifeCycleBehaviourForLifecycleEvent]already contains, comp:%s"), *(InComp->GetPathName()));
					return;
				}
				compArray.Add(InComp);
			}
			else
			{
				ProcessLGUILifecycleEvent(InComp);
			}
		}
	}
}

void ALGUIManagerActor::AddLGUILifeCycleBehavioursForUpdate(ULGUILifeCycleBehaviour* InComp)
{
	if (IsValid(InComp))
	{
		if (auto Instance = GetInstance(InComp->GetWorld(), true))
		{
			int32 index = INDEX_NONE;
			if (!Instance->LGUILifeCycleBehavioursForUpdate.Find(InComp, index))
			{
				Instance->LGUILifeCycleBehavioursForUpdate.Add(InComp);
				return;
			}
			UE_LOG(LGUI, Warning, TEXT("[ALGUIManagerActor::AddLGUILifeCycleBehavioursForUpdate]Already exist, comp:%s"), *(InComp->GetPathName()));
		}
	}
}
void ALGUIManagerActor::RemoveLGUILifeCycleBehavioursFromUpdate(ULGUILifeCycleBehaviour* InComp)
{
	if (IsValid(InComp))
	{
		if (auto Instance = GetInstance(InComp->GetWorld(), false))
		{
			auto& updateArray = Instance->LGUILifeCycleBehavioursForUpdate;
			int32 index = INDEX_NONE;
			if (updateArray.Find(InComp, index))
			{
				if (Instance->bIsExecutingUpdate)
				{
					if (index > Instance->CurrentExecutingUpdateIndex)//not execute it yet, save to remove
					{
						updateArray.RemoveAt(index);
					}
					else//already execute or current execute it, not safe to remove. should remove it after execute process complete
					{
						Instance->LGUILifeCycleBehavioursNeedToRemoveFromUpdate.Add(InComp);
					}
				}
				else//not executing update, safe to remove
				{
					updateArray.RemoveAt(index);
				}
			}
			else
			{
				UE_LOG(LGUI, Warning, TEXT("[ALGUIManagerActor::RemoveLGUILifeCycleBehavioursFromUpdate]Not exist, comp:%s"), *(InComp->GetPathName()));
			}

			//cleanup array
			int inValidCount = 0;
			for (int i = updateArray.Num() - 1; i >= 0; i--)
			{
				if (!updateArray[i].IsValid())
				{
					updateArray.RemoveAt(i);
					inValidCount++;
				}
			}
			if (inValidCount > 0)
			{
				UE_LOG(LGUI, Warning, TEXT("[ALGUIManagerActor::RemoveLGUILifeCycleBehavioursFromUpdate]Cleanup %d invalid LGUILifeCycleBehaviour"), inValidCount);
			}
		}
	}
}

void ALGUIManagerActor::AddLGUILifeCycleBehavioursForStart(ULGUILifeCycleBehaviour* InComp)
{
	if (IsValid(InComp))
	{
		if (auto Instance = GetInstance(InComp->GetWorld(), true))
		{
			int32 index = INDEX_NONE;
			if (!Instance->LGUILifeCycleBehavioursForStart.Find(InComp, index))
			{
				Instance->LGUILifeCycleBehavioursForStart.Add(InComp);
				return;
			}
			UE_LOG(LGUI, Warning, TEXT("[ALGUIManagerActor::AddLGUILifeCycleBehavioursForStart]Already exist, comp:%s"), *(InComp->GetPathName()));
		}
	}
}
void ALGUIManagerActor::RemoveLGUILifeCycleBehavioursFromStart(ULGUILifeCycleBehaviour* InComp)
{
	if (IsValid(InComp))
	{
		if (auto Instance = GetInstance(InComp->GetWorld(), false))
		{
			auto& startArray = Instance->LGUILifeCycleBehavioursForStart;
			int32 index = INDEX_NONE;
			if (startArray.Find(InComp, index))
			{
				if (Instance->bIsExecutingStart)
				{
					if (!InComp->bIsStartCalled)//if already called start then nothing to do, because start array will be cleared after execute start
					{
						startArray.RemoveAt(index);//not execute start yet, safe to remove
					}
				}
				else
				{
					startArray.RemoveAt(index);//not executing start, safe to remove
				}
			}
			else
			{
				UE_LOG(LGUI, Warning, TEXT("[ALGUIManagerActor::RemoveLGUILifeCycleBehavioursFromStart]Not exist, comp:%s"), *(InComp->GetPathName()));
			}

			//cleanup array
			int inValidCount = 0;
			for (int i = startArray.Num() - 1; i >= 0; i--)
			{
				if (!startArray[i].IsValid())
				{
					startArray.RemoveAt(i);
					inValidCount++;
				}
			}
			if (inValidCount > 0)
			{
				UE_LOG(LGUI, Warning, TEXT("[ALGUIManagerActor::RemoveLGUILifeCycleBehavioursFromStart]Cleanup %d invalid LGUILifeCycleBehaviour"), inValidCount);
			}
		}
	}
}



void ALGUIManagerActor::AddUIItem(UUIItem* InItem)
{
	if (auto Instance = GetInstance(InItem->GetWorld(), true))
	{
#if !UE_BUILD_SHIPPING
		check(!Instance->AllUIItemArray.Contains(InItem));
#endif
		Instance->AllUIItemArray.AddUnique(InItem);
	}
}
void ALGUIManagerActor::RemoveUIItem(UUIItem* InItem)
{
	if (auto Instance = GetInstance(InItem->GetWorld()))
	{
#if !UE_BUILD_SHIPPING
		check(Instance->AllUIItemArray.Contains(InItem));
#endif
		Instance->AllUIItemArray.RemoveSingle(InItem);
	}
}

void ALGUIManagerActor::AddRootUIItem(UUIItem* InItem)
{
	if (auto Instance = GetInstance(InItem->GetWorld(), true))
	{
#if !UE_BUILD_SHIPPING
		check(!Instance->AllRootUIItemArray.Contains(InItem));
#endif
		Instance->AllRootUIItemArray.AddUnique(InItem);
	}
}
void ALGUIManagerActor::RemoveRootUIItem(UUIItem* InItem)
{
	if (auto Instance = GetInstance(InItem->GetWorld()))
	{
#if !UE_BUILD_SHIPPING
		check(Instance->AllRootUIItemArray.Contains(InItem));
#endif
		Instance->AllRootUIItemArray.RemoveSingle(InItem);
	}
}

void ALGUIManagerActor::RegisterLGUICultureChangedEvent(TScriptInterface<ILGUICultureChangedInterface> InItem)
{
	if (auto Instance = GetInstance(InItem.GetObject()->GetWorld(), true))
	{
		Instance->AllCultureChangedArray.AddUnique(InItem);
	}
}
void ALGUIManagerActor::UnregisterLGUICultureChangedEvent(TScriptInterface<ILGUICultureChangedInterface> InItem)
{
	if (auto Instance = GetInstance(InItem.GetObject()->GetWorld()))
	{
		Instance->AllCultureChangedArray.RemoveSingle(InItem);
	}
}

void ALGUIManagerActor::UpdateLayout()
{
	SCOPE_CYCLE_COUNTER(STAT_UIItemUpdateLayout);

	//update Layout
	for (auto& item : AllLayoutArray)
	{
		ILGUILayoutInterface::Execute_OnUpdateLayout(item.GetObject());
	}
}
void ALGUIManagerActor::ForceUpdateLayout(UObject* WorldContextObject)
{
	if (auto world = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull))
	{
		if (auto Instance = GetInstance(world, false))
		{
			Instance->UpdateLayout();
		}
	}
}

void ALGUIManagerActor::RemoveCanvas(ULGUICanvas* InCanvas)
{
	if (auto Instance = GetInstance(InCanvas->GetWorld(), false))
	{
#if !UE_BUILD_SHIPPING
		check(Instance->AllCanvasArray.Contains(InCanvas));
#endif
		Instance->AllCanvasArray.RemoveSingle(InCanvas);
	}
}
void ALGUIManagerActor::AddCanvas(ULGUICanvas* InCanvas)
{
	if (auto Instance = GetInstance(InCanvas->GetWorld(), true))
	{
#if !UE_BUILD_SHIPPING
		check(!Instance->AllCanvasArray.Contains(InCanvas));
#endif
		Instance->AllCanvasArray.AddUnique(InCanvas);
	}
}
void ALGUIManagerActor::MarkSortLGUIRenderer()
{
	bShouldSortLGUIRenderer = true;
}
void ALGUIManagerActor::MarkSortWorldSpaceCanvas()
{
	bShouldSortWorldSpaceCanvas = true;
}
void ALGUIManagerActor::MarkSortRenderTargetSpaceCanvas()
{
	bShouldSortRenderTargetSpaceCanvas = true;
}

TSharedPtr<class FLGUIHudRenderer, ESPMode::ThreadSafe> ALGUIManagerActor::GetViewExtension(UWorld* InWorld, bool InCreateIfNotExist)
{
	if (auto Instance = GetInstance(InWorld, true))
	{
		if (!Instance->ScreenSpaceOverlayViewExtension.IsValid())
		{
			if (InCreateIfNotExist)
			{
				Instance->ScreenSpaceOverlayViewExtension = FSceneViewExtensions::NewExtension<FLGUIHudRenderer>(InWorld, nullptr);
			}
		}
		return Instance->ScreenSpaceOverlayViewExtension;
	}
	return nullptr;
}

void ALGUIManagerActor::AddRaycaster(ULGUIBaseInteractionComponent* InRaycaster)
{
	if (auto Instance = GetInstance(InRaycaster->GetWorld(), true))
	{
		auto& AllRaycasterArray = Instance->AllRaycasterArray;
		if (AllRaycasterArray.Contains(InRaycaster))return;
		//check multiple racaster
		for (auto& item : AllRaycasterArray)
		{
			if (InRaycaster->GetDepth() == item->GetDepth() && InRaycaster->GetTraceChannel() == item->GetTraceChannel())
			{
				auto msg = FString(TEXT("\
\nDetect multiple LGUIBaseInteractionComponent components with same depth and traceChannel, this may cause wrong interaction results!\
\neg: Want use mouse to click object A but get object B.\
\nPlease note:\
\n	For LGUIBaseInteractionComponents with same depth, LGUI will line trace them all and sort result on hit distance.\
\n	For LGUIBaseInteractionComponents with different depth, LGUI will sort raycasters on depth, and line trace from highest depth to lowest, if hit anything then stop line trace.\
\nLGUIXXXSpaceInteractionXXX is also a ULGUIBaseInteractionComponent component."));
				UE_LOG(LGUI, Warning, TEXT("\n%s"), *msg);
				break;
			}
		}

		AllRaycasterArray.Add(InRaycaster);
		//sort depth
		AllRaycasterArray.Sort([](const TWeakObjectPtr<ULGUIBaseInteractionComponent>& A, const TWeakObjectPtr<ULGUIBaseInteractionComponent>& B)
		{
			return A->GetDepth() > B->GetDepth();
		});
	}
}
void ALGUIManagerActor::RemoveRaycaster(ULGUIBaseInteractionComponent* InRaycaster)
{
	if (auto Instance = GetInstance(InRaycaster->GetWorld()))
	{
		int32 index;
		if (Instance->AllRaycasterArray.Find(InRaycaster, index))
		{
			Instance->AllRaycasterArray.RemoveAt(index);
		}
	}
}

void ALGUIManagerActor::SetCurrentInputModule(ULGUIBaseInputModule* InInputModule)
{
	if (auto Instance = GetInstance(InInputModule->GetWorld(), true))
	{
		Instance->CurrentInputModule = InInputModule;
	}
}
void ALGUIManagerActor::ClearCurrentInputModule(ULGUIBaseInputModule* InInputModule)
{
	if (auto Instance = GetInstance(InInputModule->GetWorld()))
	{
		Instance->CurrentInputModule = nullptr;
	}
}

void ALGUIManagerActor::AddSelectable(UUISelectableComponent* InSelectable)
{
	if (auto Instance = GetInstance(InSelectable->GetWorld(), true))
	{
		auto& AllSelectableArray = Instance->AllSelectableArray;
		if (AllSelectableArray.Contains(InSelectable))return;
		AllSelectableArray.Add(InSelectable);
	}
}
void ALGUIManagerActor::RemoveSelectable(UUISelectableComponent* InSelectable)
{
	if (auto Instance = GetInstance(InSelectable->GetWorld()))
	{
		int32 index;
		if (Instance->AllSelectableArray.Find(InSelectable, index))
		{
			Instance->AllSelectableArray.RemoveAt(index);
		}
	}
}

void ALGUIManagerActor::RegisterLGUILayout(TScriptInterface<ILGUILayoutInterface> InItem)
{
	if (auto Instance = GetInstance(InItem.GetObject()->GetWorld(), true))
	{
#if !UE_BUILD_SHIPPING
		check(!Instance->AllLayoutArray.Contains(InItem));
#endif
		Instance->AllLayoutArray.AddUnique(InItem);
	}
}
void ALGUIManagerActor::UnregisterLGUILayout(TScriptInterface<ILGUILayoutInterface> InItem)
{
	if (auto Instance = GetInstance(InItem.GetObject()->GetWorld()))
	{
#if !UE_BUILD_SHIPPING
		check(Instance->AllLayoutArray.Contains(InItem));
#endif
		Instance->AllLayoutArray.RemoveSingle(InItem);
	}
}


void ALGUIManagerActor::EndPrefabSystemProcessingActor_Implement()
{
	PrefabSystemProcessing_CurrentArrayIndex--;
	check(PrefabSystemProcessing_CurrentArrayIndex >= PrefabSystemProcessing_MinArrayIndex);
	if (PrefabSystemProcessing_CurrentArrayIndex == PrefabSystemProcessing_MinArrayIndex)//wait all prefab serialization ready then do Awake
	{
		for (int j = LGUILifeCycleBehaviours_PrefabSystemProcessing.Num() - 1; j >= 0; j--)
		{
			auto& LGUILifeCycleBehaviourArray = LGUILifeCycleBehaviours_PrefabSystemProcessing[j].LGUILifeCycleBehaviourArray;

			for (int i = LGUILifeCycleBehaviourArray.Num() - 1; i >= 0; i--)//execute from tail to head, when in prefab the deeper in hierarchy will execute earlier
			{
				auto item = LGUILifeCycleBehaviourArray[i];
				if (item.IsValid())
				{
					ProcessLGUILifecycleEvent(item.Get());
				}
			}
		}
		LGUILifeCycleBehaviours_PrefabSystemProcessing.Reset();
	}
}
void ALGUIManagerActor::ProcessLGUILifecycleEvent(ULGUILifeCycleBehaviour* InComp)
{
	if (InComp)
	{
		if (InComp->IsAllowedToCallAwake())
		{
			if (!InComp->bIsAwakeCalled)
			{
				InComp->Awake();
			}
			if (InComp->IsAllowedToCallOnEnable())
			{
				if (!InComp->bIsEnableCalled)
				{
					InComp->OnEnable();
				}
			}
		}
	}
}
void ALGUIManagerActor::BeginPrefabSystemProcessingActor(UWorld* InWorld)
{
	if (auto Instance = GetInstance(InWorld, true))
	{
		Instance->LGUILifeCycleBehaviours_PrefabSystemProcessing.Add({});
		Instance->PrefabSystemProcessing_CurrentArrayIndex++;
	}
}
void ALGUIManagerActor::EndPrefabSystemProcessingActor(UWorld* InWorld)
{
	if (auto Instance = GetInstance(InWorld, false))
	{
		Instance->EndPrefabSystemProcessingActor_Implement();
	}
}
void ALGUIManagerActor::AddActorForPrefabSystem(AActor* InActor)
{
	if (auto Instance = GetInstance(InActor->GetWorld(), true))
	{
		Instance->AllActors_PrefabSystemProcessing.AddUnique(InActor);
	}
}
void ALGUIManagerActor::RemoveActorForPrefabSystem(AActor* InActor)
{
	if (auto Instance = GetInstance(InActor->GetWorld()))
	{
		Instance->AllActors_PrefabSystemProcessing.RemoveSingle(InActor);
	}
}
bool ALGUIManagerActor::IsPrefabSystemProcessingActor(AActor* InActor)
{
	if (auto Instance = GetInstance(InActor->GetWorld()))
	{
		return Instance->AllActors_PrefabSystemProcessing.Contains(InActor);
	}
	return false;
}
PRAGMA_ENABLE_OPTIMIZATION
#undef LOCTEXT_NAMESPACE