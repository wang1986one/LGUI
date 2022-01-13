﻿// Copyright 2019-2022 LexLiu. All Rights Reserved.

#include "Core/ActorComponent/UIText.h"
#include "LGUI.h"
#include "Core/UIGeometry.h"
#include "Core/ActorComponent/LGUICanvas.h"
#include "Materials/MaterialInterface.h"
#include "Core/LGUIFontData.h"
#include "Core/LGUIFontData_BaseObject.h"
#include "Core/UIDrawcall.h"
#include "Core/Actor/LGUIManagerActor.h"

PRAGMA_DISABLE_OPTIMIZATION

#if WITH_EDITORONLY_DATA
TWeakObjectPtr<ULGUIFontData_BaseObject> UUIText::CurrentUsingFontData = nullptr;
float UUIText::CurrentFontSize = 16;
#endif
UUIText::UUIText(const FObjectInitializer& ObjectInitializer):Super(ObjectInitializer)
{
	PrimaryComponentTick.bCanEverTick = false;
#if WITH_EDITORONLY_DATA
	if (UUIText::CurrentUsingFontData.IsValid())
	{
		font = CurrentUsingFontData.Get();
	}
	size = CurrentFontSize;
#endif
	CacheTextGeometryData = FTextGeometryCache(this);
}
void UUIText::ApplyFontTextureScaleUp()
{
	auto& vertices = geometry->vertices;
	if (vertices.Num() != 0)
	{
		for (int i = 0; i < vertices.Num(); i++)
		{
			auto& uv = vertices[i].TextureCoordinate[0];
			uv *= 0.5f;
		}
		MarkUVDirty();
	}
	geometry->texture = font->GetFontTexture();
	if (RenderCanvas.IsValid())
	{
		if (drawcall.IsValid())
		{
			drawcall->texture = geometry->texture;
			drawcall->textureChanged = true;
		}
	}
	MarkCanvasUpdate(true, true, false);
}

void UUIText::ApplyFontTextureChange()
{
	if (IsValid(font))
	{
		MarkTriangleDirty();
		MarkTextureDirty();
		geometry->texture = font->GetFontTexture();
		if (RenderCanvas.IsValid())
		{
			if (drawcall.IsValid())
			{
				drawcall->texture = geometry->texture;
				drawcall->textureChanged = true;
			}
		}
	}
}

void UUIText::ApplyRecreateText()
{
	if (IsValid(font))
	{
		CacheTextGeometryData.MarkDirty();
		MarkVertexPositionDirty();
	}
}

void UUIText::BeginPlay()
{
	Super::BeginPlay();
	if (IsValid(font))
	{
		font->InitFont();
		if (!bHasAddToFont)
		{
			font->AddUIText(this);
			bHasAddToFont = true;
		}
	}
	visibleCharCount = VisibleCharCountInString(text.ToString());
}

void UUIText::TickComponent( float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction )
{
	Super::TickComponent( DeltaTime, TickType, ThisTickFunction );
}

void UUIText::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
	if (IsValid(font))
	{
		font->RemoveUIText(this);
		bHasAddToFont = false;
	}
}

void UUIText::OnRegister()
{
	Super::OnRegister();
	if (auto World = this->GetWorld())
	{
#if WITH_EDITOR
		if (!World->IsGameWorld())
		{
			if (!bHasAddToFont)
			{
				if (IsValid(font))
				{
					font->AddUIText(this);
					bHasAddToFont = true;
				}
			}
			ULGUIEditorManagerObject::RegisterLGUILayout(this);
		}
		else
#endif
		{
			ALGUIManagerActor::RegisterLGUICultureChangedEvent(this);
			ALGUIManagerActor::RegisterLGUILayout(this);
		}
	}
}
void UUIText::OnUnregister()
{
	Super::OnUnregister();
	if (auto World = this->GetWorld())
	{
#if WITH_EDITOR
		if (!World->IsGameWorld())
		{
			if (IsValid(font))
			{
				font->RemoveUIText(this);
				bHasAddToFont = false;
			}
			ULGUIEditorManagerObject::UnregisterLGUILayout(this);
		}
		else
#endif
		{
			ALGUIManagerActor::UnregisterLGUICultureChangedEvent(this);
			ALGUIManagerActor::UnregisterLGUILayout(this);
		}
	}
	else
	{
		check(0);
	}
}

