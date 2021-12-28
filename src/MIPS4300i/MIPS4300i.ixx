export module MIPS4300i;

import <bit>;
import <cassert>;
import <cfenv>;
import <cmath>;
import <concepts>;
import <limits>;
import <type_traits>;

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

		ADD, SUB, MUL, DIV, ABS, MOV, NEG, SQRT,

		BC1T, BC1F, BC1TL, BC1FL
	};

	template<typename T>
	concept FPU_NumericType =
		std::is_same<f32, typename std::remove_cv<T>::type>::value ||
		std::is_same<f64, typename std::remove_cv<T>::type>::value ||
		std::is_same<s32, typename std::remove_cv<T>::type>::value ||
		std::is_same<s64, typename std::remove_cv<T>::type>::value;

	struct
	{
		unsigned IE  : 1; /* Specifies and indicates global interrupt enable (0: disable interrupts; 1: enable interrupts) */
		unsigned EXL : 1; /* Specifies and indiciates exception level (0: normal; 1: exception) */
		unsigned ERL : 1; /* Specifies and indiciates error level (0: normal; 1: error) */
		unsigned KSU : 2; /* Specifies and indicates mode bits (00: kernel; 01: supervisor; 10: user) */
		unsigned UX  : 1; /* Enables 64-bit addressing and operations in User mode (0: 32-bit; 1: 64-bit) */
		unsigned SX  : 1; /* Enables 64-bit addressing and operations in Supervisor mode (0: 32-bit; 1: 64-bit) */
		unsigned KX  : 1; /* Enables 64-bit addressing in Kernel mode (0: 32-bit; 1: 64-bit) */
		unsigned IM  : 8; /* Interrupt Mask field */
		unsigned DS  : 9; /* Diagnostic Status field */
		unsigned RE  : 1; /* Reverse-Endian bit, enables reverse of system endianness in User mode (0: disabled; 1: reversed) */
		unsigned FR  : 1; /* Enables additional floating-point registers (0: 16 registers; 1: 32 registers) */
		unsigned RP  : 1; /* Enables low-power operation by reducing the internal clock frequency and the system interface clock frequency to one-quarter speed (0: normal; 1: low power mode) */
		unsigned CU  : 4; /* Controls the usability of each of the four coprocessor unit numbers (0: unusable; 1: usable)  */
	} status_reg{};

	struct
	{
		unsigned RM : 2;

		struct
		{
			unsigned I : 1;
			unsigned U : 1;
			unsigned O : 1;
			unsigned Z : 1;
			unsigned V : 1;
		} flags, enables;

		struct
		{
			unsigned I : 1;
			unsigned U : 1;
			unsigned O : 1;
			unsigned Z : 1;
			unsigned V : 1;
			unsigned E : 1;
		} cause;

	private:
		const unsigned padding0 : 5 = 0;

	public:
		unsigned C : 1;
		unsigned FS : 1;

	private:
		const unsigned padding1 : 7 = 0;
	} FCR31{};

	u64 PC;

	u64 GPR[32]{};
	u64 control[32]{};

	struct FloatingPointGeneralPurposeRegisters
	{
		template<typename FPU_NumericType>
		inline FPU_NumericType InterpretAs(const size_t index) const
		{
			if constexpr (sizeof FPU_NumericType < sizeof s64) /* I.e., 32 bits */
				return std::bit_cast<FPU_NumericType, s32>(s32(FGR[index]));
			else
				return std::bit_cast<FPU_NumericType, s64>(FGR[index]);
		}

		template<typename FPU_NumericType>
		inline void Set(const size_t index, const FPU_NumericType data)
		{
			/* TODO */
		}

		s64& operator[](const size_t index)
		{
			return FGR[index];
		}
	private:
		s64 FGR[32]{};
	} FGR;

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
	template<FPU_Instr instr> void FPU_Compute(const u32 instr_code);
	template<FPU_Instr instr> void FPU_Branch(const u32 instr_code);
	void FPU_Compare(const u32 instr_code);

	/* Exceptions */
	void AddressErrorException();
	void DivisionByZeroException();
	void IntegerOverflowException();
	void InexactOperationException();
	void InvalidOperationException();
	void OverflowException();
	void TrapException();
	void UnderflowException();

	void ExecuteInstruction();
}