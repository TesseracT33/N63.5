export module SerialInterface;

import MemoryUtils;
import NumericalTypes;

import <array>;

#define SI_DRAM_ADDR      0x00
#define SI_PIF_ADDR_RD64B 0x04
#define SI_PIF_ADDR_WR4B  0x08
#define SI_PIF_ADDR_WR64B 0x10
#define SI_PIF_ADDR_RD4B  0x14
#define SI_STATUS         0x18

namespace SerialInterface
{
	std::array<u8, 0x1C> mem{};

	export
	{
		enum class StatusFlag : u8
		{
			DMA_BUSY = 1 << 0, /* Set when a read or write DMA, or an IO write, is in progress. */
			IO_BUSY = 1 << 1, /* Set when either an IO read or write is in progress. */
			READ_PENDING = 1 << 2, /* Set when an IO read occurs, while an IO or DMA write is in progress. */
			DMA_ERROR = 1 << 3, /* Set when overlapping DMA requests occur. Can only be cleared with a power reset. */
			INTERRUPT = 1 < 4 /* Copy of SI interrupt flag from MIPS Interface, which is also seen in the RCP Interrupt Cause register. Writing any value to SI_STATUS clears this bit in all three locations. */
		};


		template<StatusFlag status_flag>
		void SetStatusFlag()
		{
			constexpr u8 status_flag_mask = static_cast<u8>(status_flag);
			if constexpr (status_flag == StatusFlag::INTERRUPT)
				mem[SI_STATUS + 1] |= status_flag_mask;
			else
				mem[SI_STATUS] |= status_flag_mask;
		}


		template<StatusFlag status_flag>
		void ClearStatusFlag()
		{
			constexpr u8 status_flag_mask = static_cast<u8>(status_flag);
			if constexpr (status_flag == StatusFlag::INTERRUPT)
				mem[SI_STATUS + 1] &= ~status_flag_mask;
			else
				mem[SI_STATUS] &= ~status_flag_mask;
		}


		template<std::size_t number_of_bytes>
		auto Read(const u32 addr)
		{
			return MemoryUtils::GenericRead<number_of_bytes>(&mem[addr % mem.size()]);
		}


		template<std::size_t number_of_bytes>
		void Write(const u32 addr, const auto data)
		{
			switch (addr % mem.size())
			{
			case SI_DRAM_ADDR:
				MemoryUtils::GenericWrite
					<number_of_bytes <= 3 ? number_of_bytes : 3>(&mem[SI_DRAM_ADDR], data);
				break;

			case SI_DRAM_ADDR + 1:
				MemoryUtils::GenericWrite
					<number_of_bytes <= 2 ? number_of_bytes : 2>(&mem[SI_DRAM_ADDR + 1], data);
				break;

			case SI_DRAM_ADDR + 2:
				MemoryUtils::GenericWrite<1>(&mem[SI_DRAM_ADDR + 2], data);
				break;

			default: /* TODO */
				break;
			}
		}
	}
}