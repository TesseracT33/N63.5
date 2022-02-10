module RSP;

import Memory;

#include "../Utils/EnumerateTemplateSpecializations.h"

namespace RSP
{
	template<std::integral Int>
	Int ReadMemory(const u32 addr)
	{
		return Int(0);
	}


	template<std::size_t number_of_bytes>
	void WriteMemory(const u32 addr, const auto data)
	{

	}


	ENUMERATE_TEMPLATE_SPECIALIZATIONS_READ(ReadMemory, const u32)
	ENUMERATE_TEMPLATE_SPECIALIZATIONS_WRITE(WriteMemory, const u32)
}