void UUIText::OnAnchorChange(bool InPivotChange, bool InSizeChange, bool InDiscardCache)
{
    Super::OnAnchorChange(InPivotChange, InSizeChange, InDiscardCache);
    if (InPivotChange || InSizeChange)
    {
        MarkVertexPositionDirty();
        MarkUVDirty();
    }
}

UTexture* UUIText::GetTextureToCreateGeometry()
{
	if (!IsValid(font))
	{
		font = ULGUIFontData::GetDefaultFont();
	}
	font->InitFont();
	return font->GetFontTexture();
}
bool UUIText::HaveDataToCreateGeometry()
{
	return visibleCharCount > 0 && IsValid(font);
}

void UUIText::OnBeforeCreateOrUpdateGeometry()
{
	if (!bHasAddToFont)
	{
		if (IsValid(font))
		{
			font->AddUIText(this);
			bHasAddToFont = true;
		}
	}
	if (visibleCharCount == -1)visibleCharCount = VisibleCharCountInString(text.ToString());
}
void UUIText::OnCreateGeometry()
{
	UpdateCacheTextGeometry();
	int32 vertexCount = geometry->originVerticesCount;
	//normals
	if (RenderCanvas->GetRequireNormal())
	{
		auto& normals = geometry->originNormals;
		if (normals.Num() == 0)
		{
			normals.Reserve(vertexCount);
			for (int i = 0; i < vertexCount; i++)
			{
				normals.Add(FVector(-1, 0, 0));
			}
		}
	}
	//tangents
	if (RenderCanvas->GetRequireTangent())
	{
		auto& tangents = geometry->originTangents;
		if (tangents.Num() == 0)
		{
			tangents.Reserve(vertexCount);
			for (int i = 0; i < vertexCount; i++)
			{
				tangents.Add(FVector(0, 1, 0));
			}
		}
	}
	//uv1
	if (RenderCanvas->GetRequireUV1())
	{
		auto& vertices = geometry->vertices;
		for (int i = 0; i < vertexCount; i += 4)
		{
			vertices[i].TextureCoordinate[1] = FVector2D(0, 1);
			vertices[i + 1].TextureCoordinate[1] = FVector2D(1, 1);
			vertices[i + 2].TextureCoordinate[1] = FVector2D(0, 0);
			vertices[i + 3].TextureCoordinate[1] = FVector2D(1, 0);
		}
	}
}
void UUIText::OnUpdateGeometry(bool InVertexPositionChanged, bool InVertexUVChanged, bool InVertexColorChanged)
{
	if (richText)
	{
		if (InVertexPositionChanged || InVertexUVChanged || InVertexColorChanged)
		{
			UpdateCacheTextGeometry();
		}
	}
	else
	{
		if (InVertexColorChanged && !InVertexPositionChanged && !InVertexUVChanged)
		{
			UIGeometry::UpdateUIColor(geometry, GetFinalColor());
		}
		else if (InVertexPositionChanged || InVertexUVChanged)
		{
			UpdateCacheTextGeometry();
		}
	}
}
void UUIText::OnCultureChanged_Implementation()
{
	static auto emptyText = FText();
	auto originText = text;
	text = emptyText;//just make it work, because SetText will compare text value
	SetText(originText);
}


