export module RSP:ScalarUnit;

import BuildOptions;
import Util;

import <array>;
import <format>;

namespace RSP
{
	enum class ScalarInstruction {
		/* Load instructions */
		LB, LBU, LH, LHU, LW, LWU, LL,

		/* Store instructions */
		SB, SH, SW, SC,

		/* ALU immediate instructions */
		ADDI, ADDIU, SLTI, SLTIU, ANDI, ORI, XORI, LUI,

		/* ALU three-operand instructions */
		ADD, ADDU, SUB, SUBU, SLT, SLTU, AND, OR, XOR, NOR,

		/* ALU shift instructions */
		SLL, SRL, SRA, SLLV, SRLV, SRAV,

		/* Jump instructions */
		J, JAL, JR, JALR,

		/* Branch instructions */
		BEQ, BNE, BLEZ, BGTZ, BLTZ, BGEZ, BLTZAL, BGEZAL,

		/* Special instructions */
		BREAK,

		/* COP0 Move instructions */
		MFC0, MTC0
	};

	/* Main processor instructions */
	template<ScalarInstruction> void ScalarLoad(u32 instr_code);
	template<ScalarInstruction> void ScalarStore(u32 instr_code);
	template<ScalarInstruction> void AluImmediate(u32 instr_code);
	template<ScalarInstruction> void AluThreeOperand(u32 instr_code);
	template<ScalarInstruction> void Shift(u32 instr_code);
	template<ScalarInstruction> void Jump(u32 instr_code);
	template<ScalarInstruction> void Branch(u32 instr_code);
	template<ScalarInstruction> void Move(u32 instr_code);
	void Break();

	class GPR /* scalar general-purpose registers */
	{
		std::array<s32, 32> gpr{};
	public:
		s32 Get(size_t index) const
		{
			return gpr[index];
		}
		void Set(size_t index, s32 data)
		{
			/* gpr[0] is hardwired to 0. Prefer setting it to zero every time over a branch checking if 'index' is zero. */
			gpr[index] = data;
			gpr[0] = 0;
		}
		s32& operator[](size_t index) 
		{
			return gpr[index];
		}
	} gpr{};

	bool ll_bit; /* Read from / written to by load linked and store conditional instructions. */
}