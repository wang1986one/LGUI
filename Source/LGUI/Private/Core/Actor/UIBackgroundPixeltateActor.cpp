// Copyright 2019-2020 LexLiu. All Rights Reserved.

#include "Core/Actor/UIBackgroundPixelateActor.h"
#include "LGUI.h"



AUIBackgroundPixelateActor::AUIBackgroundPixelateActor()
{
	PrimaryActorTick.bCanEverTick = false;

	UIBackgroundPixelate = CreateDefaultSubobject<UUIBackgroundPixelate>(TEXT("UIBackgroundPixelateComponent"));
	RootComponent = UIBackgroundPixelate;
}
