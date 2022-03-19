module PI;

import DMA;
import Memory;
import MI;

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
	void ApplyWriteToRdLen()
	{
		/* Initialize DMA transfer */
		u32 dma_length = Memory::ByteswappedGenericRead<u32>(&mem[PI_RD_LEN]) + 1;
		u32 rdram_start_addr = Memory::ByteswappedGenericRead<u32>(&mem[PI_DRAM_ADDR]);
		u32 cart_start_addr = Memory::ByteswappedGenericRead<u32>(&mem[PI_CART_ADDR]);

		DMA::Init<DMA::Location::RDRAM, DMA::Location::Cartridge>(dma_length, rdram_start_addr, cart_start_addr);
		SetStatusFlag<StatusFlag::DMA_BUSY>();
		ClearStatusFlag<StatusFlag::DMA_COMPLETED>();
		MI::ClearInterruptFlag<MI::InterruptType::PI>();
	}


	void ApplyWriteToWrLen()
	{
		/* Initialize DMA transfer */
		u32 dma_length = Memory::ByteswappedGenericRead<u32>(&mem[PI_WR_LEN]) + 1;
		u32 rdram_start_addr = Memory::ByteswappedGenericRead<u32>(&mem[PI_DRAM_ADDR]);
		u32 cart_start_addr = Memory::ByteswappedGenericRead<u32>(&mem[PI_CART_ADDR]);

		DMA::Init<DMA::Location::Cartridge, DMA::Location::RDRAM>(dma_length, cart_start_addr, rdram_start_addr);
		SetStatusFlag<StatusFlag::DMA_BUSY>();
		ClearStatusFlag<StatusFlag::DMA_COMPLETED>();
		MI::ClearInterruptFlag<MI::InterruptType::PI>();
	}


	template<StatusFlag status_flag>
	void SetStatusFlag()
	{
		mem[PI_STATUS + 3] |= static_cast<u8>(status_flag);
	}


	template<StatusFlag status_flag>
	void ClearStatusFlag()
	{
		mem[PI_STATUS + 3] &= ~static_cast<u8>(status_flag);
	}


	template<std::integral Int>
	Int Read(const u32 addr)
	{
		return Memory::GenericRead<Int>(&mem[addr & 0x3F]); /* TODO: number of bytes for registers is 0x34 */
	}


	template<std::size_t number_of_bytes>
	void Write(const u32 addr, const auto data)
	{
		/* TODO: for now, only allow word-aligned writes. Force 'data' to be a 32-bit integer. */
		const u32 offset = addr & 0x3C; /* TODO: number of bytes for registers is 0x34 */
		const u32 word = static_cast<u32>(data);
		switch (offset) 
		{
		case PI_DRAM_ADDR:
			std::memcpy(&mem[PI_DRAM_ADDR], &word, 4);
			break;

		case PI_CART_ADDR:
			std::memcpy(&mem[PI_CART_ADDR], &word, 4);
			break;

		case PI_RD_LEN:
			std::memcpy(&mem[PI_RD_LEN], &word, 4);
			ApplyWriteToRdLen();
			break;

		case PI_WR_LEN:
			std::memcpy(&mem[PI_WR_LEN], &word, 4);
			ApplyWriteToWrLen();
			break;

		case PI_STATUS:
		{
			constexpr static u32 reset_dma_mask = Memory::ByteswapOnLittleEndian<u32>(0x01);
			constexpr static u32 clear_interrupt_mask = Memory::ByteswapOnLittleEndian<u32>(0x02);
			if (data & reset_dma_mask)
			{ /* Reset the DMA controller and stop any transfer being done */
				mem[PI_STATUS + 3] = 0;
				MI::ClearInterruptFlag<MI::InterruptType::PI>();

			}
			if (data & clear_interrupt_mask)
			{ /* Clear Interrupt */
				ClearStatusFlag<StatusFlag::DMA_COMPLETED>();
				MI::ClearInterruptFlag<MI::InterruptType::PI>();
			}
			break;
		}

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