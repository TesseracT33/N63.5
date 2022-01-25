export module VR4300:COP0;

import NumericalTypes;

import <bit>;
import <cassert>;
import <cstring>;

namespace VR4300
{
	enum class CP0_Instruction
	{
		/* Move instructions */
		MTC0, MFC0, DMTC0, DMFC0,

		/* TLB instructions */
		TLBP, TLBR, TLBWI, TLBWR,

		/* Misc. instructions */
		ERET, CACHE
	};

	/* COP0 instructions */
	template<CP0_Instruction instr> void CP0_Move(const u32 instr_code);
	void TLBR();
	void TLBWI();
	void TLBWR();
	void TLBP();
	void ERET();
	void CACHE(const u32 instr_code);

	void DecrementRandomRegister()
	{
		/* TODO */
	}
}