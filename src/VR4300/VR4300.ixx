export module VR4300;

import <array>;
import <bit>;
import <cassert>;
import <cfenv>;
import <cmath>;
import <concepts>;
import <limits>;
import <type_traits>;

#ifdef _MSC_VER
import <intrin.h>;
#endif

import MMU;
import NumericalTypes;

namespace VR4300
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

	enum class CP0_Instr
	{
		/* Move instructions */
		MTC0, MFC0, DMTC0, DMFC0,

		/* TLB instructions */
		TLBP, TLBR, TLBWI, TLBWR,

		/* Misc. instructions */
		ERET, CACHE
	};

	enum class FPU_Instr
	{
		/* Load/store/transfer instructions */
		LWC1, SWC1, LDC1, SDC1, MTC1, MFC1, CTC1, CFC1, DMTC1, DMFC1,

		/* Conversion instructions */
		CVT_S, CVT_D, CVT_L, CVT_W, ROUND_L, ROUND_W, TRUNC_L, TRUNC_W, CEIL_L, CEIL_W, FLOOR_L, FLOOR_W,

		/* Computational instructions */
		ADD, SUB, MUL, DIV, ABS, MOV, NEG, SQRT,

		/* Branch instructions */
		BC1T, BC1F, BC1TL, BC1FL
	};

	template<typename T>
	concept FPU_NumericType =
		std::is_same_v<f32, typename std::remove_cv<T>::type> ||
		std::is_same_v<f64, typename std::remove_cv<T>::type> ||
		std::is_same_v<s32, typename std::remove_cv<T>::type> ||
		std::is_same_v<s64, typename std::remove_cv<T>::type>;

	struct
	{
		unsigned IE : 1; /* Specifies and indicates global interrupt enable (0: disable interrupts; 1: enable interrupts) */
		unsigned EXL : 1; /* Specifies and indiciates exception level (0: normal; 1: exception) */
		unsigned ERL : 1; /* Specifies and indiciates error level (0: normal; 1: error) */
		unsigned KSU : 2; /* Specifies and indicates mode bits (00: kernel; 01: supervisor; 10: user) */
		unsigned UX : 1; /* Enables 64-bit addressing and operations in User mode (0: 32-bit; 1: 64-bit) */
		unsigned SX : 1; /* Enables 64-bit addressing and operations in Supervisor mode (0: 32-bit; 1: 64-bit) */
		unsigned KX : 1; /* Enables 64-bit addressing in Kernel mode (0: 32-bit; 1: 64-bit) */
		unsigned IM : 8; /* Interrupt Mask field */
		unsigned DS : 9; /* Diagnostic Status field */
		unsigned RE : 1; /* Reverse-Endian bit, enables reverse of system endianness in User mode (0: disabled; 1: reversed) */
		unsigned FR : 1; /* Enables additional floating-point registers (0: 16 registers; 1: 32 registers) */
		unsigned RP : 1; /* Enables low-power operation by reducing the internal clock frequency and the system interface clock frequency to one-quarter speed (0: normal; 1: low power mode) */
		unsigned CU : 4; /* Controls the usability of each of the four coprocessor unit numbers (0: unusable; 1: usable)  */
	} status_reg{};

	struct
	{
		u32 index; /* (0) */
		u32 random; /* (1) */
		u64 entry_lo0; /* (2) */
		u64 entry_hi1; /* (3) */
		u64 context; /* (4) */
		u32 page_mask; /* (5) */
		u32 wired; /* (6) */
		u64 bad_v_addr; /* (8) */
		u32 count; /* (9) */
		u64 entry_hi; /* (10) */
		u32 compare; /* (11) */
		u32 status; /* (12) */
		u32 cause; /* (13) */
		u64 epc; /* (14) */
		const u32 pr_id = 0; /* (15) */
		u32 config; /* (16) */
		u32 LL_addr; /* (17) */
		u32 watch_lo; /* (18) */
		u32 watch_hi; /* (19) */
		u64 x_context; /* (20) */
		u32 parity_error; /* (26) */
		u32 cache_error; /* (27) */
		u32 tag_lo; /* (28) */
		u32 tag_hi; /* (29) */
		u64 error_epc; /* (30) */


		u32 Get(const size_t index) const
		{
			return u32();
		}

		void Set(const size_t index, u64 data)
		{

		}
	} CP0_GPR{};

	struct FPU_Control_31
	{
		void Set(const u32 data)
		{
			/* TODO */
			/* after updating RM... */
			const int new_rounding_mode = [&] {
				switch (RM)
				{
				case 0b00: return FE_TONEAREST;  /* RN */
				case 0b01: return FE_TOWARDZERO; /* RZ */
				case 0b10: return FE_UPWARD;     /* RP */
				case 0b11: return FE_DOWNWARD;   /* RM */
				default: return 0; /* impossible */
				}
			}();
			std::fesetround(new_rounding_mode);
			/* TODO: initial rounding mode? */
		}

		u32 Get() const { return std::bit_cast<u32, FPU_Control_31>(*this); }

		unsigned RM : 2;

		unsigned flag_I : 1; /* Inexact Operation */
		unsigned flag_U : 1; /* Underflow */
		unsigned flag_O : 1; /* Overflow */
		unsigned flag_Z : 1; /* Division by Zero */
		unsigned flag_V : 1; /* Invalid Operation */

		unsigned enable_I : 1;
		unsigned enable_U : 1;
		unsigned enable_O : 1;
		unsigned enable_Z : 1;
		unsigned enable_V : 1;

		unsigned cause_I : 1;
		unsigned cause_U : 1;
		unsigned cause_O : 1;
		unsigned cause_Z : 1;
		unsigned cause_V : 1;
		unsigned cause_E : 1; /* Unimplemented Operation */

	private:
		const unsigned padding0 : 5 = 0;

	public:
		unsigned C : 1;
		unsigned FS : 1;

	private:
		const unsigned padding1 : 7 = 0;
	} FCR31{};

	struct GeneralPurposeRegisters
	{
		inline u64 Get(const size_t index) const { return GPR[index]; }
		inline void Set(const size_t index, const u64 data) { if (index != 0) GPR[index] = data; }
		u64 operator[](const size_t index) { return GPR[index]; } /* returns by value so that assignments have to made through function "Set". */
	private:
		std::array<u64, 32> GPR{};
	} GPR;

	struct FloatingPointGeneralPurposeRegisters
	{
		template<typename FPU_NumericType>
		inline FPU_NumericType Get(const size_t index) const
		{
			if constexpr (std::is_same<FPU_NumericType, s32>::value)
				return s32(FGR[index]);
			else if constexpr (std::is_same<FPU_NumericType, s64>::value)
				return status_reg.FR ? FGR[index] : FGR[index] & 0xFFFFFFFF | FGR[index + 1] << 32; /* TODO: if index is odd, result is supposed to be undefined. If index == 31, then that is very bad */
			else if constexpr (std::is_same<FPU_NumericType, f32>::value)
				return std::bit_cast<f32, s32>(s32(FGR[index]));
			else if constexpr (std::is_same<FPU_NumericType, f64>::value)
				return status_reg.FR ? std::bit_cast<f64, s64>(FGR[index])
				: std::bit_cast<f64, s64>(FGR[index] & 0xFFFFFFFF | FGR[index + 1] << 32);
			else
				static_assert(false);
		}

		template<typename FPU_NumericType>
		inline void Set(const size_t index, const FPU_NumericType data)
		{
			if constexpr (std::is_same<FPU_NumericType, s32>::value)
			{
				FGR[index] = data;
			}
			else if constexpr (std::is_same<FPU_NumericType, s64>::value)
			{
				if (FCR31.FS)
					FGR[index] = data;
				else
				{
					FGR[index] = data & 0xFFFFFFFF;
					FGR[index + 1] = data >> 32; /* TODO: no clue if sign-extending will lead to unwanted results */
				}
			}
			else if constexpr (std::is_same<FPU_NumericType, f32>::value)
			{
				FGR[index] = std::bit_cast<s32, f32>(data); /* TODO: no clue if sign-extending will lead to unwanted results */
			}
			else if constexpr (std::is_same<FPU_NumericType, f64>::value)
			{
				if (FCR31.FS)
					FGR[index] = std::bit_cast<s64, f64>(data);
				else
				{
					const s64 conv = std::bit_cast<s64, f64>(data);
					FGR[index] = conv & 0xFFFFFFFF;
					FGR[index + 1] = conv >> 32; /* TODO: no clue if sign-extending will lead to unwanted results */
				}
			}
			else
				static_assert(false);
		}

	private:
		std::array<s64, 32> FGR{};
	} FGR;

	struct FloatingPointControlRegisters
	{
		u32 Get(const size_t index) const
		{
			if (index == 0)
				return 0;
			else if (index == 31)
				return FCR31.Get();
			else
				return 0; /* TODO ??? */
		}

		void Set(const size_t index, const u32 data)
		{
			if (index == 0)
				;
			else if (index == 31)
				FCR31.Set(data);
			else
				; /* TODO ??? */
		}
	} FPU_control;

	u64 PC;

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

	/* COP0 instructions */
	template<CP0_Instr instr> void CP0_Move(const u32 instr_code);
	void TLBR();
	void TLBWI();
	void TLBWR();
	void TLBP();
	void ERET();
	void CACHE(const u32 instr_code);

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
	void ReservedInstructionException();
	void TrapException();
	void UnderflowException();
	void UnimplementedOperationException();

	void ExecuteInstruction();
	void DecodeAndExecuteInstruction(const u32 instr_code);
}