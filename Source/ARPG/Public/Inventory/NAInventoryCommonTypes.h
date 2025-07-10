#pragma once

// 슬롯 개수
static constexpr int32 InventoryRowCount = 5;
static constexpr int32 InventoryColumnCount = 5;
static constexpr int32 MaxInventorySlotCount = InventoryRowCount * InventoryColumnCount;
static constexpr int32 MaxWeaponSlotCount = 4;
static constexpr int32 MaxTotalSlots = MaxInventorySlotCount + MaxWeaponSlotCount;

// 슬롯 ID 문자열 포맷
static constexpr TCHAR InventorySlotFormat[] = TEXT("Inven_%02d");
static constexpr TCHAR WeaponSlotFormat[]    = TEXT("Weapon_%02d");

static constexpr TCHAR InventorySlotPrefix[] = TEXT("Inven_");
static constexpr TCHAR WeaponSlotPrefix[]    = TEXT("Weapon_");


// 슬롯 ID에서 번호 추출 헬퍼 함수
// @param	SlotID: xxx_00 형식을 전제
static int32 ExtractSlotNumber(const FName& SlotID)
{
	const FString SlotStr = SlotID.ToString();
	int32 UnderscoreIndex = INDEX_NONE;
	if (SlotStr.FindLastChar(TEXT('_'), UnderscoreIndex))
	{
		const FString NumberStr = SlotStr.Mid(UnderscoreIndex + 1);
		if (NumberStr.IsNumeric())
		{
			return FCString::Atoi(*NumberStr);
		}
		else
		{
			// 숫자가 아님: 예외 케이스
			ensureAlwaysMsgf(false, TEXT("[ExtractSlotNumber] 잘못된 SlotID(숫자 아님): %s"), *SlotStr);
			return INT_MAX;
		}
	}
	else
	{
		// 언더바 없음: 예외 케이스
		ensureAlwaysMsgf(false, TEXT("[ExtractSlotNumber] 언더바 없는 SlotID: %s"), *SlotStr);
		return INT_MAX;
	}
}

static bool ConvertIndexToInvenGrid(int32 Index, int32& OutRow, int32& OutCol)
{
	constexpr int32 NumCols = InventoryColumnCount;
	constexpr int32 MaxIndex = MaxInventorySlotCount - 1;
	if (!ensureAlwaysMsgf(Index >= 0 && Index <= MaxIndex, TEXT("Index out of range: %d"), Index))
	{
		return false;
	}
	OutRow = Index / NumCols;
	OutCol = Index % NumCols;
	return true;
}
