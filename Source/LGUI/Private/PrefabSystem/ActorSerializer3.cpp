﻿// Copyright 2019-2021 LexLiu. All Rights Reserved.

#include "PrefabSystem/ActorSerializer3.h"
#include "Engine/World.h"
#include "Components/PrimitiveComponent.h"
#include "Runtime/Launch/Resources/Version.h"
#include "Core/Actor/LGUIManagerActor.h"
#include "LGUI.h"
#include "Core/ActorComponent/UIItem.h"

namespace LGUIPrefabSystem3
{
	ActorSerializer3::ActorSerializer3(UWorld* InTargetWorld)
	{
		TargetWorld = TWeakObjectPtr<UWorld>(InTargetWorld);
	}


	int32 ActorSerializer3::FindOrAddAssetIdFromList(UObject* AssetObject)
	{
		if (!AssetObject)return -1;
		int32 resultIndex;
		if (ReferenceAssetList.Find(AssetObject, resultIndex))
		{
			return resultIndex;//return index if found
		}
		else//add to list if not found
		{
			ReferenceAssetList.Add(AssetObject);
			return ReferenceAssetList.Num() - 1;
		}
	}

	int32 ActorSerializer3::FindOrAddClassFromList(UClass* Class)
	{
		if (!Class)return -1;
		int32 resultIndex;
		if (ReferenceClassList.Find(Class, resultIndex))
		{
			return resultIndex;
		}
		else
		{
			ReferenceClassList.Add(Class);
			return ReferenceClassList.Num() - 1;
		}
	}
	int32 ActorSerializer3::FindOrAddNameFromList(const FName& Name)
	{
		if (!Name.IsValid())return -1;
		int32 resultIndex;
		if (ReferenceNameList.Find(Name, resultIndex))
		{
			return resultIndex;
		}
		else
		{
			ReferenceNameList.Add(Name);
			return ReferenceNameList.Num() - 1;
		}
	}
	FName ActorSerializer3::FindNameFromListByIndex(int32 Id)
	{
		int32 count = ReferenceNameList.Num();
		if (Id >= count || Id < 0)
		{
			return NAME_None;
		}
		return ReferenceNameList[Id];
	}

	UObject* ActorSerializer3::FindAssetFromListByIndex(int32 Id)
	{
		int32 count = ReferenceAssetList.Num();
		if (Id >= count || Id < 0)
		{
			return nullptr;
		}
		return ReferenceAssetList[Id];
	}

	UClass* ActorSerializer3::FindClassFromListByIndex(int32 Id)
	{
		int32 count = ReferenceClassList.Num();
		if (Id >= count || Id < 0)
		{
			return nullptr;
		}
		return ReferenceClassList[Id];
	}

	const TSet<FName>& ActorSerializer3::GetSceneComponentExcludeProperties()
	{
		static TSet<FName> result = { FName("AttachParent") };
		return result;
	}

	static bool CanUseUnversionedPropertySerialization()
	{
		bool bTemp;
		static bool bAllow = GConfig->GetBool(TEXT("Core.System"), TEXT("CanUseUnversionedPropertySerialization"), bTemp, GEngineIni) && bTemp;
		return bAllow;
	}

	void ActorSerializer3::SetupArchive(FArchive& InArchive)
	{
		if (!bIsEditorOrRuntime && CanUseUnversionedPropertySerialization())
		{
			InArchive.SetUseUnversionedPropertySerialization(true);
		}
		InArchive.SetFilterEditorOnly(!bIsEditorOrRuntime);
		InArchive.SetWantBinaryPropertySerialization(!bIsEditorOrRuntime);
	}
}
