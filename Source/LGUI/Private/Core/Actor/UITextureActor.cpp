// Copyright 2019-2021 LexLiu. All Rights Reserved.

#include "Core/Actor/UITextureActor.h"
#include "LGUI.h"



AUITextureActor::AUITextureActor()
{
	PrimaryActorTick.bCanEverTick = false;

	UITexture = CreateDefaultSubobject<UUITexture>(TEXT("UITextureComponent"));
	RootComponent = UITexture;
}
