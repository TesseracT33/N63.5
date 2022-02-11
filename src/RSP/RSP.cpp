module RSP;

import Memory;

#include "../Utils/EnumerateTemplateSpecializations.h"

namespace RSP
{
	template<std::integral Int>
	Int ReadMemory(const u32 addr)
	{
		if (addr <= 0x04001FFF)
			return Memory::GenericRead<Int>(mem.data() + (addr & 0x1FFF));
		else if (addr <= 0x0403FFFF)
			return Int(0);
		return Int(0);
	}


	template<std::size_t number_of_bytes>
	void WriteMemory(const u32 addr, const auto data)
	{
		if (addr <= 0x04001FFF)
			Memory::GenericWrite<number_of_bytes>(mem.data() + (addr & 0x1FFF), data);
	}


	ENUMERATE_TEMPLATE_SPECIALIZATIONS_READ(ReadMemory, const u32)
	ENUMERATE_TEMPLATE_SPECIALIZATIONS_WRITE(WriteMemory, const u32)
}