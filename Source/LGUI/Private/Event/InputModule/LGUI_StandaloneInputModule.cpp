﻿// Copyright 2019-2020 LexLiu. All Rights Reserved.

#include "Event/InputModule/LGUI_StandaloneInputModule.h"
#include "LGUI.h"
#include "Event/LGUIEventSystem.h"

void ULGUI_StandaloneInputModule::ProcessInput()
{
	auto eventSystem = ULGUIEventSystem::GetLGUIEventSystemInstance();
	if (eventSystem == nullptr)return;

	auto leftButtonEventData = GetPointerEventData(0, true);
	FVector2D mousePos;
	this->GetWorld()->GetGameViewport()->GetMousePosition(mousePos);
	leftButtonEventData->pointerPosition = FVector(mousePos, 0);

	FHitResultContainerStruct hitResultContainer;
	bool lineTraceHitSomething = LineTrace(leftButtonEventData, hitResultContainer);
	bool resultHitSomething = false;
	ProcessPointerEvent(leftButtonEventData, lineTraceHitSomething, hitResultContainer, resultHitSomething);

	auto tempHitComp = (USceneComponent*)hitResultContainer.hitResult.Component.Get();
	hitResultContainer.hitResult.Component = nullptr;
	eventSystem->RaiseHitEvent(resultHitSomething, hitResultContainer.hitResult, tempHitComp);
}
void ULGUI_StandaloneInputModule::InputScroll(const float& inAxisValue)
{
	auto eventData = GetPointerEventData(0, true);
	if (IsValid(eventData->enterComponent))
	{
		if (inAxisValue != 0)
		{
			eventData->scrollAxisValue = inAxisValue;
			if (auto eventSystem = ULGUIEventSystem::GetLGUIEventSystemInstance())
			{
				eventSystem->CallOnPointerScroll(eventData->enterComponent, eventData, eventData->enterComponentEventFireOnAllOrOnlyTarget);
			}
		}
	}
}

void ULGUI_StandaloneInputModule::InputTrigger(bool inTriggerPress, EMouseButtonType inMouseButtonType)
{
	auto eventData = GetPointerEventData(0, true);
	eventData->nowIsTriggerPressed = inTriggerPress;
	if (eventData->nowIsTriggerPressed)
	{
		if (eventData->prevPressTriggerType != inMouseButtonType)
		{
			//clear event, because input trigger change
			ClearEventByID(eventData->pointerID);
			eventData->prevPressTriggerType = inMouseButtonType;
		}
	}
	eventData->mouseButtonType = inMouseButtonType;
}
