#include "Hash.hpp"
#include <vector>

struct PriceData
{
	std::uint64_t m_hash = 0;
	std::uint32_t m_price = 0;
	const char* m_name = { };
};

extern std::vector< PriceData > m_market_items_array;