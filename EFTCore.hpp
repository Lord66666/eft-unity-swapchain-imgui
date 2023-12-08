#include "Unity.hpp"

struct Camera;
struct GameObjectManager;

class EFTCore
{
public:
	static GameObjectManager* m_GameObjectManager;
	static Camera* m_Camera;
	static uintptr_t m_FPSCamera;

	static Camera* GetCurrentCamera();
	static HWND GetGameWindow();
	static GameObjectManager* GetObjectManager();

	static uint64_t GetGameObject(uint64_t component)
	{
		if (!MemoryUtils::IsAddressValid(component) || !component)
			return 0;

		const auto baseObject = MemoryUtils::Read<uint64_t>(component + 0x10);

		if (!MemoryUtils::IsAddressValid(baseObject) || !baseObject)
			return 0;

		const auto gameObject = MemoryUtils::Read<uint64_t>(baseObject + 0x30);

		if (!MemoryUtils::IsAddressValid(gameObject) || !gameObject)
			return 0;

		return gameObject;
	}

	static uint64_t GetComponentFromObject(uint64_t objectPtr, const char* componentName)
	{
		if (!MemoryUtils::IsAddressValid(objectPtr) || !objectPtr)
			return 0;

		uint64_t components = MemoryUtils::Read<uint64_t>(objectPtr + 0x30);

		if (!MemoryUtils::IsAddressValid(components) || !components)
			return 0;

		for (int i = 0x8; i < 0x300; i += 0x10)
		{
			uint64_t component = MemoryUtils::Read<uint64_t>(MemoryUtils::Read<uint64_t>(components + i) + 0x28);

			if (!MemoryUtils::IsAddressValid(component) || !component)
				continue;

			uintptr_t componentClass = MemoryUtils::Read<uintptr_t>(component + 0x0);

			if (MemoryUtils::IsAddressValid(componentClass) && componentClass)
			{
				componentClass = MemoryUtils::Read<uintptr_t>(componentClass + 0x0);

				if (MemoryUtils::IsAddressValid(componentClass) && componentClass)
				{
					componentClass = MemoryUtils::Read<uintptr_t>(componentClass + 0x0);

					if (MemoryUtils::IsAddressValid(componentClass) && componentClass)
						componentClass = MemoryUtils::Read<uintptr_t>(componentClass + 0x48);
				}
			}

			static char objectName[256] = { };
			MemoryUtils::ReadBuffer(componentClass, &objectName, sizeof(objectName));

			if (objectName != NULL && CRT::StrCmp(objectName, componentName) == 0)
				return component;
		}

		return 0;
	}

	static uint64_t GetObjectFromList(uint64_t listPtr, uint64_t lastObjectPtr, const char* objectName)
	{
		char name[256];
		uint64_t className = 0x0;

		BaseObject activeObject = MemoryUtils::ReadMemory<BaseObject>(listPtr);
		BaseObject lastObject = MemoryUtils::ReadMemory<BaseObject>(lastObjectPtr);

		if (activeObject.object != 0x0)
		{
			while (activeObject.object != 0 && activeObject.object != lastObject.object)
			{
				className = MemoryUtils::ReadMemory<uint64_t>(activeObject.object + 0x60);
				MemoryUtils::ReadBuffer(className + 0x0, &name, sizeof(name));

				if (CRT::StrCmp(name, objectName) == 0)
					return activeObject.object;

				activeObject = MemoryUtils::ReadMemory<BaseObject>(activeObject.nextObjectLink);
			}
		}

		if (lastObject.object != 0x0)
		{
			className = MemoryUtils::ReadMemory<uint64_t>(lastObject.object + 0x60);
			MemoryUtils::ReadBuffer(className + 0x0, &name, 256);

			if (CRT::StrCmp(name, objectName) == 0)
				return lastObject.object;
		}

		return uint64_t();
	}
};