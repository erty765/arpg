// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

template<typename BridgeType>
class TNAEdBridgeRegistry
{
public:
	static void Register(BridgeType* Service)
	{
		check(Service);
		Bridge = Service;
	}
	
	static void Unregister()
	{
		Bridge =nullptr;
	}

	static BridgeType* Get()
	{
		checkf(Bridge != nullptr, TEXT("NAEditorBridgeService has not been registered."));
		return Bridge;
	}
	
private:
	static inline BridgeType* Bridge = nullptr;
};