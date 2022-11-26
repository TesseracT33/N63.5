module VR4300:COP2;

import :COP0;
import :CPU;
import :Exceptions;
import :Operation;

import BuildOptions;

namespace VR4300
{
	void InitializeCOP2()
	{
		cop2_latch = 0;
	}


	template<Cop2Instruction instr>
	void Cop2Move(u32 rt)
	{
		AdvancePipeline(1);

		if constexpr (log_cpu_instructions) {
			current_instr_log_output = std::format("{} {}", current_instr_name, rt);
		}

		if (!cop0.status.cu2) {
			SignalCoprocessorUnusableException(2);
			return;
		}

		using enum Cop2Instruction;

		     if constexpr (OneOf(instr, CFC2, MFC2))        gpr.Set(rt, s32(cop2_latch));
		else if constexpr (instr == DMFC2)                  gpr.Set(rt, cop2_latch);
		else if constexpr (OneOf(instr, CTC2, DMTC2, MTC2)) cop2_latch = gpr[rt];
		else if constexpr (OneOf(instr, DCFC2, DCTC2))      SignalException<Exception::ReservedInstructionCop2>();
		else static_assert(AlwaysFalse<instr>);
	}


	template void Cop2Move<Cop2Instruction::CFC2>(u32);
	template void Cop2Move<Cop2Instruction::CTC2>(u32);
	template void Cop2Move<Cop2Instruction::MFC2>(u32);
	template void Cop2Move<Cop2Instruction::MTC2>(u32);
	template void Cop2Move<Cop2Instruction::DCFC2>(u32);
	template void Cop2Move<Cop2Instruction::DCTC2>(u32);
	template void Cop2Move<Cop2Instruction::DMFC2>(u32);
	template void Cop2Move<Cop2Instruction::DMTC2>(u32);
}