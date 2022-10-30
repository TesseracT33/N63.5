module RDRAM;

import Memory;

namespace RDRAM
{
	size_t GetNumberOfBytesUntilMemoryEnd(u32 start_addr)
	{
		start_addr &= sizeof(rdram) - 1;
		return std::max(size_t(0), sizeof(rdram) - start_addr);
	}


	u8* GetPointerToMemory(u32 addr)
	{
		addr &= sizeof(rdram) - 1;
		return rdram + addr;
	}


	size_t GetSize()
	{
		return sizeof(rdram);
	}


	void Initialize()
	{
		std::memset(&reg, 0, sizeof(reg));
		/* values taken from Peter Lemon RDRAMTest */
		reg.device_type = 0xB4190010;
		reg.delay = 0x2B3B1A0B;
		reg.ras_interval = 0x101C0A04;
	}


	/* 0 - $7F'FFFF */
	template<std::signed_integral Int>
	Int Read(u32 addr)
	{ /* CPU precondition: addr is always aligned */
		static_assert(sizeof(rdram) >= 0x80'0000);
		Int ret;
		std::memcpy(&ret, rdram + addr, sizeof(Int));
		return std::byteswap(ret);
	}


	/* $03F0'0000 - $03FF'FFFF */
	s32 ReadReg(u32 addr)
	{
		if (addr <= 0x3F0'0024) {
			auto reg_index = addr >> 2 & 0xF;
			s32 ret;
			std::memcpy(&ret, (s32*)(&reg) + reg_index, 4);
			return ret;
		}
		else {
			return 0;
		}
	}


	u64 RspReadCommandByteswapped(u32 addr)
	{
		/* addr may be misaligned */
		u64 command;
		for (int i = 0; i < 8; ++i) {
			*((u8*)(&command) + i) = rdram[(addr + 7 - i) & (sizeof(rdram) - 1)];
		}
		return command;
	}


	/* 0 - $7F'FFFF */
	template<size_t num_bytes>
	void Write(u32 addr, std::signed_integral auto data)
	{ /* CPU precondition: addr + number_of_bytes does not go beyond the next alignment boundary */
		data = std::byteswap(data);
		std::memcpy(rdram + addr, &data, num_bytes);
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
	template void Write<1>(u32, s8);
	template void Write<1>(u32, s16);
	template void Write<1>(u32, s32);
	template void Write<1>(u32, s64);
	template void Write<2>(u32, s16);
	template void Write<2>(u32, s32);
	template void Write<2>(u32, s64);
	template void Write<3>(u32, s32);
	template void Write<3>(u32, s64);
	template void Write<4>(u32, s32);
	template void Write<4>(u32, s64);
	template void Write<5>(u32, s64);
	template void Write<6>(u32, s64);
	template void Write<7>(u32, s64);
	template void Write<8>(u32, s64);
}