export module VR4300:CPU;

import NumericalTypes;

import <array>;

#ifdef _MSC_VER
import <intrin.h>;
#endif

namespace VR4300
{
	enum class CPU_Instruction
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

	struct GeneralPurposeRegisters
	{
		inline u64 Get(const size_t index) const { return GPR[index]; }
		inline void Set(const size_t index, const u64 data) { if (index != 0) GPR[index] = data; }
		u64 operator[](const size_t index) { return GPR[index]; } /* returns by value so that assignments have to made through function "Set". */
	private:
		std::array<u64, 32> GPR{};
	} GPR;

	u64 PC;

	u64 HI, LO;

	bool LL_bit;

	bool jump_is_pending = false;
	unsigned instructions_until_jump = 0;
	u64 addr_to_jump_to;

	unsigned p_cycle_counter = 0;

	void PrepareJump(const u64 target_address)
	{
		jump_is_pending = true;
		instructions_until_jump = 1;
		addr_to_jump_to = target_address;
	}

	/* Main processor instructions */
	template<CPU_Instruction instr> void Load(const u32 instr_code);
	template<CPU_Instruction instr> void Store(const u32 instr_code);
	template<CPU_Instruction instr> void ALU_Immediate(const u32 instr_code);
	template<CPU_Instruction instr> void ALU_ThreeOperand(const u32 instr_code);
	template<CPU_Instruction instr> void ALU_Shift(const u32 instr_code);
	template<CPU_Instruction instr> void ALU_MulDiv(const u32 instr_code);
	template<CPU_Instruction instr> void Jump(const u32 instr_code);
	template<CPU_Instruction instr> void Branch(const u32 instr_code);
	template<CPU_Instruction instr> void Trap_ThreeOperand(const u32 instr_code);
	template<CPU_Instruction instr> void Trap_Immediate(const u32 instr_code);
	void MFHI(const u32 instr_code);
	void MFLO(const u32 instr_code);
	void MTHI(const u32 instr_code);
	void MTLO(const u32 instr_code);
	void SYNC(const u32 instr_code);
	void SYSCALL(const u32 instr_code);
	void BREAK(const u32 instr_code);
}