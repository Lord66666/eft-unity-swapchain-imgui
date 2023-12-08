#include "VMTHook.h"

static void* vmt_memcpy(void* destination, const void* source, std::size_t size)
{
	auto data_source = static_cast<const std::uint8_t*>(source);
	auto data_destination = static_cast<std::uint8_t*>(destination);

	__movsb(data_destination, data_source, size);
	return static_cast<void*>(data_destination);
}

VMTHook::VMTHook()
{

}

VMTHook::~VMTHook()
{
	*this->ppTable = this->originalTable;
}

bool VMTHook::Initialize(void* ppTable)
{
	this->ppTable = reinterpret_cast<uintptr_t**>(ppTable);

	while (static_cast<uintptr_t*>(*this->ppTable)[this->functionCount])
		++this->functionCount;

	const size_t tableSize = this->functionCount * sizeof(uintptr_t);

	this->originalTable = *this->ppTable;
	this->hookedTable = make_unique<uintptr_t[]>(this->functionCount);

	vmt_memcpy(this->hookedTable.get(), this->originalTable, tableSize);

	*this->ppTable = this->hookedTable.get();

	return true;
}

bool VMTHook::CreateHook(void* pDetour, const size_t vIndex)
{
	if (vIndex > this->functionCount)
		return false;

	this->hookedTable[vIndex] = reinterpret_cast<uintptr_t>(pDetour);

	return true;
}

bool VMTHook::DisableHook(const size_t vIndex)
{
	if (vIndex > this->functionCount)
		return false;

	this->hookedTable[vIndex] = this->originalTable[vIndex];

	return true;
}