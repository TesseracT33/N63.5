module PI;

import DMA;
import Memory;

#include "../Utils/EnumerateTemplateSpecializations.h"

/* Register addresses (offsets; repeat every $34 bytes starting from $0460'0000) */
#define PI_DRAM_ADDR    0x00 /* R/W */
#define PI_CART_ADDR    0x04 /* R/W */
#define PI_RD_LEN       0x08 /* R/W */
#define PI_WR_LEN       0x0C /* R/W */
#define PI_STATUS       0x10 /* R/W */
#define PI_BSD_DOM1_LAT 0x14 /* R/W */
#define PI_BSD_DOM1_PWD 0x18 /* R/W */
#define PI_BSD_DOM1_PGS 0x1C /* R/W */
#define PI_BSD_DOM1_RLS 0x20 /* R/W */
#define PI_BSD_DOM2_LAT 0x24 /* R/W */
#define PI_BSD_DOM2_PWD 0x28 /* R/W */
#define PI_BSD_DOM2_PGS 0x2C /* R/W */
#define PI_BSD_DOM2_RLS 0x30 /* R/W */

namespace PI
{
	template<std::size_t start, std::size_t number_of_bytes /* how many bytes we want to write */>
	void WriteToDramCartAddr(const auto data)
	{
		constexpr std::size_t number_of_bytes_to_write = /* how many bytes we will actually write (we cannot go past the end of this memory region) */
			number_of_bytes <= 8 - start ? number_of_bytes : 8 - start;
		Memory::GenericWrite<number_of_bytes_to_write>(&mem[start], data);
	}


	template<std::size_t start, std::size_t number_of_bytes>
	void WriteToRdLen(const auto data)
	{
		constexpr std::size_t number_of_bytes_to_write =
			number_of_bytes <= 4 - start ? number_of_bytes : 4 - start;
		Memory::GenericWrite<number_of_bytes_to_write>(&mem[start + PI_RD_LEN], data);

		/* Initialize DMA transfer */
		u32 dma_length = Memory::ByteswappedGenericRead<u32>(&mem[PI_RD_LEN]) + 1;
		u32 rdram_start_addr = Memory::ByteswappedGenericRead<u32>(&mem[PI_DRAM_ADDR]);
		u32 cart_start_addr = Memory::ByteswappedGenericRead<u32>(&mem[PI_CART_ADDR]);

		DMA::Init<DMA::Location::RDRAM, DMA::Location::Cartridge>(dma_length, rdram_start_addr, cart_start_addr);
		SetStatusFlag<StatusFlag::DMA_BUSY>();
	}


	template<std::size_t start, std::size_t number_of_bytes>
	void WriteToWrLen(const auto data)
	{
		constexpr std::size_t number_of_bytes_to_write =
			number_of_bytes <= 4 - start ? number_of_bytes : 4 - start;
		Memory::GenericWrite<number_of_bytes_to_write>(&mem[start + PI_WR_LEN], data);

		/* Initialize DMA transfer */
		u32 dma_length = Memory::ByteswappedGenericRead<u32>(&mem[PI_WR_LEN]) + 1;
		u32 rdram_start_addr = Memory::ByteswappedGenericRead<u32>(&mem[PI_DRAM_ADDR]);
		u32 cart_start_addr = Memory::ByteswappedGenericRead<u32>(&mem[PI_CART_ADDR]);

		DMA::Init<DMA::Location::Cartridge, DMA::Location::RDRAM>(dma_length, cart_start_addr, rdram_start_addr);
		SetStatusFlag<StatusFlag::DMA_BUSY>();
	}


	void WriteToStatus(const u8 data)
	{
		if (data & 1)
		{ /* Reset the DMA controller and stop any transfer being done */
			mem[PI_STATUS] = 0;
			/* TODO */
		}
		if (data & 2)
		{ /* Clear Interrupt (DMA completed) flag (bit 3 of STATUS) */
			ClearStatusFlag<StatusFlag::DMA_COMPLETED>();
		}
	}


	template<StatusFlag status_flag>
	void SetStatusFlag()
	{
		mem[PI_STATUS] |= static_cast<u8>(status_flag);
	}


	template<StatusFlag status_flag>
	void ClearStatusFlag()
	{
		mem[PI_STATUS] &= ~static_cast<u8>(status_flag);
	}


	template<std::integral Int>
	Int Read(const u32 addr)
	{
		return Memory::GenericRead<Int>(&mem[addr & 0x3F]); /* TODO: number of bytes for registers is 0x34 */
	}


