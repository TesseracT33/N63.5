export module VR4300:COP0;

import NumericalTypes;

import <array>;
import <bit>;
import <cassert>;
import <cstring>;

namespace VR4300
{
	enum class COP0Instruction
	{
		/* Move instructions */
		MTC0, MFC0, DMTC0, DMFC0,

		/* TLB instructions */
		TLBP, TLBR, TLBWI, TLBWR,

		/* Misc. instructions */
		ERET, CACHE
	};

	/* COP0 instructions */
	template<COP0Instruction> void COP0Move(u32 instr_code);
	void TLBR();
	void TLBWI();
	void TLBWR();
	void TLBP();
	void ERET();
	void CACHE(u32 instr_code);

	std::array cop0_reg_str_repr = {
		"INDEX", "RANDOM", "ENTRY_LO_0", "ENTRY_LO_1", "CONTEXT", "PAGE_MASK", "WIRED", "COP0_7", "BAD_V_ADDR",
		"COUNT", "ENTRY_HI", "COMPARE", "STATUS", "CAUSE", "EPC", "PR_ID", "CONFIG", "LL_ADDR", "WATCH_LO",
		"WATCH_HI", "X_CONTEXT", "COP0_21", "COP0_22", "COP0_23", "COP0_24", "COP0_25", "PARITY_ERROR",
		"COP0_27", "TAG_LO", "TAG_HI", "ERROR_EPC"
	};
	static_assert(cop0_reg_str_repr.size() == 31);
}