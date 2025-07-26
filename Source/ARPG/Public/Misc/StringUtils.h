// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

struct FStringUtils
{
	template<typename EnumType>
	static FString EnumToDisplayString(const EnumType EnumeratorValue)
	{
		// For the C++ enum.
		static_assert(TIsEnum<EnumType>::Value, "EnumeratorValue must be enum types.");
		const UEnum* EnumClass = StaticEnum<EnumType>();
		check(EnumClass != nullptr);
		return EnumClass->GetNameStringByValue(static_cast<int64>(EnumeratorValue));
	}

	static FString InsertSpacesBeforeUppercaseSmart(const FString& Input)
	{
		FString Result;
		const int32 Len = Input.Len();

		for (int32 i = 0; i < Len; ++i)
		{
			const TCHAR Char = Input[i];
			if (i == 0)
			{
				Result.AppendChar(Char);
			}
			else
			{
				bool bIsCurrentUpper = FChar::IsUpper(Char);
				bool bIsPrevUpper = FChar::IsUpper(Input[i - 1]);

				// 현재가 대문자고, 이전이 소문자인 경우에만 공백 추가
				if (bIsCurrentUpper && !bIsPrevUpper)
				{
					Result.AppendChar(TEXT(' '));
				}
				Result.AppendChar(Char);
			}
		}
		return Result;
	}
};
