// Fill out your copyright notice in the Description page of Project Settings.

#pragma once
#if WITH_EDITOR

enum class ARPGEDITOR_API EItemEditorRegistrationPhase : uint8
{
	None,                    
	
	DuringInstancing,         // 엔진 초기화/서브시스템 초기화 중 등록됨
	DuringEditorRuntime       // 에디터 런타임 중(엔진 완전 초기화 이후) 등록됨
};


#endif