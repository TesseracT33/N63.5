module MIPS4300i;

#include <cassert>
#include <cmath>

namespace MIPS4300i
{
	template<FPU_Instr instr>
	void FPU_Load(const u32 instr_code)
	{
		const s16 offset = instr_code & 0xFFFF;
		const u8 ft = instr_code >> 16 & 0x1F;
		const u8 base = instr_code >> 21 & 0x1F;
		const u64 address = GPR[base] + offset;

		if constexpr (instr == FPU_Instr::LWC1)
		{
			FGR[ft] = MMU::cpu_read_mem<u32>(address);
		}
		else if constexpr (instr == FPU_Instr::LDC1)
		{
			FGR[ft] = MMU::cpu_read_mem<u64>(address);
		}
		else
		{
			static_assert(false);
		}
	}


	template<FPU_Instr instr>
	void FPU_Store(const u32 instr_code)
	{
		const s16 offset = instr_code & 0xFFFF;
		const u8 ft = instr_code >> 16 & 0x1F;
		const u8 base = instr_code >> 21 & 0x1F;
		const u64 address = GPR[base] + offset;

		if constexpr (instr == FPU_Instr::SWC1)
		{
			MMU::cpu_write_mem<u32>(address, FGR[ft]);
		}
		else if constexpr (instr == FPU_Instr::SDC1)
		{
			MMU::cpu_write_mem<u64>(address, FGR[ft]);
		}
		else
		{
			static_assert(false);
		}
	}


	template<FPU_Instr instr>
	void FPU_Move(const u32 instr_code)
	{
		const u8 fs = instr_code >> 11 & 0x1F;
		const u8 rt = instr_code >> 16 & 0x1F;

		if constexpr (instr == FPU_Instr::MTC1)
		{
			FGR[fs] = GPR[rt];
		}
		else if constexpr (instr == FPU_Instr::MFC1)
		{
			GPR[rt] = FGR[fs];
		}
		else if constexpr (instr == FPU_Instr::CTC1)
		{
			control[fs] = GPR[rt];
		}
		else if constexpr (instr == FPU_Instr::CFC1)
		{
			GPR[rt] = control[fs];
		}
		else if constexpr (instr == FPU_Instr::DMTC1)
		{
			FGR[fs] = GPR[rt];
		}
		else if constexpr (instr == FPU_Instr::DMFC1)
		{
			GPR[rt] = FGR[fs];
		}
		else
		{
			static_assert(false);
		}
	}


	template<FPU_Instr instr>
	void FPU_Convert(const u32 instr_code)
	{
		const u8 fd = instr_code >> 6 & 0x1F;
		const u8 fs = instr_code >> 11 & 0x1F;
		const u8 fmt = instr_code >> 21 & 0x1F;

		const u64 source = 0;
		//const auto source = [&] {
		//	switch (fmt)
		//	{
		//	case 16: return f32(FGR[fs]);
		//	case 17: return f64(FGR[fs]);
		//	case 20: return u32(FGR[fs]);
		//	case 21: return u64(FGR[fs]);
		//	default: assert(false);
		//	}
		//}();

		if constexpr (instr == FPU_Instr::CVT_S)
		{
			FGR[fd] = f32(source);
		}
		else if constexpr (instr == FPU_Instr::CVT_D)
		{
			FGR[fd] = f64(source);
		}
		else if constexpr (instr == FPU_Instr::CVT_L)
		{
			FGR[fd] = u64(source);
		}
		else if constexpr (instr == FPU_Instr::CVT_W)
		{
			FGR[fd] = u32(source);
		}
		else if constexpr (instr == FPU_Instr::ROUND_L)
		{
			FGR[fd] = std::round(source);
		}
		else if constexpr (instr == FPU_Instr::ROUND_W)
		{
			FGR[fd] = std::round(source);
		}
		else if constexpr (instr == FPU_Instr::TRUNC_L)
		{
			FGR[fd] = u32(source);
		}
		else if constexpr (instr == FPU_Instr::TRUNC_W)
		{
			FGR[fd] = u32(source);
		}
		else if constexpr (instr == FPU_Instr::CEIL_L)
		{
			FGR[fd] = u32(source);
		}
		else if constexpr (instr == FPU_Instr::CEIL_W)
		{
			FGR[fd] = u32(source);
		}
		else if constexpr (instr == FPU_Instr::FLOOR_L)
		{
			FGR[fd] = u32(source);
		}
		else if constexpr (instr == FPU_Instr::FLOOR_W)
		{
			FGR[fd] = u32(source);
		}
		else
		{
			static_assert(false);
		}
	}

	template void FPU_Load<FPU_Instr::LWC1>(const u32 instr_code);
	template void FPU_Load<FPU_Instr::LDC1>(const u32 instr_code);

	template void FPU_Store<FPU_Instr::SWC1>(const u32 instr_code);
	template void FPU_Store<FPU_Instr::SDC1>(const u32 instr_code);

	template void FPU_Move<FPU_Instr::MTC1>(const u32 instr_code);
	template void FPU_Move<FPU_Instr::MFC1>(const u32 instr_code);
	template void FPU_Move<FPU_Instr::CTC1>(const u32 instr_code);
	template void FPU_Move<FPU_Instr::CFC1>(const u32 instr_code);
	template void FPU_Move<FPU_Instr::DMTC1>(const u32 instr_code);
	template void FPU_Move<FPU_Instr::DMFC1>(const u32 instr_code);

	template void FPU_Convert<FPU_Instr::CVT_S>(const u32 instr_code);
	template void FPU_Convert<FPU_Instr::CVT_D>(const u32 instr_code);
	template void FPU_Convert<FPU_Instr::CVT_L>(const u32 instr_code);
	template void FPU_Convert<FPU_Instr::CVT_W>(const u32 instr_code);
	template void FPU_Convert<FPU_Instr::ROUND_L>(const u32 instr_code);
	template void FPU_Convert<FPU_Instr::ROUND_W>(const u32 instr_code);
	template void FPU_Convert<FPU_Instr::TRUNC_L>(const u32 instr_code);
	template void FPU_Convert<FPU_Instr::TRUNC_W>(const u32 instr_code);
	template void FPU_Convert<FPU_Instr::CEIL_L>(const u32 instr_code);
	template void FPU_Convert<FPU_Instr::CEIL_W>(const u32 instr_code);
	template void FPU_Convert<FPU_Instr::FLOOR_L>(const u32 instr_code);
	template void FPU_Convert<FPU_Instr::FLOOR_W>(const u32 instr_code);
}