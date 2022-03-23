module SI;

import DMA;
import Memory;
import MI;

#include "../Utils/EnumerateTemplateSpecializations.h"

#define SI_DRAM_ADDR      0x00
#define SI_PIF_ADDR_RD64B 0x04
#define SI_PIF_ADDR_WR4B  0x08
#define SI_PIF_ADDR_WR64B 0x10
#define SI_PIF_ADDR_RD4B  0x14
#define SI_STATUS         0x18

namespace SI
{
	template<StatusFlag status_flag>
	void SetStatusFlag()
	{
		constexpr u8 status_flag_mask = std::to_underlying(status_flag);
		if constexpr (status_flag == StatusFlag::INTERRUPT)
			mem[SI_STATUS + 2] |= status_flag_mask;
		else
			mem[SI_STATUS + 3] |= status_flag_mask;
	}


	template<StatusFlag status_flag>
	void ClearStatusFlag()
	{
		constexpr u8 status_flag_mask = std::to_underlying(status_flag);
		if constexpr (status_flag == StatusFlag::INTERRUPT)
			mem[SI_STATUS + 2] &= ~status_flag_mask;
		else
			mem[SI_STATUS + 3] &= ~status_flag_mask;
	}


	template<std::integral Int>
	Int Read(const u32 addr)
	{
		return Memory::GenericRead<Int>(&mem[addr & 0x1F]);  /* TODO: number of register bytes is 0x1C.. */
	}


	template<std::size_t number_of_bytes>
	void Write(const u32 addr, const auto data)
	{
		/* TODO: for now, only allow word-aligned writes. Force 'data' to be a 32-bit integer. */
		const u32 offset = addr & 0x1C;
		const auto word = static_cast<u32>(data);
		switch (offset)
		{
		case SI_DRAM_ADDR:
			std::memcpy(&mem[SI_DRAM_ADDR], &word, 4);
			break;

		case SI_PIF_ADDR_RD4B:
		{
			std::memcpy(&mem[SI_PIF_ADDR_RD4B], &word, 4);
			const u32 pif_ram_start_addr = Memory::ByteswapOnLittleEndian<u32>(word);
			const u32 rdram_start_addr = Memory::ByteswappedGenericRead<u32>(&mem[SI_DRAM_ADDR]);
			DMA::Init<DMA::Type::SI, DMA::Location::PIF, DMA::Location::RDRAM>(4, pif_ram_start_addr, rdram_start_addr);
			break;
		}

		case SI_PIF_ADDR_RD64B:
		{
			std::memcpy(&mem[SI_PIF_ADDR_RD64B], &word, 4);
			const u32 pif_ram_start_addr = Memory::ByteswapOnLittleEndian<u32>(word);
			const u32 rdram_start_addr = Memory::ByteswappedGenericRead<u32>(&mem[SI_DRAM_ADDR]);
			DMA::Init<DMA::Type::SI, DMA::Location::PIF, DMA::Location::RDRAM>(64, pif_ram_start_addr, rdram_start_addr);
			break;
		}

		case SI_PIF_ADDR_WR4B:
		{
			std::memcpy(&mem[SI_PIF_ADDR_WR4B], &word, 4);
			const u32 pif_ram_start_addr = Memory::ByteswapOnLittleEndian<u32>(word);
			const u32 rdram_start_addr = Memory::ByteswappedGenericRead<u32>(&mem[SI_DRAM_ADDR]);
			DMA::Init<DMA::Type::SI, DMA::Location::RDRAM, DMA::Location::PIF>(4, rdram_start_addr, pif_ram_start_addr);
			break;
		}

		case SI_PIF_ADDR_WR64B:
		{
			std::memcpy(&mem[SI_PIF_ADDR_WR64B], &word, 4);
			const u32 pif_ram_start_addr = Memory::ByteswapOnLittleEndian<u32>(word);
			const u32 rdram_start_addr = Memory::ByteswappedGenericRead<u32>(&mem[SI_DRAM_ADDR]);
			DMA::Init<DMA::Type::SI, DMA::Location::RDRAM, DMA::Location::PIF>(64, rdram_start_addr, pif_ram_start_addr);
			break;
		}

		case SI_STATUS:
			/* Writing any value to SI_STATUS clears bit 12 (SI Interrupt flag), not only here,
			   but also in the RCP Interrupt Cause register and in MI. */
			mem[SI_STATUS + 2] &= ~0x10;
			MI::ClearInterruptFlag<MI::InterruptType::SI>();
			// TODO: RCP flag
			break;

		default: /* TODO */
			break;
		}
	}


	ENUMERATE_TEMPLATE_SPECIALIZATIONS_READ(Read, u32)
	ENUMERATE_TEMPLATE_SPECIALIZATIONS_WRITE(Write, u32)


	template void ClearStatusFlag<StatusFlag::DMA_BUSY>();
	template void ClearStatusFlag<StatusFlag::IO_BUSY>();
	template void ClearStatusFlag<StatusFlag::READ_PENDING>();
	template void ClearStatusFlag<StatusFlag::DMA_ERROR>();
	template void ClearStatusFlag<StatusFlag::INTERRUPT>();
	template void SetStatusFlag<StatusFlag::DMA_BUSY>();
	template void SetStatusFlag<StatusFlag::IO_BUSY>();
	template void SetStatusFlag<StatusFlag::READ_PENDING>();
	template void SetStatusFlag<StatusFlag::DMA_ERROR>();
	template void SetStatusFlag<StatusFlag::INTERRUPT>();
}