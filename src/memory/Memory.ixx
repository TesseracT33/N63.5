export module Memory;

import NumericalTypes;

namespace Memory
{
	export
	template<typename T = u32 /*temp*/>
	T ReadPhysical(const u32 physical_address)
	{
		return T();
	}
}