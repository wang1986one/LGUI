// Copyright 2019 LexLiu. All Rights Reserved.

#pragma once

#include "SlateBasics.h"
#include "ISceneOutliner.h"
#include "Widgets/Input/SComboButton.h"

class SLGUISceneOutlinerButton : public SComboButton
{
protected:
	virtual FReply OnButtonClicked()override;
	virtual FReply OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)override;
	virtual FReply OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent) override;
public:
	TWeakObjectPtr<AActor> _TreeItemActor;
};