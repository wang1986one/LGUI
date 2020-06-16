﻿// Copyright 2019-2020 LexLiu. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "LGUIPointerEventData.h"
#include "LGUIPointerSelectDeselectInterface.generated.h"


UINTERFACE(Blueprintable, MinimalAPI)
class ULGUIPointerSelectDeselectInterface : public UInterface
{
	GENERATED_BODY()
};
//select, deselect
class LGUI_API ILGUIPointerSelectDeselectInterface
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = LGUI)
		bool OnPointerSelect(ULGUIPointerEventData* eventData);
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = LGUI)
		bool OnPointerDeselect(ULGUIPointerEventData* eventData);
};