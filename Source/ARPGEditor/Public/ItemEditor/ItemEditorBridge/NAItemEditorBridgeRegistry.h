#pragma once

class ARPGEDITOR_API FNAItemEditorBridgeRegistry
{
public:
	static void RegisterBridgeService(class INAItemEditorBridge* Service)
	{
		check(Service);
		Bridge =Service;
	}

	static void UnregisterBridgeService()
	{
		Bridge =nullptr;
	}

	static INAItemEditorBridge* Get()
	{
		checkf(Bridge != nullptr, TEXT("EditorBridgeService has not been registered."));
		return Bridge;
	}

private:
	static inline INAItemEditorBridge* Bridge = nullptr;
};
