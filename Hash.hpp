#include <cstdint>

namespace secure_hasher
{
	constexpr std::uint64_t HashPrime = 1099511628211;
	constexpr std::uint64_t HashBasis = 14695981039346656037;

	template< typename Type >
	constexpr bool IsTerminator(const Type character)
	{
		return (character == static_cast<const Type>(0));
	}

	template< typename Type >
	constexpr bool IsUpper(const Type character)
	{
		return (character >= static_cast<const Type>(65) && character <= static_cast<const Type>(90));
	}

	template< typename Type >
	constexpr Type ToLower(const Type character)
	{
		if (IsUpper(character))
			return (character + static_cast<const Type>(32));

		return character;
	}

	template< typename Type >
	constexpr size_t GetLength(const Type* const data)
	{
		size_t length = 0;

		while (true)
		{
			if (IsTerminator(data[length]))
				break;

			length++;
		}

		return length;
	}

	template< typename Type >
	constexpr std::uint64_t HashCompute(std::uint64_t hash, const Type* const data, size_t size, bool ignore_case)
	{
		const auto element = static_cast<std::uint64_t>(ignore_case ? ToLower(data[0]) : data[0]);
		return (size == 0) ? hash : HashCompute((hash * HashPrime) ^ element, data + 1, size - 1, ignore_case);
	}

	template< typename Type >
	constexpr std::uint64_t Hash(const Type* const data, size_t size, bool ignore_case)
	{
		return HashCompute(HashBasis, data, size, ignore_case);
	}

	constexpr std::uint64_t Hash(const char* const data, bool ignore_case)
	{
		const auto length = GetLength(data);
		return Hash(data, length, ignore_case);
	}

	constexpr std::uint64_t Hash(const wchar_t* const data, bool ignore_case)
	{
		const auto length = GetLength(data);
		return Hash(data, length, ignore_case);
	}
}

#define HASH( Data )																							\
	[ & ]()																													\
	{																																\
		auto hash = secure_hasher::Hash( Data, true );	\
		return hash;																									\
	}()