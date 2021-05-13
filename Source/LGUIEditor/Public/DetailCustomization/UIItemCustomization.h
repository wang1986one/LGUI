// Copyright 2019-2021 LexLiu. All Rights Reserved.

#include "IDetailCustomization.h"
#pragma once

/**
 * 
 */
class FUIItemCustomization : public IDetailCustomization
{
public:
	FUIItemCustomization();
	~FUIItemCustomization();

	static TSharedRef<IDetailCustomization> MakeInstance();
	/** IDetailCustomization interface */
	virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override;
private:
	TArray<TWeakObjectPtr<class UUIItem>> TargetScriptArray;
	FText GetDepthInfo(TWeakObjectPtr<class UUIItem> InTargetScript)const;

	bool GetIsAnchorsEnabled()const;
	FText GetAnchorsTooltipText()const;

	void ForceRefreshEditor(IDetailLayoutBuilder* DetailBuilder);

	void ForceUpdateUI();

	bool OnCanCopyAnchor()const;
	bool OnCanPasteAnchor()const;
	void OnCopyAnchor();
	void OnPasteAnchor(IDetailLayoutBuilder* DetailBuilder);
	void OnCopyHierarchyIndex();
	void OnPasteHierarchyIndex(TSharedRef<IPropertyHandle> PropertyHandle);
	void OnCopyDepth();
	void OnPasteDepth(TSharedRef<IPropertyHandle> PropertyHandle);

	EVisibility GetDisplayNameWarningVisibility()const;
	FReply OnClickFixButton();
};
