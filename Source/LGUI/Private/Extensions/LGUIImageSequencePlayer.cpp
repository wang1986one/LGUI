﻿// Copyright 2019-Present LexLiu. All Rights Reserved.

#include "Extensions/LGUIImageSequencePlayer.h"
#include "LTweenBPLibrary.h"
#include "Core/ActorComponent/UITexture.h"
#include "Core/Actor/LGUIManagerActor.h"

ULGUIImageSequencePlayer::ULGUIImageSequencePlayer()
{
	PrimaryComponentTick.bCanEverTick = false;
}
void ULGUIImageSequencePlayer::BeginPlay()
{
	Super::BeginPlay();	
	if (playOnStart)
	{
		Play();
	}
}

void ULGUIImageSequencePlayer::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
	Stop();
}

void ULGUIImageSequencePlayer::OnRegister()
{
	Super::OnRegister();
#if WITH_EDITOR
	if (GetWorld() && GetWorld()->WorldType == EWorldType::Editor)
	{
		editorPlayDelegateHandle = ULGUIEditorManagerObject::RegisterEditorTickFunction([=](float deltaTime) {
			if (!previewInEditor)return;
			if (!CanPlay())return;
			duration = GetDuration();
			PrepareForPlay();
			UpdateAnimation(deltaTime);
			});
	}
#endif
}
void ULGUIImageSequencePlayer::OnUnregister()
{
	Super::OnUnregister();
#if WITH_EDITOR
	if (editorPlayDelegateHandle.IsValid())
	{
		ULGUIEditorManagerObject::UnregisterEditorTickFunction(editorPlayDelegateHandle);
	}
#endif
}

void ULGUIImageSequencePlayer::Play()
{
	if (!CanPlay())return;
	if (!isPlaying)
	{
		isPlaying = true;
		elapsedTime = 0.0f;
		duration = GetDuration();
		PrepareForPlay();
		playDelegateHandle = ULTweenBPLibrary::RegisterUpdateEvent(this, FLTweenUpdateDelegate::CreateUObject(this, &ULGUIImageSequencePlayer::UpdateAnimation));
		UpdateAnimation(0);
	}
	if (isPaused)
	{
		isPaused = false;
	}
}

void ULGUIImageSequencePlayer::Stop()
{
	if (isPlaying)
	{
		isPlaying = false;
		if (playDelegateHandle.IsValid())
		{
			ULTweenBPLibrary::UnregisterUpdateEvent(this, playDelegateHandle);
		}
	}
}

void ULGUIImageSequencePlayer::SeekFrame(int frameNumber)
{
	elapsedTime = frameNumber / fps;
	if (CanPlay())
	{
		OnUpdateAnimation(frameNumber);
	}
}
void ULGUIImageSequencePlayer::SeekTime(float time)
{
	elapsedTime = time;
	if (CanPlay())
	{
		OnUpdateAnimation(elapsedTime * fps);
	}
}

void ULGUIImageSequencePlayer::UpdateAnimation(float deltaTime)
{
	if (isPaused)return;
	elapsedTime += deltaTime;
	if (elapsedTime > duration)
	{
		if (loop)
		{
			elapsedTime -= duration;
		}
		else
		{
			Stop();
			return;
		}
	}
	int frameNumber = (int)(elapsedTime * fps);
	OnUpdateAnimation(frameNumber);
}

void ULGUIImageSequencePlayer::SetFps(float value)
{
	fps = value;
}
void ULGUIImageSequencePlayer::SetLoop(bool value)
{
	loop = value;
}
