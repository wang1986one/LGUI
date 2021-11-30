// Copyright 2019-2021 LexLiu. All Rights Reserved.

#pragma once

#include "GameFramework/Actor.h"
#include "LGUIPrefabHelperActor.generated.h"

UCLASS(NotBlueprintable, NotBlueprintType, HideCategories = (Rendering, Actor, Input))
class LGUI_API ALGUIPrefabHelperActor : public AActor
{
	GENERATED_BODY()
	
	
public:	
	// Sets default values for this actor's properties
	ALGUIPrefabHelperActor();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void Destroyed() override;

public:
#if WITH_EDITOR
	UFUNCTION(BlueprintCallable, Category = LGUI)
		void LoadPrefab(USceneComponent* InParent);
	UFUNCTION(BlueprintCallable, Category = LGUI)
		void SavePrefab();
	UFUNCTION(BlueprintCallable, Category = LGUI)
		void RevertPrefab();
	//delete this prefab actor
	UFUNCTION(BlueprintCallable, Category = LGUI)
		void DeleteThisInstance();
	UFUNCTION(BlueprintCallable, Category = LGUI)
		void SetPrefabAsset(ULGUIPrefab* InPrefab)
	{
		PrefabAsset = InPrefab;
	}
	UFUNCTION(BlueprintCallable, Category = LGUI)
		ULGUIPrefab* GetPrefabAsset()const { return PrefabAsset; }
	UFUNCTION(BlueprintCallable, Category = LGUI)
		AActor* GetLoadedRootActor()const { return LoadedRootActor; }
	void MoveActorToPrefabFolder();
	void CleanupPrefabAndActor();
private:
#endif
#if WITH_EDITORONLY_DATA
public:
	UPROPERTY(VisibleAnywhere, Category = "LGUI")
		ULGUIPrefab* PrefabAsset;
	UPROPERTY(VisibleAnywhere, Category = "LGUI")
		AActor* LoadedRootActor;
	/** All loaded actor */
	UPROPERTY(VisibleAnywhere, Category = "LGUI")
		TArray<AActor*> AllLoadedActorArray;
	UPROPERTY(VisibleAnywhere, Category = "LGUI")
		TMap<FGuid, UObject*> MapGuidToObject;
	FColor IdentityColor = FColor::Black;
	bool IsRandomColor = true;
	static TArray<FColor> AllColors;
private:
	static FName PrefabFolderName;
#endif
};
