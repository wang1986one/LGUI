// Copyright 2019-2020 LexLiu. All Rights Reserved.

#include "Core/Actor/UIBaseActor.h"
#include "LGUI.h"
#include "Core/ActorComponent/UIItem.h"


AUIBaseActor::AUIBaseActor()
{
	PrimaryActorTick.bCanEverTick = false;

	SpawnCollisionHandlingMethod = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
}
UUIItem* AUIBaseActor::GetUIItem()const
{
	UE_LOG(LGUI, Error, TEXT("[AUIBaseActor::GetUIItem]This function must be override!"));
	return nullptr;
}