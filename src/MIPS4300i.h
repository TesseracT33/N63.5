#pragma once

#include "MMU.h"
#include "NumericalTypes.h"

class MIPS4300i
{
public:
	MMU* mmu;

	void Run(const int cycles);

private:
	enum class Instr
	{
		/* Load instructions */
		LB, LBU, LH, LHU, LW, LWU, LWL, LWR, LD, LDL, LDR, LL, LLD,

		/* Store instructions */
		SB, SH, SW, SWL, SWR, SC, SDC, SD, SDL, SDR,

		/* ALU immediate instructions */
		ADDI, ADDIU, SLTI, SLTIU, ANDI, ORI, XORI, LUI, DADDI, DADDIU,

		/* ALU three-operand instructions */
		ADD, ADDU, SUB, SUBU, SLT, SLTU, AND, OR, XOR, NOR, DADD, DADDU, DSUB, DSUBU,

		/* ALU shift instructions */
		SLL, SRL, SRA, SLLV, SRLV, SRAV, DSLL, DSRL, DSRA, DSLLV, DSRLV, DSRAV, DSLL32, DSRL32, DSRA32,

		/* ALU mul/div instructions */
		MULT, MULTU, DIV, DIVU, MFHI, MFLO, MTHI, MTLO, DMULT, DMULTU, DDIV, DDIVU,

		/* Jump instructions */
		J, JAL, JR, JALR,

		/* Branch instructions */
		BEQ, BNE, BLEZ, BGTZ, BLTZ, BGEZ, BLTZAL, BGEZAL, BEQL, BNEL, BLEZL, BGTZL, BLTZL, BGEZL, BLTZALL, BGEZALL,

		/* Trap instructions */
		TGE, TGEU, TLT, TLTU, TEQ, TNE, TGEI, TGEIU, TLTI, TLTIU, TEQI, TNEI,

		/* Special instructions */
		SYNC, SYSCALL, BREAK,

		/* Coprocessor instructions */
		LWCz, SWCz, MTCz, MFCz, CTCz, CFCz, COPz
	};

	enum Endianness {
		Little, Big
	} endianness;

	u64 GPR[32];
	f64 FPR[32];

	f32 FCR0, FCR31;

	u64 HI, LO;

	bool LL;

	u64 PC;

	bool jump_next_instruction = false;
	u64 addr_to_jump_to;

	void ExecuteInstruction();

	u32 instr_code;

	template<Instr instr> void Load(const u32 instr_code);
	template<Instr instr> void Store(const u32 instr_code);
	template<Instr instr> void ALU_Immediate(const u32 instr_code);
	template<Instr instr> void ALU_ThreeOperand(const u32 instr_code);
	template<Instr instr> void ALU_Shift(const u32 instr_code);
	template<Instr instr> void ALU_MulDiv(const u32 instr_code);
	template<Instr instr> void Branch(const u32 instr_code);
	template<Instr instr> void Trap_ThreeOperand(const u32 instr_code);
	template<Instr instr> void Trap_Immediate(const u32 instr_code);

	void MFHI(const u32 instr_code);
	void MFLO(const u32 instr_code);
	void MTHI(const u32 instr_code);
	void MTLO(const u32 instr_code);

	void J(const u32 instr_code);
	void JAL(const u32 instr_code);
	void JR(const u32 instr_code);
	void JALR(const u32 instr_code);

	void SYNC(const u32 instr_code);
	void SYSCALL(const u32 instr_code);
	void BREAK(const u32 instr_code);

	void AddressErrorException()
	{

	}

	void IntegerOverflowException()
	{

	}

	void TrapException()
	{

	}
};