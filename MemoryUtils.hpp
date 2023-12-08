#pragma once
#include "Loader.hpp"

constexpr std::uintptr_t MinimumUserAddress = 0x0000000000010000;
constexpr std::uintptr_t MaximumUserAddress = 0x00007FFFFFFFFFFF;

#include <codecvt>
#include <locale>

class MemoryUtils
{
public:
	static bool IsValidPtr(uint64_t address)
	{
		return (address && sizeof(address)) ? true : false;
	}

	inline static std::uintptr_t ToAddress(const void* pointer)
	{
		return reinterpret_cast<std::uintptr_t>(pointer);
	}

	inline static bool IsUserAddress(std::uintptr_t address)
	{
		return (address >= MinimumUserAddress && address <= MaximumUserAddress);
	}

	inline static bool IsUserAddress(const void* pointer)
	{
		const auto address = ToAddress(pointer);
		return IsUserAddress(address);
	}

	inline static bool IsAddressValid(std::uintptr_t address)
	{
		return IsUserAddress(address);
	}

	inline static bool IsAddressValid(const void* pointer)
	{
		const auto address = ToAddress(pointer);
		return IsAddressValid(address);
	}

	template <typename t> t static Read(uint64_t const address)
	{
		if (IsAddressValid(address))
			return *reinterpret_cast<t*>(address);

		return t();
	}

	template <typename t> static void Write(uint64_t const address, t data)
	{
		*reinterpret_cast<t*>(address) = data;
	}

	static void* memutils_memcpy(void* destination, const void* source, std::size_t size)
	{
		auto data_source = static_cast<const std::uint8_t*>(source);
		auto data_destination = static_cast<std::uint8_t*>(destination);

		__movsb(data_destination, data_source, size);
		return static_cast<void*>(data_destination);
	}

	static BOOL ReadBuffer(uint64_t address, LPVOID lpBuffer, SIZE_T nSize)
	{
		return memutils_memcpy(lpBuffer, (LPVOID)address, nSize) != 0;
	}

	template<typename T>
	static inline T ReadMemory(uint64_t address)
	{
		T buffer{ };
		MemoryUtils::ReadBuffer(address, &buffer, sizeof(T));
		return buffer;
	}

	static std::string GetUnicodeString(uint64_t address, int strLength)
	{
		char16_t wcharTmp[64] = { '\0' };
		ReadBuffer(address, wcharTmp, strLength * 2);

		std::string utfStr = std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t>{}.to_bytes(wcharTmp);

		return utfStr;
	}
};