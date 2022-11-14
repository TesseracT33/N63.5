module VR4300:COP2;

import :COP0;
import :CPU;
import :Exceptions;
import :Operation;

import DebugOptions;

namespace VR4300
{
	void InitializeCOP2()
	{
		cop2_latch = 0;
	}


	template<COP2Instruction instr>
	void COP2Move(u32 instr_code)
	{
		AdvancePipeline(1);

		auto rt = instr_code >> 16 & 0x1F;

		if constexpr (log_cpu_instructions) {
			current_instr_log_output = std::format("{} {}", current_instr_name, rt);
		}

		if (!cop0.status.cu2) {
			SignalCoprocessorUnusableException(2);
			return;
		}

		using enum COP2Instruction;

		     if constexpr (OneOf(instr, CFC2, MFC2))        gpr.Set(rt, s32(cop2_latch));
		else if constexpr (instr == DMFC2)                  gpr.Set(rt, cop2_latch);
		else if constexpr (OneOf(instr, CTC2, DMTC2, MTC2)) cop2_latch = gpr[rt];
		else if constexpr (OneOf(instr, DCFC2, DCTC2))      SignalException<Exception::ReservedInstructionCop2>();
		else static_assert(AlwaysFalse<instr>);
	}


	template void COP2Move<COP2Instruction::CFC2>(u32);
	template void COP2Move<COP2Instruction::CTC2>(u32);
	template void COP2Move<COP2Instruction::MFC2>(u32);
	template void COP2Move<COP2Instruction::MTC2>(u32);
	template void COP2Move<COP2Instruction::DCFC2>(u32);
	template void COP2Move<COP2Instruction::DCTC2>(u32);
	template void COP2Move<COP2Instruction::DMFC2>(u32);
	template void COP2Move<COP2Instruction::DMTC2>(u32);
}