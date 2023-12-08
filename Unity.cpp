#include "Unity.hpp"

uint64_t Unity::UnityPlayer = 0;
uint64_t Unity::UnityGameWindow = 0;
uint64_t Unity::UnitySwapChainPtr = 0;
uint64_t Unity::UnityGameObjectManager = 0;

float Unity::displayWidth = 0.0f;
float Unity::displayHeight = 0.0f;

IDXGISwapChain* D3D11SwapChain::GetSwapChain() const
{
	return m_SwapChain;
}

BOOL Unity::Init()
{
	LPCSTR lpUnityPlayer = xor ("UnityPlayer.dll");

	Unity::UnityPlayer = ModuleUtils::GetModuleBase(lpUnityPlayer);

	if (!Unity::UnityPlayer)
	{
		return FALSE;
	}

	auto address = ModuleUtils::PatternScanInModule(Unity::UnityPlayer, xor ("48 8B 0D ? ? ? ? E8 ? ? ? ? 48 8B 03 48 8B CB 48 83 C4 20 5B")); 

	if (!address)
	{
		return FALSE;
	}

	auto SwapChainPtrVA = ToRVA(address, 7);

	Unity::UnitySwapChainPtr = reinterpret_cast<uint64_t>(SwapChainPtrVA);

	address = ModuleUtils::PatternScanInModule(Unity::UnityPlayer, xor ("48 8B 15 ? ? ? ? 48 83 C2 20 48 3B DA 74 36"));

	if (!address)
	{
		return FALSE;
	}

	auto GameObjectManagerPtrVA = ToRVA(address, 7);

	Unity::UnityGameObjectManager = reinterpret_cast<uint64_t>(GameObjectManagerPtrVA);

	address = ModuleUtils::PatternScanInModule(Unity::UnityPlayer, xor ("48 8B 1D ? ? ? ? 48 8B F8 E8 ? ? ? ? 84 C0"));

	if (!address)
	{
		return FALSE;
	}

	auto GameWindowPtrVA = ToRVA(address, 7);;

	Unity::UnityGameWindow = reinterpret_cast<uint64_t>(GameWindowPtrVA);

	return TRUE;
}

D3D11SwapChain* Unity::GetD3D11SwapChain()
{
	return MemoryUtils::Read<D3D11SwapChain*>(Unity::UnitySwapChainPtr);
}

const char* GameObject::GetName()
{
	return MemoryUtils::Read<const char*>((uintptr_t)this + 0x60);
}