#if WITH_EDITOR
void UUIText::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	if (auto Property = PropertyChangedEvent.Property)
	{
		auto PropertyName = Property->GetFName();
		if (PropertyName == GET_MEMBER_NAME_CHECKED(UUIText, text))
		{
			
		}
		else if (PropertyName == GET_MEMBER_NAME_CHECKED(UUIText, font))
		{
			UUIText::CurrentUsingFontData = font;
		}
		else if (PropertyName == GET_MEMBER_NAME_CHECKED(UUIText, size))
		{
			UUIText::CurrentFontSize = size;
		}
	}
	Super::PostEditChangeProperty(PropertyChangedEvent);
}
void UUIText::EditorForceUpdate()
{
	Super::EditorForceUpdate();

	visibleCharCount = VisibleCharCountInString(text.ToString());
	if (!IsValid(font))
	{
		font = ULGUIFontData::GetDefaultFont();
		if (IsValid(font))
		{
			font->AddUIText(this);
			bHasAddToFont = true;
		}
	}
}
void UUIText::OnPreChangeFontProperty()
{
	if (IsValid(font))
	{
		font->RemoveUIText(this);
		bHasAddToFont = false;
	}
}
void UUIText::OnPostChangeFontProperty()
{
	if (IsValid(font))
	{
		font->AddUIText(this);
		bHasAddToFont = true;
	}
}
#endif

FVector2D UUIText::GetTextRealSize()
{
	UpdateCacheTextGeometry();
	return CacheTextGeometryData.textRealSize;
}



void UUIText::SetFont(ULGUIFontData_BaseObject* newFont) {
	if (font != newFont)
	{
		//remove from old
		if (IsValid(font))
		{
			font->RemoveUIText(this);
			bHasAddToFont = false;
		}
		font = newFont;

		MarkTextureDirty();
		//add to new
		if (IsValid(font))
		{
			font->AddUIText(this);
			bHasAddToFont = true;
		}
	}
}
void UUIText::SetText(const FText& newText) {
	if (!text.EqualTo(newText))
	{
		text = newText;

		int newVisibleCharCount = VisibleCharCountInString(text.ToString());
		if (newVisibleCharCount != visibleCharCount)//visible char count change
		{
			MarkTriangleDirty();
			visibleCharCount = newVisibleCharCount;
		}
		else//visible char count not change, just mark update vertex and uv
		{
			MarkVertexPositionDirty();
			MarkUVDirty();
		}
	}
}


void UUIText::SetFontSize(float newSize) {
	newSize = FMath::Clamp(newSize, 0.0f, 200.0f);
	if (size != newSize)
	{
		MarkVertexPositionDirty();
		size = newSize;
	}
}
void UUIText::SetFontSpace(FVector2D newSpace) {
	if (space != newSpace)
	{
		MarkVertexPositionDirty();
		space = newSpace;
	}
}
void UUIText::SetParagraphHorizontalAlignment(UITextParagraphHorizontalAlign newHAlign) {
	if (hAlign != newHAlign)
	{
		MarkVertexPositionDirty();
		hAlign = newHAlign;
	}
}
void UUIText::SetParagraphVerticalAlignment(UITextParagraphVerticalAlign newVAlign) {
	if (vAlign != newVAlign)
	{
		MarkVertexPositionDirty();
		vAlign = newVAlign;
	}
}
void UUIText::SetOverflowType(UITextOverflowType newOverflowType) {
	if (overflowType != newOverflowType)
	{
		if (overflowType == UITextOverflowType::ClampContent
			|| newOverflowType == UITextOverflowType::ClampContent
			)
			MarkTriangleDirty();
		else
			MarkVertexPositionDirty();
		overflowType = newOverflowType;
	}
}
void UUIText::SetAdjustWidth(bool newAdjustWidth) {
	if (adjustWidth != newAdjustWidth)
	{
		adjustWidth = newAdjustWidth;
		MarkVertexPositionDirty();
	}
}
void UUIText::SetAdjustHeight(bool newAdjustHeight) {
	if (adjustHeight != newAdjustHeight)
	{
		adjustHeight = newAdjustHeight;
		MarkVertexPositionDirty();
	}
}
void UUIText::SetFontStyle(UITextFontStyle newFontStyle) {
	if (fontStyle != newFontStyle)
	{
		if ((fontStyle == UITextFontStyle::None || fontStyle == UITextFontStyle::Italic)
			&& (newFontStyle == UITextFontStyle::None || newFontStyle == UITextFontStyle::Italic))//these only affect vertex position
		{
			MarkVertexPositionDirty();
		}
		else
		{
			MarkTriangleDirty();
		}
		fontStyle = newFontStyle;
	}
}
void UUIText::SetRichText(bool newRichText)
{
	if (richText != newRichText)
	{
		MarkTriangleDirty();
		richText = newRichText;
	}
}

