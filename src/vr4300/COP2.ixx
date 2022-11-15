export module VR4300:COP2;

import Util;

import <format>;

namespace VR4300
{
	enum class Cop2Instruction {
		CFC2, CTC2, MFC2, MTC2, DCFC2, DCTC2, DMFC2, DMTC2
	};

	template<Cop2Instruction>
	void Cop2Move(u32 instr_code);

	void InitializeCop22();

	u64 cop2_latch;
}