export module PeripheralInterface;

import DMA;
import MemoryUtils;
import NumericalTypes;

import <array>;

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

namespace PeripheralInterface
{
	export enum class StatusFlag : u8
	{
		DMA_BUSY = 1 << 0,
		IO_BUSY = 1 << 1,
		DMA_ERROR = 1 << 2,
		DMA_COMPLETED = 1 << 3
	};


	std::array<u8, 0x34> mem{};


	template<std::size_t start, std::size_t number_of_bytes /* how many bytes we want to write */>
	void WriteToDramCartAddr(const auto data)
	{
		constexpr std::size_t number_of_bytes_to_write = /* how many bytes we will actually write (we cannot go past the end of this memory region) */
			number_of_bytes <= 8 - start ? number_of_bytes : 8 - start;
		MemoryUtils::GenericWrite<number_of_bytes_to_write>(&mem[start], data);
	}


	template<std::size_t start, std::size_t number_of_bytes>
	void WriteToRdLen(const auto data)
	{
		constexpr std::size_t number_of_bytes_to_write =
			number_of_bytes <= 4 - start ? number_of_bytes : 4 - start;
		MemoryUtils::GenericWrite<number_of_bytes_to_write>(&mem[start + PI_RD_LEN], data);

		/* Initialize DMA transfer */
		std::size_t dma_length;
		std::memcpy(&dma_length, &mem[PI_RD_LEN], 3); /* total amount of bytes to transfer - 1 */
		dma_length++;

		u32 rdram_start_addr;
		std::memcpy(&rdram_start_addr, &mem[PI_DRAM_ADDR], 3);

		u32 cart_start_addr;
		std::memcpy(&cart_start_addr, &mem[PI_CART_ADDR], 3);

		DMA::Init<DMA::Location::RDRAM, DMA::Location::Cartridge>(dma_length, rdram_start_addr, cart_start_addr);
	}


	template<std::size_t start, std::size_t number_of_bytes>
	void WriteToWrLen(const auto data)
	{
		constexpr std::size_t number_of_bytes_to_write =
			number_of_bytes <= 4 - start ? number_of_bytes : 4 - start;
		MemoryUtils::GenericWrite<number_of_bytes_to_write>(&mem[start + PI_WR_LEN], data);

		/* Initialize DMA transfer */
		std::size_t dma_length;
		std::memcpy(&dma_length, &mem[PI_WR_LEN], 3); /* total amount of bytes to transfer - 1 */
		dma_length++;

		u32 cart_start_addr;
		std::memcpy(&cart_start_addr, &mem[PI_CART_ADDR], 3);

		u32 rdram_start_addr;
		std::memcpy(&rdram_start_addr, &mem[PI_DRAM_ADDR], 3);

		DMA::Init<DMA::Location::Cartridge, DMA::Location::RDRAM>(dma_length, cart_start_addr, rdram_start_addr);
	}


	export
	{
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


		template<std::size_t number_of_bytes>
		auto Read(const u32 addr)
		{
			return MemoryUtils::GenericRead<number_of_bytes>(&mem[addr % mem.size()]);
		}

		template<std::size_t number_of_bytes>
		void Write(const u32 addr, const auto data)
		{
			/* A single branch, and 'number_of_bytes' being passed as a template argument
			   to 'WriteToDramCartAddr' etc means that memcpy will be optimized to mov. */
			switch (addr % mem.size())
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
				break;

			case PI_RD_LEN + 2:
				WriteToRdLen<2, number_of_bytes>(data);
				break;

			case PI_RD_LEN + 3:
				WriteToRdLen<3, number_of_bytes>(data);
				break;

			case PI_WR_LEN:
				WriteToWrLen<0, number_of_bytes>(data);
				break;

			case PI_WR_LEN + 1:
				WriteToWrLen<1, number_of_bytes>(data);
				break;

			case PI_WR_LEN + 2:
				WriteToWrLen<2, number_of_bytes>(data);
				break;

			case PI_WR_LEN + 3:
				WriteToWrLen<3, number_of_bytes>(data);
				break;

			case PI_STATUS:
				if (data & 1)
				{ /* Reset the DMA controller and stop any transfer being done */
					/* TODO */
				}
				if (data & 2)
				{ /* Clear Interrupt (DMA completed) flag (bit 3 of STATUS) */
					ClearStatusFlag<StatusFlag::DMA_COMPLETED>();
				}
				break;

			default: /* TODO */
				break;
			}
		}
	}
}