void UUIText::OnUpdateLayout_Implementation()
{
	if (!this->RenderCanvas.IsValid())return;

	if (bTextLayoutDirty)
	{
		if (UpdateCacheTextGeometry())
		{
			bTextLayoutDirty = false;
			if (overflowType == UITextOverflowType::HorizontalOverflow)
			{
				if (adjustWidth) SetWidth(CacheTextGeometryData.textRealSize.X);
			}
			else if (overflowType == UITextOverflowType::VerticalOverflow)
			{
				if (adjustHeight) SetHeight(CacheTextGeometryData.textRealSize.Y);
			}
		}
	}
}

bool UUIText::UpdateCacheTextGeometry()
{
	if (!IsValid(this->GetFont()))return false;

	CacheTextGeometryData.SetInputParameters(
		this->text.ToString()
		, this->visibleCharCount
		, this->GetWidth()
		, this->GetHeight()
		, this->GetPivot()
		, this->GetFinalColor()
		, this->GetFontSpace()
		, this->GetFontSize()
		, this->GetParagraphHorizontalAlignment()
		, this->GetParagraphVerticalAlignment()
		, this->GetOverflowType()
		, this->GetAdjustWidth()
		, this->GetAdjustHeight()
		, this->GetFontStyle()
		, this->GetRichText()
		, this->GetFont()
	);
	if (geometry->vertices.Num() == 0)
	{
		CacheTextGeometryData.MarkDirty();
	}
	CacheTextGeometryData.ConditaionalCalculateGeometry();
	return true;
}

void UUIText::MarkVertexPositionDirty()
{
	bTextLayoutDirty = true;
	CacheTextGeometryData.MarkDirty();
	Super::MarkVertexPositionDirty();
}
void UUIText::MarkUVDirty()
{
	bTextLayoutDirty = true;
	CacheTextGeometryData.MarkDirty();
	Super::MarkUVDirty();
}
void UUIText::MarkTriangleDirty()
{
	bTextLayoutDirty = true;
	CacheTextGeometryData.MarkDirty();
	Super::MarkTriangleDirty();
}
void UUIText::MarkTextureDirty()
{
	bTextLayoutDirty = true;
	CacheTextGeometryData.MarkDirty();
	Super::MarkTextureDirty();
}

void UUIText::MarkAllDirtyRecursive()
{
	CacheTextGeometryData.MarkDirty();
	bTextLayoutDirty = true;
	Super::MarkAllDirtyRecursive();
}
int UUIText::VisibleCharCountInString(const FString& srcStr)
{
	int count = srcStr.Len();
	if (count == 0)return 0;
	int result = 0;
	for (int i = 0; i < count; i++)
	{
		auto charIndexItem = srcStr[i];
		if (IsVisibleChar(charIndexItem) == false)
		{
			continue;
		}
		result++;
	}
	return result;
}
const TArray<FUITextCharProperty>& UUIText::GetCharPropertyArray(bool createIfNotExist)
{
	UpdateCacheTextGeometry();
	return CacheTextGeometryData.cacheCharPropertyArray;
}
const TArray<FUIText_RichTextCustomTag>& UUIText::GetRichTextCustomTagArray(bool createIfNotExist)
{
	UpdateCacheTextGeometry();
	return CacheTextGeometryData.cacheRichTextCustomTagArray;
}





