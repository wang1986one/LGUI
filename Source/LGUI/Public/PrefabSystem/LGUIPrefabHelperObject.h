﻿// Copyright 2019-2022 LexLiu. All Rights Reserved.

#pragma once

#include "LGUI.h"
#include "Components/SceneComponent.h"
#include "LGUIPrefabHelperObject.generated.h"

class ULGUIPrefab;
class ULGUIPrefabOverrideParameterObject;
struct FLGUISubPrefabData;
class AActor;

/**
 * helper object for manage prefab's load/save
 */
UCLASS(ClassGroup = (LGUI), NotBlueprintType, NotBlueprintable)
class LGUI_API ULGUIPrefabHelperObject : public UObject
{
	GENERATED_BODY()

public:	
	ULGUIPrefabHelperObject();

	virtual void PostInitProperties()override;
	virtual bool IsEditorOnly() const { return false; }

	UPROPERTY(VisibleAnywhere, Category = "LGUI")
		ULGUIPrefab* PrefabAsset = nullptr;
	UPROPERTY(VisibleAnywhere, Category = "LGUI")
		AActor* LoadedRootActor;
	UPROPERTY(VisibleAnywhere, Category = "LGUI")
		TArray<AActor*> AllLoadedActorArray;
	UPROPERTY(VisibleAnywhere, Category = "LGUI")
		TMap<FGuid, UObject*> MapGuidToObject;
	UPROPERTY(VisibleAnywhere, Category = "LGUI")
		TMap<AActor*, FLGUISubPrefabData> SubPrefabMap;
#if WITH_EDITORONLY_DATA
	UPROPERTY(VisibleAnywhere, Category = "LGUI")
		FDateTime TimePointWhenSavePrefab;
	UPROPERTY(VisibleAnywhere, Category = "LGUI")
		bool bIsInsidePrefabEditor = true;
#endif
	void RevertPrefab();
	void LoadPrefab(UWorld* InWorld, USceneComponent* InParent);
#if WITH_EDITOR
	virtual void BeginDestroy()override;
	virtual void PostEditUndo()override;

	static void SetActorPropertyInOutliner(AActor* Actor, bool InListed);

	//make sub prefab's actors as normal actor
	void UnlinkSubPrefab(AActor* InSubPrefabActor);
	void UnlinkPrefab(AActor* InPrefabActor);
	ULGUIPrefab* GetSubPrefabAsset(AActor* InSubPrefabActor);
	void SavePrefab();
	void ClearLoadedPrefab();
	bool IsActorBelongsToSubPrefab(const AActor* InActor);
	bool IsActorBelongsToThis(const AActor* InActor, bool InCludeSubPrefab);
	void AddMemberPropertyToSubPrefab(AActor* InSubPrefabActor, UObject* InObject, FName InPropertyName);
	void RemoveMemberPropertyFromSubPrefab(AActor* InSubPrefabActor, UObject* InObject, FName InPropertyName);
	void RemoveAllMemberPropertyFromSubPrefab(AActor* InSubPrefabActor, UObject* InObject);
	FLGUISubPrefabData GetSubPrefabData(AActor* InSubPrefabActor);
#endif
};