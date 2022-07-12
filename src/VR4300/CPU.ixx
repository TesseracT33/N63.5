export module VR4300:CPU;

import :Operation;

import NumericalTypes;

import <array>;
import <format>;
import <limits>;
import <string>;
import <type_traits>;

#ifdef _MSC_VER
import <intrin.h>;
#endif

namespace VR4300
{
	enum class CPUInstruction
	{
		/* Load instructions */
		LB, LBU, LH, LHU, LW, LWU, LWL, LWR, LD, LDL, LDR, LL, LLD,

		/* Store instructions */
		SB, SH, SW, SWL, SWR, SC, SCD, SD, SDL, SDR,

		/* ALU immediate instructions */
		ADDI, ADDIU, SLTI, SLTIU, ANDI, ORI, XORI, LUI, DADDI, DADDIU,

		/* ALU three-operand instructions */
		ADD, ADDU, SUB, SUBU, SLT, SLTU, AND, OR, XOR, NOR, DADD, DADDU, DSUB, DSUBU,

		/* ALU shift instructions */
		SLL, SRL, SRA, SLLV, SRLV, SRAV, DSLL, DSRL, DSRA, DSLLV, DSRLV, DSRAV, DSLL32, DSRL32, DSRA32,

		/* ALU mul/div instructions */
		MULT, MULTU, DIV, DIVU, DMULT, DMULTU, DDIV, DDIVU,

		/* Move instructions */
		MFHI, MFLO, MTHI, MTLO,

		/* Jump instructions */
		J, JAL, JR, JALR,

		/* Branch instructions */
		BEQ, BNE, BLEZ, BGTZ, BLTZ, BGEZ, BLTZAL, BGEZAL, BEQL, BNEL, BLEZL, BGTZL, BLTZL, BGEZL, BLTZALL, BGEZALL,

		/* Trap instructions */
		TGE, TGEU, TLT, TLTU, TEQ, TNE, TGEI, TGEIU, TLTI, TLTIU, TEQI, TNEI,

		/* Special instructions */
		SYNC, SYSCALL, BREAK
	};

	void PrepareJump(u64 target_address);

	/* Main processor instructions */
	template<CPUInstruction> void CPULoad(u32 instr_code);
	template<CPUInstruction> void CPUStore(u32 instr_code);
	template<CPUInstruction> void ALUImmediate(u32 instr_code);
	template<CPUInstruction> void ALUThreeOperand(u32 instr_code);
	template<CPUInstruction> void ALUShift(u32 instr_code);
	template<CPUInstruction> void ALUMulDiv(u32 instr_code);
	template<CPUInstruction> void Jump(u32 instr_code);
	template<CPUInstruction> void CPUBranch(u32 instr_code);
	template<CPUInstruction> void TrapThreeOperand(u32 instr_code);
	template<CPUInstruction> void TrapImmediate(u32 instr_code);
	template<CPUInstruction> void CPUMove(u32 instr_code);
	void Sync();
	void Syscall();
	void Break();

	bool jump_is_pending = false;
	bool last_instr_was_load = false;
	uint instructions_until_jump = 0;
	u64 addr_to_jump_to;
	u8 last_load_target_reg;
}