FString UUIText::GetSubStringByLine(const FString& inString, int32& inOutLineStartIndex, int32& inOutLineEndIndex, int32& inOutCharStartIndex, int32& inOutCharEndIndex)
{
	if (inString.Len() == 0)//no text
		return inString;
	SetText(FText::FromString(inString));
	UpdateCacheTextGeometry();
	int lineCount = inOutLineEndIndex - inOutLineStartIndex;
	auto& cacheTextPropertyArray = CacheTextGeometryData.cacheTextPropertyArray;
	if (inOutLineEndIndex + 1 >= cacheTextPropertyArray.Num())
	{
		inOutLineEndIndex = cacheTextPropertyArray.Num() - 1;
		if (lineCount < cacheTextPropertyArray.Num())
		{
			inOutLineStartIndex = inOutLineEndIndex - lineCount;
		}
	}
	if (inOutLineStartIndex < 0)
	{
		inOutLineStartIndex = 0;
		if (lineCount < cacheTextPropertyArray.Num())
		{
			inOutLineEndIndex = inOutLineStartIndex + lineCount;
		}
	}
	inOutCharStartIndex = cacheTextPropertyArray[inOutLineStartIndex].charPropertyList[0].charIndex;
	auto& endLine = cacheTextPropertyArray[inOutLineEndIndex];
	inOutCharEndIndex = endLine.charPropertyList[endLine.charPropertyList.Num() - 1].charIndex;
	return inString.Mid(inOutCharStartIndex, inOutCharEndIndex - inOutCharStartIndex);
}
//caret is at left side of char
void UUIText::FindCaretByIndex(int32 caretPositionIndex, FVector2D& outCaretPosition, int32& outCaretPositionLineIndex)
{
	outCaretPosition.X = outCaretPosition.Y = 0;
	outCaretPositionLineIndex = 0;
	if (text.ToString().Len() == 0)
	{
		float pivotOffsetX = this->GetWidth() * (0.5f - this->GetPivot().X);
		float pivotOffsetY = this->GetHeight() * (0.5f - this->GetPivot().Y);
		switch (hAlign)
		{
		case UITextParagraphHorizontalAlign::Left:
		{
			outCaretPosition.X = pivotOffsetX - this->GetWidth() * 0.5f;
		}
			break;
		case UITextParagraphHorizontalAlign::Center:
		{
			outCaretPosition.X = pivotOffsetX;
		}
			break;
		case UITextParagraphHorizontalAlign::Right:
		{
			outCaretPosition.X = pivotOffsetX + this->GetWidth() * 0.5f;
		}
			break;
		}
		switch (vAlign)
		{
		case UITextParagraphVerticalAlign::Top:
		{
			outCaretPosition.Y = pivotOffsetY + this->GetHeight() * 0.5f - size * 0.5f;//fixed offset
		}
			break;
		case UITextParagraphVerticalAlign::Middle:
		{
			outCaretPosition.Y = pivotOffsetY;
		}
			break;
		case UITextParagraphVerticalAlign::Bottom:
		{
			outCaretPosition.Y = pivotOffsetY - this->GetHeight() * 0.5f + size * 0.5f;//fixed offset
		}
			break;
		}
	}
	else
	{
		UpdateCacheTextGeometry();
		auto& cacheTextPropertyArray = CacheTextGeometryData.cacheTextPropertyArray;

		if (caretPositionIndex == 0)//first char
		{
			outCaretPosition = cacheTextPropertyArray[0].charPropertyList[0].caretPosition;
		}
		else//not first char
		{
			int lineCount = cacheTextPropertyArray.Num();//line count
			if (lineCount == 1)//only one line
			{
				auto& firstLine = cacheTextPropertyArray[0];
				auto& charProperty = firstLine.charPropertyList[caretPositionIndex];
				outCaretPosition = charProperty.caretPosition;
				outCaretPositionLineIndex = 0;
				return;
			}
			//search all lines, find charIndex == caretPositionIndex
			for (int lineIndex = 0; lineIndex < lineCount; lineIndex ++)
			{
				auto& lineItem = cacheTextPropertyArray[lineIndex];
				int lineCharCount = lineItem.charPropertyList.Num();
				for (int lineCharIndex = 0; lineCharIndex < lineCharCount; lineCharIndex++)
				{
					auto& charItem = lineItem.charPropertyList[lineCharIndex];
					if (caretPositionIndex == charItem.charIndex)//found it
					{
						outCaretPosition = charItem.caretPosition;
						outCaretPositionLineIndex = lineIndex;
						return;
					}
				}
			}
		}
	}
}
void UUIText::FindCaretUp(FVector2D& inOutCaretPosition, int32 inCaretPositionLineIndex, int32& outCaretPositionIndex)
{
	if (text.ToString().Len() == 0)//no text
		return;
	UpdateCacheTextGeometry();
	auto& cacheTextPropertyArray = CacheTextGeometryData.cacheTextPropertyArray;
	auto lineCount = cacheTextPropertyArray.Num();//line count
	if (lineCount == 1)//only one line
		return;
	outCaretPositionIndex = 0;

	//find nearest char to caret from this line
	auto& lineItem = cacheTextPropertyArray[inCaretPositionLineIndex];
	int charCount = lineItem.charPropertyList.Num();//char count of this line
	float nearestDistance = MAX_FLT;
	int32 nearestIndex = -1;
	for (int charIndex = 0; charIndex < charCount; charIndex++)
	{
		auto& charItem = lineItem.charPropertyList[charIndex];
		float distance = FMath::Abs(charItem.caretPosition.X - inOutCaretPosition.X);
		if (distance <= nearestDistance)
		{
			nearestDistance = distance;
			nearestIndex = charIndex;
			outCaretPositionIndex = charItem.charIndex;
		}
	}
	inOutCaretPosition = lineItem.charPropertyList[nearestIndex].caretPosition;
}
void UUIText::FindCaretDown(FVector2D& inOutCaretPosition, int32 inCaretPositionLineIndex, int32& outCaretPositionIndex)
{
	if (text.ToString().Len() == 0)//no text
		return;
	UpdateCacheTextGeometry();
	auto& cacheTextPropertyArray = CacheTextGeometryData.cacheTextPropertyArray;
	auto lineCount = cacheTextPropertyArray.Num();//line count
	if (lineCount == 1)//only one line
		return;
	outCaretPositionIndex = 0;

	//find nearest char to caret from this line
	auto& lineItem = cacheTextPropertyArray[inCaretPositionLineIndex];
	int charPropertyCount = lineItem.charPropertyList.Num();//char count of this line
	float nearestDistance = MAX_FLT;
	int32 nearestIndex = -1;
	for (int charPropertyIndex = 0; charPropertyIndex < charPropertyCount; charPropertyIndex++)
	{
		auto& charItem = lineItem.charPropertyList[charPropertyIndex];
		float distance = FMath::Abs(charItem.caretPosition.X - inOutCaretPosition.X);
		if (distance <= nearestDistance)
		{
			nearestDistance = distance;
			nearestIndex = charPropertyIndex;
			outCaretPositionIndex = charItem.charIndex;
		}
	}
	inOutCaretPosition = lineItem.charPropertyList[nearestIndex].caretPosition;
}
//find caret by position, caret is on left side of char
void UUIText::FindCaretByPosition(FVector inWorldPosition, FVector2D& outCaretPosition, int32& outCaretPositionLineIndex, int32& outCaretPositionIndex)
{
	if (text.ToString().Len() == 0)//no text
	{
		outCaretPositionIndex = 0;
		FindCaretByIndex(0, outCaretPosition, outCaretPositionLineIndex);
	}
	else
	{
		UpdateCacheTextGeometry();
		auto& cacheTextPropertyArray = CacheTextGeometryData.cacheTextPropertyArray;

		auto localPosition = this->GetComponentTransform().InverseTransformPosition(inWorldPosition);
		auto localPosition2D = FVector2D(localPosition.Y, localPosition.Z);

		float nearestDistance = MAX_FLT;
		int lineCount = cacheTextPropertyArray.Num();
		//find the nearest line, only need to compare Y
		for (int lineIndex = 0; lineIndex < lineCount; lineIndex++)
		{
			auto& lineItem = cacheTextPropertyArray[lineIndex];
			float distance = FMath::Abs(lineItem.charPropertyList[0].caretPosition.Y - localPosition2D.Y);
			if (distance <= nearestDistance)
			{
				nearestDistance = distance;
				outCaretPositionLineIndex = lineIndex;
			}
		}
		//then find nearest char, only need to compare X
		nearestDistance = MAX_FLT;
		auto& nearestLine = cacheTextPropertyArray[outCaretPositionLineIndex];
		int charCount = nearestLine.charPropertyList.Num();
		for (int charIndex = 0; charIndex < charCount; charIndex++)
		{
			auto& charItem = nearestLine.charPropertyList[charIndex];
			float distance = FMath::Abs(charItem.caretPosition.X - localPosition2D.X);
			if (distance <= nearestDistance)
			{
				nearestDistance = distance;
				outCaretPositionIndex = charItem.charIndex;
				outCaretPosition = charItem.caretPosition;
			}
		}
	}
}

