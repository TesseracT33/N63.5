export module MIPS4300i;

import MMU;
import NumericalTypes;

namespace MIPS4300i
{
	export
	{
		void Reset();
		void Run(const int cycles);
	}

	enum class Instr
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

	enum class FPU_Instr
	{
		LWC1, SWC1, LDC1, SDC1, MTC1, MFC1, CTC1, CFC1, DMTC1, DMFC1,

		CVT_S, CVT_D, CVT_L, CVT_W, ROUND_L, ROUND_W, TRUNC_L, TRUNC_W, CEIL_L, CEIL_W, FLOOR_L, FLOOR_W,

		ADD, SUB, MUL, DIV, ABS, MOV, NEG, SQRT
	};

	u64 PC;

	u64 GPR[32]{};
	u64 FGR[64]{};
	u64 control[32];

	u64 HI, LO;

	bool LL;

	bool jump_next_instruction;
	u64 addr_to_jump_to;

	/* Main processor instructions */
	template<Instr instr> void Load(const u32 instr_code);
	template<Instr instr> void Store(const u32 instr_code);
	template<Instr instr> void ALU_Immediate(const u32 instr_code);
	template<Instr instr> void ALU_ThreeOperand(const u32 instr_code);
	template<Instr instr> void ALU_Shift(const u32 instr_code);
	template<Instr instr> void ALU_MulDiv(const u32 instr_code);
	template<Instr instr> void Jump(const u32 instr_code);
	template<Instr instr> void Branch(const u32 instr_code);
	template<Instr instr> void Trap_ThreeOperand(const u32 instr_code);
	template<Instr instr> void Trap_Immediate(const u32 instr_code);
	void MFHI(const u32 instr_code);
	void MFLO(const u32 instr_code);
	void MTHI(const u32 instr_code);
	void MTLO(const u32 instr_code);
	void SYNC(const u32 instr_code);
	void SYSCALL(const u32 instr_code);
	void BREAK(const u32 instr_code);
	void CACHE(const u32 instr_code);

	/* COP0 instructions */
	void MTC0(const u32 instr_code);
	void MFC0(const u32 instr_code);
	void DMTC0(const u32 instr_code);
	void DMFC0(const u32 instr_code);
	void TLBR();
	void TLBWI();
	void TLBWR();
	void TLBP();
	void ERET();

	/* COP1/FPU instructions */
	template<FPU_Instr instr> void FPU_Load(const u32 instr_code);
	template<FPU_Instr instr> void FPU_Store(const u32 instr_code);
	template<FPU_Instr instr> void FPU_Move(const u32 instr_code);
	template<FPU_Instr instr> void FPU_Convert(const u32 instr_code);

	void AddressErrorException();
	void IntegerOverflowException();
	void TrapException();

	void ExecuteInstruction();
}