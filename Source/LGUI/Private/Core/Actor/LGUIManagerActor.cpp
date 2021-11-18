// Copyright 2019-2021 LexLiu. All Rights Reserved.

#include "Core/Actor/LGUIManagerActor.h"
#include "LGUI.h"
#include "Utils/LGUIUtils.h"
#include "Core/ActorComponent/UIItem.h"
#include "Core/ActorComponent/UIText.h"
#include "Core/ActorComponent/LGUICanvas.h"
#include "Event/Raycaster/LGUIBaseRaycaster.h"
#include "Engine/World.h"
#include "Interaction/UISelectableComponent.h"
#include "Core/LGUISettings.h"
#include "Event/InputModule/LGUIBaseInputModule.h"
#include "PrefabSystem/ActorSerializer.h"
#include "Core/Actor/UIBaseActor.h"
#include "Core/ActorComponent/UIBatchGeometryRenderable.h"
#include "Core/ActorComponent/UIBaseRenderable.h"
#include "Core/ActorComponent/UIPostProcessRenderable.h"
#include "Engine/Engine.h"
#include "Layout/UILayoutBase.h"
#include "Core/HudRender/LGUIRenderer.h"
#include "Core/ILGUICultureChangedInterface.h"
#if WITH_EDITOR
#include "Editor.h"
#include "DrawDebugHelpers.h"
#include "Engine/Selection.h"
#include "EditorViewportClient.h"
#include "PrefabSystem/LGUIPrefabHelperActor.h"
#include "PrefabSystem/LGUIPrefabHelperComponent.h"
#include "EngineUtils.h"
#include "Layout/LGUICanvasScaler.h"
#endif


ULGUIEditorManagerObject* ULGUIEditorManagerObject::Instance = nullptr;
ULGUIEditorManagerObject::ULGUIEditorManagerObject()
{

}
void ULGUIEditorManagerObject::BeginDestroy()
{
#if WITH_EDITORONLY_DATA
	if (OnSelectionChangedDelegateHandle.IsValid())
	{
		USelection::SelectObjectEvent.Remove(OnSelectionChangedDelegateHandle);
	}
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
#endif
	Instance = nullptr;
	Super::BeginDestroy();
}

