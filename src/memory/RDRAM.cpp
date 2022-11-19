module RDRAM;

namespace RDRAM
{
	size_t GetNumberOfBytesUntilMemoryEnd(u32 addr)
	{
		/* TODO handle mirroring (for DMA) */
		return sizeof(rdram) - (addr & (sizeof(rdram) - 1));
	}


	u8* GetPointerToMemory(u32 addr)
	{
		return rdram + (addr & (sizeof(rdram) - 1));
	}


	size_t GetSize()
	{
		return sizeof(rdram);
	}


	void Initialize()
	{
		std::memset(rdram, 0, sizeof(rdram));
		std::memset(&reg, 0, sizeof(reg));
		/* values taken from Peter Lemon RDRAMTest */
		reg.device_type = 0xB419'0010;
		reg.delay = 0x2B3B'1A0B;
		reg.ras_interval = 0x101C'0A04;
	}


	/* 0 - $7F'FFFF */
	template<std::signed_integral Int>
	Int Read(u32 addr)
	{ /* CPU precondition: addr is always aligned */
		Int ret;
		std::memcpy(&ret, rdram + (addr & (sizeof(rdram) - 1)), sizeof(Int));
		return std::byteswap(ret);
	}


	/* $03F0'0000 - $03FF'FFFF */
	s32 ReadReg(u32 addr)
	{
		static_assert(sizeof(reg) >> 2 == 0x10);
		u32 offset = addr >> 2 & 0xF;
		s32 ret;
		std::memcpy(&ret, (s32*)(&reg) + offset, 4);
		return ret;
	}


	u64 RdpReadCommandByteswapped(u32 addr)
	{
		/* addr may be misaligned */
		u64 command;
		for (int i = 0; i < 8; ++i) {
			*((u8*)(&command) + i) = rdram[(addr + 7 - i) & (sizeof(rdram) - 1)];
		}
		return command;
	}


	u32 RdpReadCommand(u32 addr)
	{
		/* addr may be misaligned */
		u64 command;
		for (int i = 0; i < 8; ++i) {
			*((u8*)(&command) + i) = rdram[(addr + i) & (sizeof(rdram) - 1)];
		}
		return command;
	}


	/* 0 - $7F'FFFF */
	template<size_t access_size, typename... MaskT>
	void Write(u32 addr, s64 data, MaskT... mask)
	{ /* Precondition: phys_addr is aligned to access_size if sizeof...(mask) == 0 */
		static_assert(std::has_single_bit(access_size) && access_size <= 8);
		static_assert(sizeof...(mask) <= 1);
		auto to_write = [&] {
			if constexpr (access_size == 1) return u8(data);
			if constexpr (access_size == 2) return std::byteswap(u16(data));
			if constexpr (access_size == 4) return std::byteswap(u32(data));
			if constexpr (access_size == 8) return std::byteswap(data);
		}();
		static constexpr bool apply_mask = sizeof...(mask) == 1;
		if constexpr (apply_mask) {
			addr &= ~(access_size - 1);
		}
		u8* ram = rdram + (addr & (sizeof(rdram) - 1));
		if constexpr (apply_mask) {
			u64 existing;
			std::memcpy(&existing, ram, access_size);
			to_write |= existing & (..., mask);
		}
		std::memcpy(ram, &to_write, access_size);
	}


	/* $03F0'0000 - $03FF'FFFF */
	void WriteReg(u32 addr, s32 data)
	{
		/* TODO */
	}


	template s8 Read<s8>(u32);
	template s16 Read<s16>(u32);
	template s32 Read<s32>(u32);
	template s64 Read<s64>(u32);
	template void Write<1>(u32, s64);
	template void Write<2>(u32, s64);
	template void Write<4>(u32, s64);
	template void Write<8>(u32, s64);
	template void Write<4, s64>(u32, s64, s64);
	template void Write<8, s64>(u32, s64, s64);
}