	template<std::size_t number_of_bytes>
	void Write(const u32 addr, const auto data)
	{
		/* A single branch, and 'number_of_bytes' being passed as a template argument
		   to 'WriteToDramCartAddr' etc means that memcpy will be optimized to mov. */
		switch (addr & 0x3F) /* TODO: number of bytes for registers is 0x34 */
		{
		case PI_DRAM_ADDR:
			WriteToDramCartAddr<0, number_of_bytes>(data);
			break;

		case PI_DRAM_ADDR + 1:
			WriteToDramCartAddr<1, number_of_bytes>(data);
			if constexpr (number_of_bytes == 8)
				WriteToRdLen<0, number_of_bytes>(data >> 56);
			break;

		case PI_DRAM_ADDR + 2:
			WriteToDramCartAddr<2, number_of_bytes>(data);
			if constexpr (number_of_bytes >= 7)
				WriteToRdLen<0, number_of_bytes>(data >> 48);
			break;

		case PI_DRAM_ADDR + 3:
			WriteToDramCartAddr<3, number_of_bytes>(data);
			if constexpr (number_of_bytes >= 6)
				WriteToRdLen<0, number_of_bytes>(data >> 40);
			break;

		case PI_CART_ADDR:
			WriteToDramCartAddr<4, number_of_bytes>(data);
			if constexpr (number_of_bytes >= 5)
				WriteToRdLen<0, number_of_bytes>(data >> 32);
			break;

		case PI_CART_ADDR + 1:
			WriteToDramCartAddr<5, number_of_bytes>(data);
			if constexpr (number_of_bytes >= 4)
				WriteToRdLen<0, number_of_bytes>(data >> 24);
			break;

		case PI_CART_ADDR + 2:
			WriteToDramCartAddr<6, number_of_bytes>(data);
			if constexpr (number_of_bytes >= 3)
				WriteToRdLen<0, number_of_bytes>(data >> 16);
			break;

		case PI_CART_ADDR + 3:
			WriteToDramCartAddr<7, number_of_bytes>(data);
			if constexpr (number_of_bytes >= 2)
				WriteToRdLen<0, number_of_bytes>(data >> 8);
			break;

		case PI_RD_LEN:
			/* TODO: theoretically, what happens if both PI_RD_LEN and PI_WR_LEN are written to?
			   For now, only care about PI_RD_LEN if that happens. */
			WriteToRdLen<0, number_of_bytes>(data);
			break;

		case PI_RD_LEN + 1:
			WriteToRdLen<1, number_of_bytes>(data);
			if constexpr (number_of_bytes == 8)
				WriteToStatus(u8(data >> 56));
			break;

		case PI_RD_LEN + 2:
			WriteToRdLen<2, number_of_bytes>(data);
			if constexpr (number_of_bytes >= 7)
				WriteToStatus(u8(data >> 48));
			break;

		case PI_RD_LEN + 3:
			WriteToRdLen<3, number_of_bytes>(data);
			if constexpr (number_of_bytes >= 6)
				WriteToStatus(u8(data >> 40));
			break;

		case PI_WR_LEN:
			WriteToWrLen<0, number_of_bytes>(data);
			if constexpr (number_of_bytes >= 5)
				WriteToStatus(u8(data >> 32));
			break;

		case PI_WR_LEN + 1:
			WriteToWrLen<1, number_of_bytes>(data);
			if constexpr (number_of_bytes >= 4)
				WriteToStatus(u8(data >> 24));
			break;

		case PI_WR_LEN + 2:
			WriteToWrLen<2, number_of_bytes>(data);
			if constexpr (number_of_bytes >= 3)
				WriteToStatus(u8(data >> 16));
			break;

		case PI_WR_LEN + 3:
			WriteToWrLen<3, number_of_bytes>(data);
			if constexpr (number_of_bytes >= 2)
				WriteToStatus(u8(data >> 8));
			break;

		case PI_STATUS:
			WriteToStatus(u8(data));
			break;

		default: /* TODO */
			break;
		}
	}


	ENUMERATE_TEMPLATE_SPECIALIZATIONS_READ(Read, const u32);
	ENUMERATE_TEMPLATE_SPECIALIZATIONS_WRITE(Write, const u32);

	template void ClearStatusFlag<StatusFlag::DMA_BUSY>();
	template void ClearStatusFlag<StatusFlag::IO_BUSY>();
	template void ClearStatusFlag<StatusFlag::DMA_ERROR>();
	template void ClearStatusFlag<StatusFlag::DMA_COMPLETED>();
	template void SetStatusFlag<StatusFlag::DMA_BUSY>();
	template void SetStatusFlag<StatusFlag::IO_BUSY>();
	template void SetStatusFlag<StatusFlag::DMA_ERROR>();
	template void SetStatusFlag<StatusFlag::DMA_COMPLETED>();
}