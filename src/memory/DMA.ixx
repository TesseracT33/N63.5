export module DMA;

import NumericalTypes;

namespace DMA
{
	export
	{
		enum class Location
		{
			RDRAM, Cartridge
		};

		template<Location source, Location dest>
		void Init(const size_t length, const u32 source_start_addr, const u32 dest_start_addr);
	}

	template<Location location>
	u8* GetPointerFromAddress(const u32 addr);

	template<Location location>
	size_t GetNumberOfBytesUntilRegionEnd(const u32 addr);
}