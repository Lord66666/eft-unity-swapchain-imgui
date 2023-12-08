#include "LazyImporter.h"
#include <cstdint>
#include <memory>
#include <Windows.h>

using namespace std;

class VMTHook
{
private:
	uintptr_t** ppTable = nullptr;
	size_t functionCount = 0;

	unique_ptr<uintptr_t[]> hookedTable = nullptr;
	uintptr_t* originalTable = nullptr;
public:
	VMTHook();
	~VMTHook();

	bool Initialize(void* ppTable);
	bool CreateHook(void* pDetour, const size_t vIndex);
	bool DisableHook(const size_t vIndex);

	template <typename T = void*> inline const T GetFunctionPtr(size_t vIndex)
	{
		return reinterpret_cast<T>(this->originalTable[vIndex]);
	}
};