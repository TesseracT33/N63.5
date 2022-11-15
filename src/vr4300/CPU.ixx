export module VR4300:CPU;

import Util;

import <array>;
import <format>;
import <limits>;
import <string>;
import <type_traits>;
import <utility>;

#ifdef _MSC_VER
import <intrin.h>;
#endif

namespace VR4300
{
	enum class CpuInstruction {
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
		SYNC, SYSCALL, BREAK, CACHE
	};

	enum class Reg {
		zero, at, v0, v1, a0, a1, a2, a3, t0, t1, t2, t3, t4, t5, t6, t7,
		s0, s1, s2, s3, s4, s5, s6, s7, t8, t9, k0, k1, gp, sp, fp, ra
	};

	void PrepareJump(u64 target_address);

	/* Main processor instructions */
	template<CpuInstruction> void CPULoad(u32 instr_code);
	template<CpuInstruction> void CPUStore(u32 instr_code);
	template<CpuInstruction> void ALUImmediate(u32 instr_code);
	template<CpuInstruction> void ALUThreeOperand(u32 instr_code);
	template<CpuInstruction> void ALUShift(u32 instr_code);
	template<CpuInstruction> void ALUMulDiv(u32 instr_code);
	template<CpuInstruction> void Jump(u32 instr_code);
	template<CpuInstruction> void CPUBranch(u32 instr_code);
	template<CpuInstruction> void TrapThreeOperand(u32 instr_code);
	template<CpuInstruction> void TrapImmediate(u32 instr_code);
	template<CpuInstruction> void CPUMove(u32 instr_code);
	void Sync();
	void Syscall();
	void Break();

	class GPR
	{
		std::array<s64, 32> gpr{};
	public:
		s64 Get(size_t index) const;
		s64 Get(Reg reg) const;
		void Set(size_t index, s64 data);
		void Set(Reg reg, s64 data);
		s64 operator[](size_t index);
	} gpr;
}