void ULGUIEditorManagerObject::Tick(float DeltaTime)
{
#if WITH_EDITORONLY_DATA
	//draw frame
	for (auto item : allUIItem)
	{
		if (!item.IsValid())continue;
		if (!IsValid(item->GetWorld()))continue;
		if (
			item->GetWorld()->WorldType != EWorldType::Editor//actually, ULGUIEditorManagerObject only collect editor mode UIItem, so only this Editor condition will trigger.
															//so only Editor mode will draw frame. the two modes below will not work, just leave it as a reference.
			&& item->GetWorld()->WorldType != EWorldType::Game
			&& item->GetWorld()->WorldType != EWorldType::PIE
			&& item->GetWorld()->WorldType != EWorldType::EditorPreview
			)continue;

		ULGUIEditorManagerObject::DrawFrameOnUIItem(item.Get());
	}

	for (auto item : allLayoutArray)
	{
		if (item.IsValid())
		{
			if (item->GetIsActiveAndEnable())
			{
				item->ConditionalRebuildLayout();
			}
		}
	}

	for (auto item : rootUIItems)
	{
		item->UpdateRootUIItem();
	}
	
	int ScreenSpaceOverlayCanvasCount = 0;
	for (auto item : allCanvas)
	{
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
	for (auto item : allCanvas)
	{
		if (item.IsValid())
		{
			item->UpdateRootCanvasDrawcall();
		}
	}

	if (ScreenSpaceOverlayCanvasCount > 1)
	{
		if (PrevScreenSpaceOverlayCanvasCount != ScreenSpaceOverlayCanvasCount)//only show message when change
		{
			PrevScreenSpaceOverlayCanvasCount = ScreenSpaceOverlayCanvasCount;
			auto errMsg = FString::Printf(TEXT("Detect multiply LGUICanvas renderred with ScreenSpaceOverlay mode, this is not allowed! There should be only one ScreenSpace UI in a world!"));
			UE_LOG(LGUI, Error, TEXT("%s"), *errMsg);
			LGUIUtils::EditorNotification(FText::FromString(errMsg), 10.0f);
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

	if (allCanvas.Num() > 0)
	{
		if (bShouldSortLGUIRenderer || bShouldSortWorldSpaceCanvas || bShouldSortRenderTargetSpaceCanvas)
		{
			allCanvas.Sort([](const TWeakObjectPtr<ULGUICanvas>& A, const TWeakObjectPtr<ULGUICanvas>& B)
				{
					return A->GetSortOrder() < B->GetSortOrder();
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
#if WITH_EDITORONLY_DATA
bool ULGUIEditorManagerObject::CanExecuteSelectionConvert = true;
#endif
#if WITH_EDITOR
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
				auto msg = FString(TEXT("[ULGUIEditorManagerObject::InitCheck]Trying to create a LGUIEditorManagerObject in game mode, this is not allowed!"));
				UE_LOG(LGUI, Error, TEXT("%s"), *msg);
				LGUIUtils::EditorNotification(FText::FromString(msg));
				return nullptr;
			}
			Instance = NewObject<ULGUIEditorManagerObject>();
			Instance->AddToRoot();
			UE_LOG(LGUI, Log, TEXT("[ULGUIManagerObject::InitCheck]No Instance for LGUIManagerObject, create!"));
			//selection
			Instance->OnSelectionChangedDelegateHandle = USelection::SelectionChangedEvent.AddUObject(Instance, &ULGUIEditorManagerObject::OnSelectionChanged);
			//actor label
			Instance->OnActorLabelChangedDelegateHandle = FCoreDelegates::OnActorLabelChanged.AddUObject(Instance, &ULGUIEditorManagerObject::OnActorLabelChanged);
			//reimport asset
			Instance->OnAssetReimportDelegateHandle = GEditor->GetEditorSubsystem<UImportSubsystem>()->OnAssetReimport.AddUObject(Instance, &ULGUIEditorManagerObject::OnAssetReimport);
			//delete actor
			Instance->OnActorDeletedDelegateHandle = FEditorDelegates::OnDeleteActorsEnd.AddUObject(Instance, &ULGUIEditorManagerObject::OnActorDeleted);
			//open map
			Instance->OnMapOpenedDelegateHandle = FEditorDelegates::OnMapOpened.AddUObject(Instance, &ULGUIEditorManagerObject::OnMapOpened);
		}
		else
		{
			return false;
		}
	}
	return true;
}

void ULGUIEditorManagerObject::SortDrawcallOnRenderMode(ELGUIRenderMode InRenderMode)
{
	int32 startRenderPriority = 0;
	int32 prevSortOrder = INT_MIN;
	int32 prevCanvasDrawcallCount = 0;//prev Canvas's drawcall count
	for (int i = 0; i < allCanvas.Num(); i++)
	{
		auto canvasItem = this->allCanvas[i];
		if (canvasItem.IsValid())
		{
			if (canvasItem->GetActualRenderMode() == InRenderMode)
			{
				auto canvasItemSortOrder = canvasItem->GetSortOrder();
				if (canvasItemSortOrder != prevSortOrder)
				{
					prevSortOrder = canvasItemSortOrder;
					startRenderPriority += prevCanvasDrawcallCount;
				}
				int32 canvasItemDrawcallCount = canvasItem->SortDrawcall(startRenderPriority);

				if (canvasItemSortOrder == prevSortOrder)//if Canvas's depth is equal, then take the max drawcall count
				{
					if (prevCanvasDrawcallCount < canvasItemDrawcallCount)
					{
						prevCanvasDrawcallCount = canvasItemDrawcallCount;
					}
				}
				else
				{
					prevCanvasDrawcallCount = canvasItemDrawcallCount;
				}
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
			if (!IsValid(prefabActor->GetPrefabComponent()->GetLoadedRootActor()))
			{
				LGUIUtils::DestroyActorWithHierarchy(prefabActor, false);
			}
		}
	}
}
void ULGUIEditorManagerObject::OnMapOpened(const FString& FileName, bool AsTemplate)
{
	//Restore prefabs
	if (GWorld == nullptr)return;
	for (TActorIterator<ALGUIPrefabHelperActor> ActorItr(GWorld); ActorItr; ++ActorItr)
	{
		auto prefabActor = *ActorItr;
		if (IsValid(prefabActor))
		{
			if (IsValid(prefabActor->GetPrefabComponent()->PrefabAsset))
			{
				if (IsValid(prefabActor->GetPrefabComponent()->GetLoadedRootActor()))
				{
					
				}
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
		}
	}
}

void ULGUIEditorManagerObject::RefreshAllUI()
{
	if (Instance != nullptr)
	{
		for (auto itemCanvas : Instance->allCanvas)
		{
			if (itemCanvas.IsValid())
			{
				auto uiItem = itemCanvas->GetUIItem();
				if (uiItem != nullptr)
				{
					uiItem->MarkAllDirtyRecursive();
					uiItem->EditorForceUpdateImmediately();
				}
			}
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
		Instance->allUIItem.Add(InItem);
	}
}
void ULGUIEditorManagerObject::RemoveUIItem(UUIItem* InItem)
{
	if (Instance != nullptr)
	{
		Instance->allUIItem.RemoveSingle(InItem);
	}
}

void ULGUIEditorManagerObject::RemoveCanvas(ULGUICanvas* InCanvas)
{
	if (Instance != nullptr)
	{
		int32 foundIndex = Instance->allCanvas.IndexOfByKey(InCanvas);
		if (foundIndex != INDEX_NONE)
		{
			Instance->allCanvas.RemoveAt(foundIndex);
		}
	}
}
void ULGUIEditorManagerObject::AddCanvas(ULGUICanvas* InCanvas)
{
	if (GetInstance(InCanvas->GetWorld(), true))
	{
		auto& canvasArray = Instance->allCanvas;
		canvasArray.AddUnique(InCanvas);
	}
}

const TArray<UUIItem*>& ULGUIEditorManagerObject::GetAllUIItem()
{
	tempUIItemArray.Reset();
	for (auto item : allUIItem)
	{
		if (item.IsValid())
		{
			tempUIItemArray.Add(item.Get());
		}
	}
	return tempUIItemArray;
}

void ULGUIEditorManagerObject::AddRootUIItem(UUIItem* InItem)
{
	if (GetInstance(InItem->GetWorld(), true))
	{
		Instance->rootUIItems.AddUnique(InItem);
	}
}
void ULGUIEditorManagerObject::RemoveRootUIItem(UUIItem* InItem)
{
	if (Instance != nullptr)
	{
		Instance->rootUIItems.RemoveSingle(InItem);
	}
}

void ULGUIEditorManagerObject::AddLayout(UUILayoutBase* InLayout)
{
	if (InitCheck(InLayout->GetWorld()))
	{
		Instance->allLayoutArray.Add(InLayout);
	}
}
void ULGUIEditorManagerObject::RemoveLayout(UUILayoutBase* InLayout)
{
	if (Instance != nullptr)
	{
		Instance->allLayoutArray.RemoveSingle(InLayout);
	}
}
void ULGUIEditorManagerObject::DrawFrameOnUIItem(UUIItem* item)
{
	const auto& widget = item->GetWidget();
	auto extends = FVector(widget.width, widget.height, 0.1) * 0.5f;
	bool canDraw = false;
	auto DrawColor = FColor(128, 128, 128, 128);//gray means normal object
	if (ULGUIEditorManagerObject::IsSelected(item->GetOwner()))//select self
	{
		DrawColor = FColor(0, 255, 0, 255);//green means selected object
		extends += FVector(0, 0, 1);
		canDraw = true;
	}
	else
	{
		//parent selected
		if (IsValid(item->GetParentAsUIItem()))
		{
			if (ULGUIEditorManagerObject::IsSelected(item->GetParentAsUIItem()->GetOwner()))
			{
				canDraw = true;
			}
		}
		//child selected
		const auto childrenCompArray = item->GetAttachUIChildren();
		for (auto uiComp : childrenCompArray)
		{
			if (IsValid(uiComp) && IsValid(uiComp->GetOwner()) && ULGUIEditorManagerObject::IsSelected(uiComp->GetOwner()))
			{
				canDraw = true;
				break;
			}
		}
		//other object of same hierarchy is selected
		if (IsValid(item->GetParentAsUIItem()))
		{
			const auto& sameLevelCompArray = item->GetParentAsUIItem()->GetAttachUIChildren();
			for (auto uiComp : sameLevelCompArray)
			{
				if (IsValid(uiComp) && IsValid(uiComp->GetOwner()) && ULGUIEditorManagerObject::IsSelected(uiComp->GetOwner()))
				{
					canDraw = true;
					break;
				}
			}
		}
	}
	//canvas scaler
	if (!canDraw)
	{
		if (item->IsCanvasUIItem())
		{
			if (auto canvasScaler = item->GetOwner()->FindComponentByClass<ULGUICanvasScaler>())
			{
				if (ULGUIEditorManagerObject::AnySelectedIsChildOf(item->GetOwner()))
				{
					canDraw = true;
					DrawColor = FColor(255, 227, 124);
				}
			}
		}
	}

	if (canDraw)
	{
		auto worldTransform = item->GetComponentTransform();
		FVector relativeOffset(0, 0, 0);
		relativeOffset.X = (0.5f - widget.pivot.X) * widget.width;
		relativeOffset.Y = (0.5f - widget.pivot.Y) * widget.height;
		auto worldLocation = worldTransform.TransformPosition(relativeOffset);
		//calculate world location
		if (item->GetParentAsUIItem() != nullptr)
		{
			FVector relativeLocation = item->GetRelativeLocation();
			const auto& parentWidget = item->GetParentAsUIItem()->GetWidget();
			switch (widget.anchorHAlign)
			{
			case UIAnchorHorizontalAlign::Left:
			{
				relativeLocation.X = parentWidget.width * (-parentWidget.pivot.X);
				relativeLocation.X += widget.anchorOffsetX;
			}
			break;
			case UIAnchorHorizontalAlign::Center:
			{
				relativeLocation.X = parentWidget.width * (0.5f - parentWidget.pivot.X);
				relativeLocation.X += widget.anchorOffsetX;
			}
			break;
			case UIAnchorHorizontalAlign::Right:
			{
				relativeLocation.X = parentWidget.width * (1 - parentWidget.pivot.X);
				relativeLocation.X += widget.anchorOffsetX;
			}
			break;
			case UIAnchorHorizontalAlign::Stretch:
			{
				relativeLocation.X = -parentWidget.pivot.X * parentWidget.width;
				relativeLocation.X += widget.stretchLeft;
				relativeLocation.X += widget.pivot.X * widget.width;
			}
			break;
			}
			switch (widget.anchorVAlign)
			{
			case UIAnchorVerticalAlign::Top:
			{
				relativeLocation.Y = parentWidget.height * (1 - parentWidget.pivot.Y);
				relativeLocation.Y += widget.anchorOffsetY;
			}
			break;
			case UIAnchorVerticalAlign::Middle:
			{
				relativeLocation.Y = parentWidget.height * (0.5f - parentWidget.pivot.Y);
				relativeLocation.Y += widget.anchorOffsetY;
			}
			break;
			case UIAnchorVerticalAlign::Bottom:
			{
				relativeLocation.Y = parentWidget.height * (-parentWidget.pivot.Y);
				relativeLocation.Y += widget.anchorOffsetY;
			}
			break;
			case UIAnchorVerticalAlign::Stretch:
			{
				relativeLocation.Y = -parentWidget.pivot.Y * parentWidget.height;
				relativeLocation.Y += widget.stretchBottom;
				relativeLocation.Y += widget.pivot.Y * widget.height;
			}
			break;
			}
			auto relativeTf = item->GetRelativeTransform();
			relativeTf.SetLocation(relativeLocation);
			FTransform calculatedWorldTf;
			FTransform::Multiply(&calculatedWorldTf, &relativeTf, &(item->GetParentAsUIItem()->GetComponentTransform()));
			worldLocation = calculatedWorldTf.TransformPosition(relativeOffset);
		}
		DrawDebugBox(item->GetWorld(), worldLocation, extends * worldTransform.GetScale3D(), worldTransform.GetRotation(), DrawColor);//@todo: screen-space UI should draw on screen-space, but no clue to achieve that
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
		Instance->AllActors_PrefabSystemProcessing.Reset();
	}
}
void ULGUIEditorManagerObject::EndPrefabSystemProcessingActor()
{
	if (Instance != nullptr)
	{
		Instance->AllActors_PrefabSystemProcessing.Reset();
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

bool ULGUIEditorManagerObject::RaycastHitUI(UWorld* InWorld, const TArray<UUIItem*>& InUIItems, const FVector& LineStart, const FVector& LineEnd
	, TWeakObjectPtr<UUIBaseRenderable> PrevSelectTarget, TWeakObjectPtr<AActor> PrevSelectedActor
	, TWeakObjectPtr<UUIBaseRenderable>& ResultSelectTarget, TWeakObjectPtr<AActor>& ResultSelectedActor
)
{
	TArray<FHitResult> HitResultArray;
	for (auto uiItem : InUIItems)
	{
		if (uiItem->GetWorld() == InWorld)
		{
			if (auto uiRenderable = Cast<UUIBaseRenderable>(uiItem))
			{
				FHitResult hitInfo;
				auto originRaycastComplex = uiRenderable->GetRaycastComplex();
				auto originRaycastTarget = uiRenderable->IsRaycastTarget();
				uiRenderable->SetRaycastComplex(true);//in editor selection, make the ray hit actural triangle
				uiRenderable->SetRaycastTarget(true);
				if (uiRenderable->LineTraceUI(hitInfo, LineStart, LineEnd))
				{
					if (uiRenderable->GetRenderCanvas()->IsPointVisible(hitInfo.Location))
					{
						HitResultArray.Add(hitInfo);
					}
				}
				uiRenderable->SetRaycastComplex(originRaycastComplex);
				uiRenderable->SetRaycastTarget(originRaycastTarget);
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
					if (AUIRenderable->GetDepth() == BUIRenderable->GetDepth())//if item's depth is equal then sort on distance
					{
						return A.Distance < B.Distance;
					}
					else
						return AUIRenderable->GetDepth() > BUIRenderable->GetDepth();
				}
				else//if Canvas's depth not equal then sort on Canvas's SortOrder
				{
					return AUIRenderable->GetRenderCanvas()->GetSortOrder() > BUIRenderable->GetRenderCanvas()->GetSortOrder();
				}
			});
		if (auto uiRenderableComp = Cast<UUIBaseRenderable>(HitResultArray[0].Component.Get()))//target need to select
		{
			if (PrevSelectTarget.Get() == uiRenderableComp)//if selection not change, then select hierarchy up
			{
				if (auto parentActor = PrevSelectedActor->GetAttachParentActor())
				{
					ResultSelectedActor = parentActor;
				}
				else//not have parent, loop back to origin
				{
					ResultSelectedActor = uiRenderableComp->GetOwner();
				}
			}
			else
			{
				ResultSelectedActor = uiRenderableComp->GetOwner();
			}
			ResultSelectTarget = uiRenderableComp;
			return true;
		}
	}
	return false;
}
void ULGUIEditorManagerObject::OnSelectionChanged(UObject* newSelection)
{
	if (!IsValid(GEditor))return;
	if (!ULGUIEditorManagerObject::CanExecuteSelectionConvert)return;
	if (IsCalculatingSelection)return;//incase infinite recursive, because selection can change inside this function
	IsCalculatingSelection = true;

	AUIBaseActor* selectedActor = nullptr;
	ULGUICanvas* selectedCanvas = nullptr;
	if (USelection* selection = Cast<USelection>(newSelection))
	{
		for (int i = 0; i < selection->Num(); i++)
		{
			selectedActor = Cast<AUIBaseActor>(selection->GetSelectedObject(i));
			if (IsValid(selectedActor))
			{
				selectedCanvas = selectedActor->FindComponentByClass<ULGUICanvas>();
				if (IsValid(selectedCanvas))
					break;
			}
		}
	}
	if (IsValid(selectedCanvas))
	{
		auto world = selectedCanvas->GetWorld();
		if (!world->IsGameWorld())
		{
			if (ULGUIEditorManagerObject::Instance != nullptr)
			{
				auto& allUIItems = ULGUIEditorManagerObject::Instance->GetAllUIItem();
				if (GEditor)
				{
					if (auto viewport = GEditor->GetActiveViewport())
					{
						if (viewport->HasMouseCapture())
						{
							auto mouseX = viewport->GetMouseX();
							auto mouseY = viewport->GetMouseY();
							if (mouseX < UUIItemEditorHelperComp::viewRect.Size().X && mouseY < UUIItemEditorHelperComp::viewRect.Size().Y)
							{
								FVector rayOrigin, rayDirection;
								auto client = (FEditorViewportClient*)viewport->GetClient();
								FSceneView::DeprojectScreenToWorld(FVector2D(mouseX, mouseY), UUIItemEditorHelperComp::viewRect, UUIItemEditorHelperComp::viewMatrices.GetInvViewMatrix(), UUIItemEditorHelperComp::viewMatrices.GetInvProjectionMatrix(), rayOrigin, rayDirection);
								float lineTraceLength = 10000;
								//find hit UIBatchGeometryRenderable
								auto lineStart = rayOrigin;
								auto lineEnd = rayOrigin + rayDirection * lineTraceLength;
								auto prevSelectTarget = LastSelectTarget;
								auto prevSelectActor = LastSelectedActor;
								if (RaycastHitUI(world, allUIItems, lineStart, lineEnd, prevSelectTarget, prevSelectActor, LastSelectTarget, LastSelectedActor))
								{
									GEditor->SelectNone(true, true);
									GEditor->SelectActor(LastSelectedActor.Get(), true, true);
									goto END;
								}
							}
						}
					}
				}
			}
		}
	}

	LastSelectTarget.Reset();
	LastSelectedActor.Reset();

	END:
	IsCalculatingSelection = false;
}
#if 0
void ULGUIEditorManagerObject::LogObjectFlags(UObject* obj)
{
	EObjectFlags of = obj->GetFlags();
	UE_LOG(LGUI, Log, TEXT("obj:%s\
\n	flagValue:%d\
\n	RF_Public:%d\
\n	RF_Standalone:%d\
\n	RF_MarkAsNative:%d\
\n	RF_Transactional:%d\
\n	RF_ClassDefaultObject:%d\
\n	RF_ArchetypeObject:%d\
\n	RF_Transient:%d\
\n	RF_MarkAsRootSet:%d\
\n	RF_TagGarbageTemp:%d\
\n	RF_NeedInitialization:%d\
\n	RF_NeedLoad:%d\
\n	RF_KeepForCooker:%d\
\n	RF_NeedPostLoad:%d\
\n	RF_NeedPostLoadSubobjects:%d\
\n	RF_NewerVersionExists:%d\
\n	RF_BeginDestroyed:%d\
\n	RF_FinishDestroyed:%d\
\n	RF_BeingRegenerated:%d\
\n	RF_DefaultSubObject:%d\
\n	RF_WasLoaded:%d\
\n	RF_TextExportTransient:%d\
\n	RF_LoadCompleted:%d\
\n	RF_InheritableComponentTemplate:%d\
\n	RF_DuplicateTransient:%d\
\n	RF_StrongRefOnFrame:%d\
\n	RF_NonPIEDuplicateTransient:%d\
\n	RF_Dynamic:%d\
\n	RF_WillBeLoaded:%d\
")
, *obj->GetPathName()
, obj->GetFlags()
, obj->HasAnyFlags(EObjectFlags::RF_Public)
, obj->HasAnyFlags(EObjectFlags::RF_Standalone)
, obj->HasAnyFlags(EObjectFlags::RF_MarkAsNative)
, obj->HasAnyFlags(EObjectFlags::RF_Transactional)
, obj->HasAnyFlags(EObjectFlags::RF_ClassDefaultObject)
, obj->HasAnyFlags(EObjectFlags::RF_ArchetypeObject)
, obj->HasAnyFlags(EObjectFlags::RF_Transient)
, obj->HasAnyFlags(EObjectFlags::RF_MarkAsRootSet)
, obj->HasAnyFlags(EObjectFlags::RF_TagGarbageTemp)
, obj->HasAnyFlags(EObjectFlags::RF_NeedInitialization)
, obj->HasAnyFlags(EObjectFlags::RF_NeedLoad)
, obj->HasAnyFlags(EObjectFlags::RF_KeepForCooker)
, obj->HasAnyFlags(EObjectFlags::RF_NeedPostLoad)
, obj->HasAnyFlags(EObjectFlags::RF_NeedPostLoadSubobjects)
, obj->HasAnyFlags(EObjectFlags::RF_NewerVersionExists)
, obj->HasAnyFlags(EObjectFlags::RF_BeginDestroyed)
, obj->HasAnyFlags(EObjectFlags::RF_FinishDestroyed)
, obj->HasAnyFlags(EObjectFlags::RF_BeingRegenerated)
, obj->HasAnyFlags(EObjectFlags::RF_DefaultSubObject)
, obj->HasAnyFlags(EObjectFlags::RF_WasLoaded)
, obj->HasAnyFlags(EObjectFlags::RF_TextExportTransient)
, obj->HasAnyFlags(EObjectFlags::RF_LoadCompleted)
, obj->HasAnyFlags(EObjectFlags::RF_InheritableComponentTemplate)
, obj->HasAnyFlags(EObjectFlags::RF_DuplicateTransient)
, obj->HasAnyFlags(EObjectFlags::RF_StrongRefOnFrame)
, obj->HasAnyFlags(EObjectFlags::RF_NonPIEDuplicateTransient)
, obj->HasAnyFlags(EObjectFlags::RF_Dynamic)
, obj->HasAnyFlags(EObjectFlags::RF_WillBeLoaded)
);
}
#endif
#endif



TMap<UWorld*, ALGUIManagerActor*> ALGUIManagerActor::WorldToInstanceMap ;
ALGUIManagerActor::ALGUIManagerActor()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickGroup = ETickingGroup::TG_DuringPhysics;
	SpawnCollisionHandlingMethod = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
#if WITH_EDITOR
	FEditorDelegates::BeginPIE.AddLambda([=](const bool isSimulating) {
		for (auto keyValue : WorldToInstanceMap)
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
			for (auto keyValue : WorldToInstanceMap)
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
					auto msg = FString(TEXT("[ALGUIManagerActor::GetInstance]Trying to create a LGUIManagerActor in edit mode, this is not allowed!"));
					UE_LOG(LGUI, Error, TEXT("%s"), *msg);
					LGUIUtils::EditorNotification(FText::FromString(msg));
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

DECLARE_CYCLE_STAT(TEXT("LGUIBehaviour Update"), STAT_LGUIBehaviourUpdate, STATGROUP_LGUI);
DECLARE_CYCLE_STAT(TEXT("LGUIBehaviour Start"), STAT_LGUIBehaviourStart, STATGROUP_LGUI);
DECLARE_CYCLE_STAT(TEXT("UIItem UpdateLayoutAndGeometry"), STAT_UIItemUpdateLayoutAndGeometry, STATGROUP_LGUI);
DECLARE_CYCLE_STAT(TEXT("Canvas UpdateDrawcall"), STAT_UpdateDrawcall, STATGROUP_LGUI);
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
			for (auto item : allUIItem)
			{
				if (!IsValid(item))continue;
				if (!IsValid(item->GetWorld()))continue;

				ULGUIEditorManagerObject::DrawFrameOnUIItem(item);
			}
		}
	}
#endif

	//Update culture
	{
		if (bShouldUpdateOnCultureChanged)
		{
			bShouldUpdateOnCultureChanged = false;
			for (auto item : cultureChanged)
			{
				ILGUICultureChangedInterface::Execute_OnCultureChanged(item.GetObject());
			}
		}
	}

	//LGUIBehaviour start
	{
		if (LGUIBehavioursForStart.Num() > 0)
		{
			bIsExecutingStart = true;
			SCOPE_CYCLE_COUNTER(STAT_LGUIBehaviourStart);
			for (int i = 0; i < LGUIBehavioursForStart.Num(); i++)
			{
				auto item = LGUIBehavioursForStart[i];
				if (item.IsValid())
				{
					item->Start();
					LGUIBehavioursForUpdate.Add(item);
				}
			}
			LGUIBehavioursForStart.Reset();
			bIsExecutingStart = false;
		}
	}

	//LGUIBehaviour update
	{
		bIsExecutingUpdate = true;
		SCOPE_CYCLE_COUNTER(STAT_LGUIBehaviourUpdate);
		for (int i = 0; i < LGUIBehavioursForUpdate.Num(); i++)
		{
			CurrentExecutingUpdateIndex = i;
			auto item = LGUIBehavioursForUpdate[i];
			if (item.IsValid())
			{
				item->Update(DeltaTime);
			}
		}
		bIsExecutingUpdate = false;
		CurrentExecutingUpdateIndex = -1;
		//remove these padding things
		if (LGUIBehavioursNeedToRemoveFromUpdate.Num() > 0)
		{
			for (auto item : LGUIBehavioursNeedToRemoveFromUpdate)
			{
				LGUIBehavioursForUpdate.Remove(item);
			}
			LGUIBehavioursNeedToRemoveFromUpdate.Reset();
		}
	}

#if WITH_EDITOR
	int ScreenSpaceOverlayCanvasCount = 0;
	for (auto item : allCanvas)
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
			auto errMsg = FString::Printf(TEXT("Detect multiply LGUICanvas renderred with ScreenSpaceOverlay mode, this is not allowed! There should be only one ScreenSpace UI in a world!"));
			UE_LOG(LGUI, Error, TEXT("%s"), *errMsg);
			LGUIUtils::EditorNotification(FText::FromString(errMsg), 10.0f);
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
		SCOPE_CYCLE_COUNTER(STAT_UpdateDrawcall);
		for (auto item : allCanvas)
		{
			if (item.IsValid())
			{
				item->UpdateRootCanvasDrawcall();
			}
		}
	}

	//sort render order
	if (allCanvas.Num() > 0)
	{
		if (bShouldSortLGUIRenderer || bShouldSortWorldSpaceCanvas || bShouldSortRenderTargetSpaceCanvas)
		{
			//@todo: no need to sort all canvas
			allCanvas.Sort([](const TWeakObjectPtr<ULGUICanvas>& A, const TWeakObjectPtr<ULGUICanvas>& B)
				{
					return A->GetSortOrder() < B->GetSortOrder();
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

void ALGUIManagerActor::SortDrawcallOnRenderMode(ELGUIRenderMode InRenderMode)
{
	int32 startRenderPriority = 0;
	int32 prevSortOrder = INT_MIN;
	int32 prevCanvasDrawcallCount = 0;//prev Canvas's drawcall count
	for (int i = 0; i < allCanvas.Num(); i++)
	{
		auto canvasItem = this->allCanvas[i];
		if (canvasItem.IsValid())
		{
			if (canvasItem->GetActualRenderMode() == InRenderMode)
			{
				auto canvasItemSortOrder = canvasItem->GetSortOrder();
				bool sameSortOrder = canvasItemSortOrder == prevSortOrder;
				if (!sameSortOrder)
				{
					prevSortOrder = canvasItemSortOrder;
					startRenderPriority += prevCanvasDrawcallCount;
				}
				int32 canvasItemDrawcallCount = canvasItem->SortDrawcall(startRenderPriority);

				if (sameSortOrder)//if Canvas's sortOrder is equal, then take the max drawcall count
				{
					if (prevCanvasDrawcallCount < canvasItemDrawcallCount)
					{
						prevCanvasDrawcallCount = canvasItemDrawcallCount;
					}
				}
				else
				{
					prevCanvasDrawcallCount = canvasItemDrawcallCount;
				}
			}
		}
	}
}

void ALGUIManagerActor::AddLGUIComponentForLifecycleEvent(ULGUIBehaviour* InComp)
{
	if (IsValid(InComp))
	{
		if (auto Instance = GetInstance(InComp->GetWorld(), true))
		{
			if (IsPrefabSystemProcessingActor(InComp->GetOwner()))
			{
				if (Instance->PrefabSystemProcessing_CurrentArrayIndex < 0 || Instance->PrefabSystemProcessing_CurrentArrayIndex >= Instance->LGUIBehaviours_PrefabSystemProcessing.Num())
				{
					UE_LOG(LGUI, Warning, TEXT("[ALGUIManagerActor::AddLGUIComponent]array out of range, index:%d, arrayCount:%d"), Instance->PrefabSystemProcessing_CurrentArrayIndex, Instance->LGUIBehaviours_PrefabSystemProcessing.Num());
					return;
				}
				auto& compArray = Instance->LGUIBehaviours_PrefabSystemProcessing[Instance->PrefabSystemProcessing_CurrentArrayIndex].LGUIBehaviourArray;
				if (compArray.Contains(InComp))
				{
					UE_LOG(LGUI, Warning, TEXT("[ALGUIManagerActor::AddLGUIComponent]already contains, comp:%s"), *(InComp->GetPathName()));
					return;
				}
				compArray.Add(InComp);
			}
			else
			{
				ProcessLGUIComponentLifecycleEvent(InComp);
			}
		}
	}
}

void ALGUIManagerActor::AddLGUIBehavioursForUpdate(ULGUIBehaviour* InComp)
{
	if (IsValid(InComp))
	{
		if (auto Instance = GetInstance(InComp->GetWorld(), true))
		{
			int32 index = INDEX_NONE;
			if (!Instance->LGUIBehavioursForUpdate.Find(InComp, index))
			{
				Instance->LGUIBehavioursForUpdate.Add(InComp);
				return;
			}
			UE_LOG(LGUI, Warning, TEXT("[ALGUIManagerActor::AddLGUIBehavioursForUpdate]Already exist, comp:%s"), *(InComp->GetPathName()));
		}
	}
}
void ALGUIManagerActor::RemoveLGUIBehavioursFromUpdate(ULGUIBehaviour* InComp)
{
	if (IsValid(InComp))
	{
		if (auto Instance = GetInstance(InComp->GetWorld(), false))
		{
			auto& updateArray = Instance->LGUIBehavioursForUpdate;
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
						Instance->LGUIBehavioursNeedToRemoveFromUpdate.Add(InComp);
					}
				}
				else//not executing update, safe to remove
				{
					updateArray.RemoveAt(index);
				}
			}
			else
			{
				UE_LOG(LGUI, Warning, TEXT("[ALGUIManagerActor::RemoveLGUIBehavioursFromUpdate]Not exist, comp:%s"), *(InComp->GetPathName()));
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
				UE_LOG(LGUI, Warning, TEXT("[ALGUIManagerActor::RemoveLGUIBehavioursFromUpdate]Cleanup %d invalid LGUIBehaviour"), inValidCount);
			}
		}
	}
}

void ALGUIManagerActor::AddLGUIBehavioursForStart(ULGUIBehaviour* InComp)
{
	if (IsValid(InComp))
	{
		if (auto Instance = GetInstance(InComp->GetWorld(), true))
		{
			int32 index = INDEX_NONE;
			if (!Instance->LGUIBehavioursForStart.Find(InComp, index))
			{
				Instance->LGUIBehavioursForStart.Add(InComp);
				return;
			}
			UE_LOG(LGUI, Warning, TEXT("[ALGUIManagerActor::AddLGUIBehavioursForStart]Already exist, comp:%s"), *(InComp->GetPathName()));
		}
	}
}
void ALGUIManagerActor::RemoveLGUIBehavioursFromStart(ULGUIBehaviour* InComp)
{
	if (IsValid(InComp))
	{
		if (auto Instance = GetInstance(InComp->GetWorld(), false))
		{
			auto& startArray = Instance->LGUIBehavioursForStart;
			int32 index = INDEX_NONE;
			if (startArray.Find(InComp, index))
			{
				if (Instance->bIsExecutingStart)
				{
					if (!InComp->isStartCalled)//if already called start then nothing to do, because start array will be cleared after execute start
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
				UE_LOG(LGUI, Warning, TEXT("[ALGUIManagerActor::RemoveLGUIBehavioursFromStart]Not exist, comp:%s"), *(InComp->GetPathName()));
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
				UE_LOG(LGUI, Warning, TEXT("[ALGUIManagerActor::RemoveLGUIBehavioursFromStart]Cleanup %d invalid LGUIBehaviour"), inValidCount);
			}
		}
	}
}



void ALGUIManagerActor::AddUIItem(UUIItem* InItem)
{
	if (auto Instance = GetInstance(InItem->GetWorld(), true))
	{
		Instance->allUIItem.AddUnique(InItem);
	}
}
void ALGUIManagerActor::RemoveUIItem(UUIItem* InItem)
{
	if (auto Instance = GetInstance(InItem->GetWorld()))
	{
		Instance->allUIItem.RemoveSingle(InItem);
	}
}

void ALGUIManagerActor::AddRootUIItem(UUIItem* InItem)
{
	if (auto Instance = GetInstance(InItem->GetWorld(), true))
	{
		Instance->rootUIItems.AddUnique(InItem);
	}
}
void ALGUIManagerActor::RemoveRootUIItem(UUIItem* InItem)
{
	if (auto Instance = GetInstance(InItem->GetWorld()))
	{
		Instance->rootUIItems.RemoveSingle(InItem);
	}
}

void ALGUIManagerActor::RegisterLGUICultureChangedEvent(TScriptInterface<ILGUICultureChangedInterface> InItem)
{
	if (auto Instance = GetInstance(InItem.GetObject()->GetWorld(), true))
	{
		Instance->cultureChanged.AddUnique(InItem);
	}
}
void ALGUIManagerActor::UnregisterLGUICultureChangedEvent(TScriptInterface<ILGUICultureChangedInterface> InItem)
{
	if (auto Instance = GetInstance(InItem.GetObject()->GetWorld()))
	{
		Instance->cultureChanged.RemoveSingle(InItem);
	}
}

void ALGUIManagerActor::UpdateLayout()
{
	SCOPE_CYCLE_COUNTER(STAT_UIItemUpdateLayoutAndGeometry);

	//update Layout
	for (auto item : allLayoutArray)
	{
		if (IsValid(item))
		{
			if (item->GetIsActiveAndEnable())
			{
				item->ConditionalRebuildLayout();
			}
		}
	}


	for (auto item : rootUIItems)
	{
		item->UpdateRootUIItem();
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
		int32 foundIndex = Instance->allCanvas.IndexOfByKey(InCanvas);
		if (foundIndex != INDEX_NONE)
		{
			Instance->allCanvas.RemoveAt(foundIndex);
		}
	}
}
void ALGUIManagerActor::AddCanvas(ULGUICanvas* InCanvas)
{
	RemoveCanvas(InCanvas);
	if (auto Instance = GetInstance(InCanvas->GetWorld(), true))
	{
		auto& canvasArray = Instance->allCanvas;
		canvasArray.AddUnique(InCanvas);
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

void ALGUIManagerActor::AddRaycaster(ULGUIBaseRaycaster* InRaycaster)
{
	if (auto Instance = GetInstance(InRaycaster->GetWorld(), true))
	{
		auto& raycasterArray = Instance->raycasterArray;
		if (raycasterArray.Contains(InRaycaster))return;
		//check multiple racaster
		for (auto item : raycasterArray)
		{
			if (InRaycaster->depth == item->depth && InRaycaster->traceChannel == item->traceChannel)
			{
				auto msg = FString(TEXT("\
Detect multiple LGUIBaseRaycaster components with same depth and traceChannel, this may cause wrong interaction results!\
\neg: Want use mouse to click object A but get object B.\
\nPlease note:\
\n	For LGUIBaseRaycasters with same depth, LGUI will line trace them all and sort result on hit distance.\
\n	For LGUIBaseRaycasters with different depth, LGUI will sort raycasters on depth, and line trace from highest depth to lowest, if hit anything then stop line trace.\
\nLGUIXXXSpaceInteractionXXX is also a LGUIBaseRaycaster component."));
				UE_LOG(LGUI, Warning, TEXT("\n%s"), *msg);
				break;
			}
		}

		raycasterArray.Add(InRaycaster);
		//sort depth
		raycasterArray.Sort([](const ULGUIBaseRaycaster& A, const ULGUIBaseRaycaster& B)
		{
			return A.depth > B.depth;
		});
	}
}
void ALGUIManagerActor::RemoveRaycaster(ULGUIBaseRaycaster* InRaycaster)
{
	if (auto Instance = GetInstance(InRaycaster->GetWorld()))
	{
		int32 index;
		if (Instance->raycasterArray.Find(InRaycaster, index))
		{
			Instance->raycasterArray.RemoveAt(index);
		}
	}
}

void ALGUIManagerActor::SetInputModule(ULGUIBaseInputModule* InInputModule)
{
	if (auto Instance = GetInstance(InInputModule->GetWorld(), true))
	{
		Instance->currentInputModule = InInputModule;
	}
}
void ALGUIManagerActor::ClearInputModule(ULGUIBaseInputModule* InInputModule)
{
	if (auto Instance = GetInstance(InInputModule->GetWorld()))
	{
		Instance->currentInputModule = nullptr;
	}
}

void ALGUIManagerActor::AddSelectable(UUISelectableComponent* InSelectable)
{
	if (auto Instance = GetInstance(InSelectable->GetWorld(), true))
	{
		auto& allSelectableArray = Instance->allSelectableArray;
		if (allSelectableArray.Contains(InSelectable))return;
		allSelectableArray.Add(InSelectable);
	}
}
void ALGUIManagerActor::RemoveSelectable(UUISelectableComponent* InSelectable)
{
	if (auto Instance = GetInstance(InSelectable->GetWorld()))
	{
		int32 index;
		if (Instance->allSelectableArray.Find(InSelectable, index))
		{
			Instance->allSelectableArray.RemoveAt(index);
		}
	}
}

void ALGUIManagerActor::AddLayout(UUILayoutBase* InLayout)
{
	if (auto Instance = GetInstance(InLayout->GetWorld(), true))
	{
		auto& allLayoutArray = Instance->allLayoutArray;
		if (allLayoutArray.Contains(InLayout))return;
		allLayoutArray.Add(InLayout);
	}
}
void ALGUIManagerActor::RemoveLayout(UUILayoutBase* InLayout)
{
	if (auto Instance = GetInstance(InLayout->GetWorld()))
	{
		int32 index;
		if (Instance->allLayoutArray.Find(InLayout, index))
		{
			Instance->allLayoutArray.RemoveAt(index);
		}
	}
}


void ALGUIManagerActor::EndPrefabSystemProcessingActor_Implement()
{
	auto LGUIBehaviourArray = LGUIBehaviours_PrefabSystemProcessing.Pop().LGUIBehaviourArray;
	PrefabSystemProcessing_CurrentArrayIndex--;

	for (int i = 0; i < LGUIBehaviourArray.Num(); i++)
	{
		auto item = LGUIBehaviourArray[i];
		if (item.IsValid())
		{
			ProcessLGUIComponentLifecycleEvent(item.Get());
		}
	}
	LGUIBehaviourArray.Reset();
}
void ALGUIManagerActor::ProcessLGUIComponentLifecycleEvent(ULGUIBehaviour* InComp)
{
	if (InComp)
	{
		if (auto rootComp = InComp->GetRootComponent())
		{
			if (rootComp->GetIsUIActiveInHierarchy())
			{
				if (!InComp->isAwakeCalled)
				{
					InComp->Awake();
				}
				if (InComp->GetIsActiveAndEnable())
				{
					if (!InComp->isEnableCalled)
					{
						InComp->OnEnable();
					}
				}
			}
		}
		else
		{
			if (!InComp->isAwakeCalled)
			{
				InComp->Awake();
			}
			if (InComp->GetIsActiveAndEnable())
			{
				if (!InComp->isEnableCalled)
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
		Instance->LGUIBehaviours_PrefabSystemProcessing.Add({});
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
