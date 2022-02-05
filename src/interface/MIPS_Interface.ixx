export module MIPS_Interface;

import MemoryUtils;
import NumericalTypes;

import <array>;

/* Register addresses (offsets; repeat every 16 bytes starting from $0430'000) */
#define MI_MODE_0      0x0 /* R/W */
#define MI_MODE_1      0x1 /* R/W */
#define MI_MODE_2      0x2 /* R   */
#define MI_MODE_3      0x3 /* R   */
#define MI_VERSION_0   0x4 /* R   */
#define MI_VERSION_1   0x5 /* R   */
#define MI_VERSION_2   0x6 /* R   */
#define MI_VERSION_3   0x7 /* R   */
#define MI_INTERRUPT_0 0x8 /* R   */
#define MI_INTERRUPT_1 0x9 /* R   */
#define MI_INTERRUPT_2 0xA /* R   */
#define MI_INTERRUPT_3 0xB /* R   */
#define MI_MASK_0      0xC /* R/W */
#define MI_MASK_1      0xD /* R/W */
#define MI_MASK_2      0xE /* R   */
#define MI_MASK_3      0xF /* R   */

namespace MIPS_Interface
{
	export enum class InterruptType : u8 {
		SP = 1 << 0,
		SI = 1 << 1, /* Set when a SI DMA to/from PIF RAM finishes */
		AI = 1 << 2, /* Set when no more samples remain in an audio stream */
		VI = 1 << 3, /* Set when VI_V_CURRENT == VI_V_INTR */
		PI = 1 << 4, /* Set when a PI DMA finishes */
		DP = 1 << 5  /* Set when a full sync completes */
	};


	std::array<u8, 0x10> mem{};


	template<InterruptType interrupt_type>
	void SetInterruptFlag()
	{
		const u8 interrupt_type_mask = static_cast<u8>(interrupt_type);
		mem[MI_INTERRUPT_0] |= interrupt_type_mask;
		if (mem[MI_INTERRUPT_0] & mem[MI_MASK_0] & interrupt_type_mask)
		{
			/* TODO: tell the cpu about interrupt */
		}
	}


	template<InterruptType interrupt_type>
	void ClearInterruptFlag()
	{
		const u8 interrupt_type_mask = static_cast<u8>(interrupt_type);
		mem[MI_INTERRUPT_0] &= ~interrupt_type_mask;
	}


	template<InterruptType interrupt_type>
	void SetInterruptMask()
	{
		const u8 interrupt_mask = static_cast<u8>(interrupt_type);
		mem[MI_MASK_0] |= interrupt_mask;
		if (mem[MI_INTERRUPT_0] & mem[MI_MASK_0] & interrupt_mask)
		{
			/* TODO: tell the cpu about interrupt */
		}
	}


	template<InterruptType interrupt_type>
	void ClearInterruptMask()
	{
		const u8 interrupt_mask = static_cast<u8>(interrupt_type);
		mem[MI_INTERRUPT_0] &= ~interrupt_mask;
	}


	void WriteToMiMode0(const u8 data)
	{
		mem[MI_MODE_0] = data;
		/* todo */
	}


	void WriteToMiMode1(const u8 data)
	{
		mem[MI_MODE_1] = mem[MI_MODE_1] & 0xC0 | data & 0x3F;
		/* todo */
	}


	void WriteToMask0(const u8 data)
	{
		/* TODO: unclear what would happen if two adjacent bits would be set */
		if (data & 0x01)
			ClearInterruptMask<InterruptType::SP>();
		else if (data & 0x02)
			SetInterruptMask<InterruptType::SP>();

		if (data & 0x04)
			ClearInterruptMask<InterruptType::SI>();
		else if (data & 0x08)
			SetInterruptMask<InterruptType::SI>();

		if (data & 0x10)
			ClearInterruptMask<InterruptType::AI>();
		else if (data & 0x20)
			SetInterruptMask<InterruptType::AI>();

		if (data & 0x40)
			ClearInterruptMask<InterruptType::VI>();
		else if (data & 0x80)
			SetInterruptMask<InterruptType::VI>();
	}


	void WriteToMask1(const u8 data)
	{
		if (data & 0x01)
			ClearInterruptMask<InterruptType::PI>();
		else if (data & 0x02)
			SetInterruptMask<InterruptType::PI>();

		if (data & 0x04)
			ClearInterruptMask<InterruptType::DP>();
		else if (data & 0x08)
			SetInterruptMask<InterruptType::DP>();
	}


	export
	{
		void Initialize()
		{
			mem.fill(0);
			constexpr static u8 rsp_version = 0x02; /* https://n64brew.dev/wiki/MIPS_Interface */
			constexpr static u8 rdp_version = 0x02;
			constexpr static u8 rac_version = 0x01;
			constexpr static u8 io_version  = 0x02;
			mem[MI_VERSION_0] = io_version;
			mem[MI_VERSION_1] = rac_version;
			mem[MI_VERSION_2] = rdp_version;
			mem[MI_VERSION_3] = rsp_version; /* TODO: endianness? */
		}


		template<std::size_t number_of_bytes>
		auto Read(const u32 addr)
		{
			return MemoryUtils::GenericRead<number_of_bytes>(&mem[addr & 0xF]);
		}


		template<std::size_t number_of_bytes>
		void Write(const u32 addr, const auto data)
		{
			switch (addr & 0xF)
			{
			case MI_MODE_0:
				WriteToMiMode0(u8(data));
				if constexpr (number_of_bytes > 1)
					WriteToMiMode1(u8(data >> 8));
				break;

			case MI_MODE_1:
				WriteToMiMode1(u8(data));
				break;

			case MI_VERSION_1:
				if constexpr (number_of_bytes == 8)
					WriteToMask0(u8(data >> 56));
				break;

			case MI_VERSION_2:
				if constexpr (number_of_bytes >= 7)
					WriteToMask0(u8(data >> 48));
				if constexpr (number_of_bytes == 8)
					WriteToMask1(u8(data >> 56));
				break;

			case MI_VERSION_3:
				if constexpr (number_of_bytes >= 6)
					WriteToMask0(u8(data >> 40));
				if constexpr (number_of_bytes >= 7)
					WriteToMask1(u8(data >> 48));
				break;

			case MI_INTERRUPT_0:
				if constexpr (number_of_bytes >= 5)
					WriteToMask0(u8(data >> 32));
				if constexpr (number_of_bytes >= 6)
					WriteToMask1(u8(data >> 40));
				break;

			case MI_INTERRUPT_1:
				if constexpr (number_of_bytes >= 4)
					WriteToMask0(u8(data >> 24));
				if constexpr (number_of_bytes >= 5)
					WriteToMask1(u8(data >> 32));
				break;

			case MI_INTERRUPT_2:
				if constexpr (number_of_bytes >= 3)
					WriteToMask0(u8(data >> 16));
				if constexpr (number_of_bytes >= 4)
					WriteToMask1(u8(data >> 24));
				break;

			case MI_INTERRUPT_3:
				if constexpr (number_of_bytes >= 2)
					WriteToMask0(u8(data >> 8));
				if constexpr (number_of_bytes >= 3)
					WriteToMask1(u8(data >> 16));
				break;

			case MI_MASK_0:
				WriteToMask0(u8(data));
				if constexpr (number_of_bytes >= 2)
					WriteToMask1(u8(data >> 8));
				break;

			case MI_MASK_1:
				WriteToMask1(u8(data));
				break;

			default:
				break;
			}
		}
	}
}