#include "EFTMath.hpp"

#include <dxgi.h>

#define JOIN_IMPL(A, B) A ## B
#define JOIN(A, B)	    JOIN_IMPL(A, B)

#define FIELD_PAD(Size) uint8_t JOIN(__pad, __COUNTER__)[Size] = { }

class D3D11SwapChain;

class D3D11SwapChain
{
public:
	IDXGISwapChain* GetSwapChain() const;

protected:
	FIELD_PAD(0x3E0); // E8 ? ? ? ? 4C 8B F0 48 85 C0 0F 84 ? ? ? ? 48 8B 08 48 8B 79 40 E8 ? ? ? ? 8B D8 E8 ? ? ? ? 44 8B
	IDXGISwapChain* m_SwapChain = nullptr;
};

class GameObject
{
public:
	const char* GetName();
};

struct BaseObject
{
	uint64_t previousObjectLink;
	uint64_t nextObjectLink;
	uint64_t object;
};

struct GameObjectManager
{
	FIELD_PAD(0x10);
	uint64_t lastTaggedObject;
	uint64_t taggedObjects;
	uint64_t lastActiveObject;
	uint64_t activeObjects;
};

template <typename T>
class ListInternal
{
public:
	//char pad_0x0000[0x20];
	FIELD_PAD(0x20);
	T* entries[128];
};

template <typename T>
class List
{
public:
	FIELD_PAD(0x10);
	ListInternal<T>* listBase;
	__int32 itemCount;
};

class Unity
{
public:
	static uint64_t UnityPlayer;
	static uint64_t UnityGameWindow;
	static uint64_t UnitySwapChainPtr;
	static uint64_t UnityGameObjectManager;

	static float displayWidth;
	static float displayHeight;
	
	static BOOL Init();

	static D3D11SwapChain* GetD3D11SwapChain();
};