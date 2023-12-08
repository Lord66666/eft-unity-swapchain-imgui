#include "EFTCore.hpp"

GameObjectManager* EFTCore::m_GameObjectManager = nullptr;
Camera* EFTCore::m_Camera = nullptr;
uintptr_t EFTCore::m_FPSCamera = 0;

typedef Camera* (__fastcall* tGetCamera)();
tGetCamera GetCamera = nullptr;

Camera* EFTCore::GetCurrentCamera()
{
	uintptr_t procedure = Unity::UnityPlayer + 0x92C250; // UnityEngine.Camera::get_main

	if (!MemoryUtils::IsUserAddress(procedure))
		return nullptr;

	GetCamera = (tGetCamera) procedure;

	return GetCamera();
}

HWND EFTCore::GetGameWindow()
{
	return MemoryUtils::Read<HWND>(Unity::UnityGameWindow);
}

GameObjectManager* EFTCore::GetObjectManager()
{
	return MemoryUtils::Read<GameObjectManager*>(Unity::UnityGameObjectManager);
}