module Memory;

import AI;
import Cart;
import BuildOptions;
import Logging;
import MI;
import PI;
import PIF;
import RDRAM;
import RI;
import RDP;
import RSP;
import SI;
import VI;

namespace Memory
{
#define READ_INTERFACE(io, INT, addr) [&] {                                    \
	if constexpr (sizeof(INT) == 4) {                                          \
		return io::ReadReg(addr);                                              \
	}                                                                          \
	else {                                                                     \
		Log(std::format(                                                       \
			"Attempted to read IO region at address ${:08X} for sized int {}", \
			addr, sizeof(INT)));                                               \
		return INT{};                                                          \
	}}()


#define WRITE_INTERFACE(io, access_size, addr, data)                            \
	if constexpr (access_size == 4) {                                           \
		io::WriteReg(addr, data);                                               \
	}                                                                           \
	else {                                                                      \
		Log(std::format(                                                        \
			"Attempted to write IO region at address ${:08X} for sized int {}", \
			addr, access_size));                                                \
	}


	template<std::signed_integral Int>
	Int Read(u32 addr)
	{ /* Precondition: 'addr' is aligned according to the size of 'Int' */
		if (addr <= 0x03EF'FFFF) {
			return RDRAM::Read<Int>(addr);
		}
		if (addr <= 0x048F'FFFF) {
			switch ((addr >> 20) - 0x3F) {
			case 0: /* $03F0'0000 - $03FF'FFFF */
				return READ_INTERFACE(RDRAM, Int, addr);

			case 1: /* $0400'0000 - $040F'FFFF */
				return RSP::ReadMemoryCpu<Int>(addr);

			case 2: /* $0410'0000 - $041F'FFFF */
				return READ_INTERFACE(RDP, Int, addr);

			case 3: /* $0420'0000 - $042F'FFFF */
				Log(std::format("Unexpected cpu read to address ${:08X}", addr));
				return Int{};

			case 4: /* $0430'0000 - $043F'FFFF */
				return READ_INTERFACE(MI, Int, addr);

			case 5: /* $0440'0000 - $044F'FFFF */
				return READ_INTERFACE(VI, Int, addr);

			case 6: /* $0450'0000 - $045F'FFFF */
				return READ_INTERFACE(AI, Int, addr);

			case 7: /* $0460'0000 - $046F'FFFF */
				return READ_INTERFACE(PI, Int, addr);

			case 8: /* $0470'0000 - $047F'FFFF */
				return READ_INTERFACE(RI, Int, addr);

			case 9: /* $0480'0000 - $048F'FFFF */
				return READ_INTERFACE(SI, Int, addr);

			default:
				std::unreachable();
			}
		}
		if (addr >= 0x800'0000 && addr <= 0xFFF'FFFF) {
			return Cart::ReadSram<Int>(addr);
		}
		if (addr >= 0x1000'0000 && addr <= 0x1FBF'FFFF) {
			return Cart::ReadRom<Int>(addr);
		}
		if ((addr & 0xFFFF'F800) == 0x1FC0'0000) { /* $1FC0'0000 - $1FC0'07FF */
			return PIF::ReadMemory<Int>(addr);
		}
		Log(std::format("Unexpected cpu read to address ${:08X}", addr));
		return Int{};
	}


	template<size_t access_size, typename... MaskT>
	void Write(u32 addr, s64 data, MaskT... mask)
	{
		static_assert(std::has_single_bit(access_size) && access_size <= 8);
		static_assert(sizeof...(mask) <= 1);
		if (addr <= 0x03EF'FFFF) {
			RDRAM::Write<access_size>(addr, data, mask...);
		}
		else if (addr <= 0x048F'FFFF) {
			switch ((addr >> 20) - 0x3F) {
			case 0: /* $03F0'0000 - $03FF'FFFF */
				WRITE_INTERFACE(RDRAM, access_size, addr, data); break;

			case 1: /* $0400'0000 - $040F'FFFF */
				RSP::WriteMemoryCpu<access_size>(addr, data); break;

			case 2: /* $0410'0000 - $041F'FFFF */
				WRITE_INTERFACE(RDP, access_size, addr, data); break;

			case 3: /* $0420'0000 - $042F'FFFF */
				Log(std::format("Unexpected cpu write to address ${:08X}", addr)); break;

			case 4: /* $0430'0000 - $043F'FFFF */
				WRITE_INTERFACE(MI, access_size, addr, data); break;

			case 5: /* $0440'0000 - $044F'FFFF */
				WRITE_INTERFACE(VI, access_size, addr, data); break;

			case 6: /* $0450'0000 - $045F'FFFF */
				WRITE_INTERFACE(AI, access_size, addr, data); break;

			case 7: /* $0460'0000 - $046F'FFFF */
				WRITE_INTERFACE(PI, access_size, addr, data); break;

			case 8: /* $0470'0000 - $047F'FFFF */
				WRITE_INTERFACE(RI, access_size, addr, data); break;

			case 9: /* $0480'0000 - $048F'FFFF */
				WRITE_INTERFACE(SI, access_size, addr, data); break;

			default:
				std::unreachable();
			}
		}
		else if (addr >= 0x800'0000 && addr <= 0xFFF'FFFF) {
			Cart::WriteSram<access_size>(addr, data);
		}
		else if (addr >= 0x1000'0000 && addr <= 0x1FBF'FFFF) {
			Cart::WriteRom<access_size>(addr, data);
		}
		else if ((addr & 0xFFFF'F800) == 0x1FC0'0000) { /* $1FC0'0000 - $1FC0'07FF */
			PIF::WriteMemory<access_size>(addr, data);
		}
		else {
			Log(std::format("Unexpected cpu write to address ${:08X}", addr));
		}
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