void UUIText::GetSelectionProperty(int32 InSelectionStartCaretIndex, int32 InSelectionEndCaretIndex, TArray<FUITextSelectionProperty>& OutSelectionProeprtyArray)
{
	OutSelectionProeprtyArray.Reset();
	if (text.ToString().Len() == 0)return;
	UpdateCacheTextGeometry();
	auto& cacheTextPropertyArray = CacheTextGeometryData.cacheTextPropertyArray;
	//start
	FVector2D startCaretPosition;
	int32 startCaretPositionLineIndex;
	FindCaretByIndex(InSelectionStartCaretIndex, startCaretPosition, startCaretPositionLineIndex);
	//end
	FVector2D endCaretPosition;
	int32 endCaretPositionLineIndex;
	FindCaretByIndex(InSelectionEndCaretIndex, endCaretPosition, endCaretPositionLineIndex);
	//if select from down to up, then convert it from up to down
	if (startCaretPositionLineIndex > endCaretPositionLineIndex)
	{
		auto tempInt = endCaretPositionLineIndex;
		endCaretPositionLineIndex = startCaretPositionLineIndex;
		startCaretPositionLineIndex = tempInt;
		auto tempV2 = endCaretPosition;
		endCaretPosition = startCaretPosition;
		startCaretPosition = tempV2;
	}
	
	if (startCaretPositionLineIndex == endCaretPositionLineIndex)//same line
	{
		FUITextSelectionProperty selectionProperty;
		selectionProperty.Pos = startCaretPosition;
		selectionProperty.Size = endCaretPosition.X - startCaretPosition.X;
		OutSelectionProeprtyArray.Add(selectionProperty);
	}
	else//different line
	{
		//first line
		FUITextSelectionProperty selectionProperty;
		selectionProperty.Pos = startCaretPosition;
		auto& firstLineCharPropertyList = cacheTextPropertyArray[startCaretPositionLineIndex].charPropertyList;
		auto& firstLineLastCharProperty = firstLineCharPropertyList[firstLineCharPropertyList.Num() - 1];
		selectionProperty.Size = FMath::RoundToInt(firstLineLastCharProperty.caretPosition.X - startCaretPosition.X);
		//selectionProperty.Size = (1.0f - this->GetPivot().X) * this->GetWidth() - startCaretPosition.X;
		OutSelectionProeprtyArray.Add(selectionProperty);
		//middle line, use this->GetWidth() as size
		int middleLineCount = endCaretPositionLineIndex - startCaretPositionLineIndex - 1;
		for (int i = 0; i < middleLineCount; i++)
		{
			auto& charPropertyList = cacheTextPropertyArray[startCaretPositionLineIndex + i + 1].charPropertyList;
			auto& firstPosition = charPropertyList[0].caretPosition;
			auto& lasPosition = charPropertyList[charPropertyList.Num() - 1].caretPosition;
			selectionProperty.Pos = firstPosition;
			selectionProperty.Size = FMath::RoundToInt(lasPosition.X - firstPosition.X);
			OutSelectionProeprtyArray.Add(selectionProperty);
		}
		//end line
		auto& firstPosition = cacheTextPropertyArray[endCaretPositionLineIndex].charPropertyList[0].caretPosition;
		selectionProperty.Pos = firstPosition;
		selectionProperty.Size = FMath::RoundToInt(endCaretPosition.X - firstPosition.X);
		OutSelectionProeprtyArray.Add(selectionProperty);
	}
}

PRAGMA_ENABLE_OPTIMIZATION
