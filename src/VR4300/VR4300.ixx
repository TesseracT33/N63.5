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

import NumericalTypes;

namespace VR4300
{
	export
	{
		void Reset();
		void Run(const int cycles);

		enum class OperatingMode { User, Supervisor, Kernel } operatingMode;

		void AddressErrorException();
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

	/* COP1 registers */
	struct
	{
		struct /* (0) */
		{
			u32 index :  6; /* Index to the TLB entry affected by the TLB Read (TLBR) and TLB Write (TLBW) instructions. */
			u32       : 25;
			u32 P     :  1; /* Shows the success (0) or failure (1) of the last TLB Probe (TLBP) instruction executed. */
		} index{};

		u32 random : 6; /* (1); Decrements every instruction, and specifies the entry in the TLB that is affected by the TLB Write instruction.  */

		struct /* (2), (3); Used to rewrite the TLB or to check coincidence of a TLB entry when addresses are converted. */
		{
			u32 G   :  1; /* Global. If this bit is set in both EntryLo0 and EntryLo1, then the processor ignores the ASID during TLB lookup. */
			u32 V   :  1; /* Valid. If this bit is set, it indicates that the TLB entry is valid; otherwise, a TLBL or TLBS miss occurs. */
			u32 D   :  1; /* Dirty. If this bit is set, the page is marked as dirty, and therefore writable. */
			u32 C   :  3; /* Specifies the TLB page attribute. */
			u32 PFN : 20; /* Page frame number; the high-order bits of the physical address. */
			u32     :  6;
		} entry_lo_0{}, entry_lo_1{};

		struct /* (4) */
		{
			u64 : 4;
			u64 bad_vpn2 : 19;
			u64 pte_base : 41;
		} context{};

		struct /* (5) */
		{
			u32 : 13;
			u32 MASK : 12; /* Sets the page size for each TLB entry. 0 => 4 KB; 3 => 16 KB; 15 => 64 KB; 63 => 256 KB; 255 => 1 MB; 1023 => 4 MB; 4095 => 16 MB. Else, the operation of the TLB is undefined. */
			u32 : 7;
		} page_mask{};

		u32 wired : 6; /* (6) */

		u64 bad_v_addr; /* (8) */
		u32 count; /* (9) */

		struct /* (10) */
		{
			u64 ASID : 8;
			u64 : 5;
			u64 VPN2 : 27;
			u64 Fill : 22;
			u64 R : 2;
		} entry_hi{}; /* TODO can be 32 bits large with different lengths of VPN2 etc */

		u32 compare; /* (11) */

		struct /* (12) */
		{
			u32 IE : 1; /* Specifies and indicates global interrupt enable (0: disable interrupts; 1: enable interrupts) */
			u32 EXL : 1; /* Specifies and indiciates exception level (0: normal; 1: exception) */
			u32 ERL : 1; /* Specifies and indiciates error level (0: normal; 1: error) */
			u32 KSU : 2; /* Specifies and indicates mode bits (00: kernel; 01: supervisor; 10: user) */
			u32 UX : 1; /* Enables 64-bit addressing and operations in User mode (0: 32-bit; 1: 64-bit) */
			u32 SX : 1; /* Enables 64-bit addressing and operations in Supervisor mode (0: 32-bit; 1: 64-bit) */
			u32 KX : 1; /* Enables 64-bit addressing in Kernel mode (0: 32-bit; 1: 64-bit) */
			u32 IM : 8; /* Interrupt Mask field */
			u32 DS : 9; /* Diagnostic Status field */
			u32 RE : 1; /* Reverse-Endian bit, enables reverse of system endianness in User mode (0: disabled; 1: reversed) */
			u32 FR : 1; /* Enables additional floating-point registers (0: 16 registers; 1: 32 registers) */
			u32 RP : 1; /* Enables low-power operation by reducing the internal clock frequency and the system interface clock frequency to one-quarter speed (0: normal; 1: low power mode) */
			u32 CU : 4; /* Controls the usability of each of the four coprocessor unit numbers (0: unusable; 1: usable)  */
		} status{};

		struct /* (13) */
		{
			u32 : 2;
			u32 exc_code : 5;
			u32 : 1;
			u32 IP : 8;
			u32 : 12;
			u32 CE : 2;
			u32 : 1;
			u32 BD : 1;
		} cause{};

		u64 epc; /* (14) */
		const u32 pr_id = 0; /* (15) */

		struct /* (16) */
		{
			u32 K0 : 3;
			u32 CU : 1;
			u32 : 11;
			u32 BE : 1;
			u32 : 8;
			u32 EP : 4;
			u32 EC : 3;
			u32 : 1;
		} config{};

		u32 LL_addr; /* (17) */

		struct /* (18) */
		{
			u32 W : 1;
			u32 R : 1;
			u32 : 1;
			u32 p_addr_0 : 29;
		} watch_lo{};

		u32 watch_hi_p_addr_1 : 4; /* (19) */

		struct /* (20) */
		{
			u64 : 4;
			u64 bad_vpn2 : 27;
			u64 R : 2;
			u64 pte_base : 31;
		} x_context{};

		u32 parity_err_diagnostic : 8; /* (26) */

		const u32 cache_error = 0; /* (27) */

		struct /* (28) */
		{
			u32 : 6;
			u32 p_state : 2;
			u32 p_tag_lo : 20;
			u32 : 4;
		} tag_lo{};

		const u32 tag_hi = 0; /* (29) */
		u64 error_epc; /* (30) */


		u64 Get(const size_t register_index) const 
		{
			auto get_struct_reg = [] (const auto& struc) -> u64 /* Non-UB type punning between a struct register and an u32/u64. */
			{
				if constexpr (sizeof struc == sizeof u64) 
					return std::bit_cast<u64, std::remove_reference_t<decltype(struc)>>(struc);
				else if constexpr (sizeof struc == sizeof u32)
					return static_cast<u64>(std::bit_cast<u32, std::remove_reference_t<decltype(struc)>>(struc));
				else
					static_assert(false, "Incorrectly sized struct given.");
			};

			switch (register_index)
			{
			case 0: return get_struct_reg(index);
			case 1: return random;
			case 2: return get_struct_reg(entry_lo_0); 
			case 3: return get_struct_reg(entry_lo_1); 
			case 4: return get_struct_reg(context); 
			case 5: return get_struct_reg(page_mask); 
			case 6: return wired; 
			case 8: return bad_v_addr; 
			case 9: return count; 
			case 10: return get_struct_reg(entry_hi); 
			case 11: return compare; 
			case 12: return get_struct_reg(status); 
			case 13: return get_struct_reg(cause);
			case 14: return epc; 
			case 15: return pr_id;
			case 16: return get_struct_reg(config);
			case 17: return LL_addr; 
			case 18: return get_struct_reg(watch_lo); 
			case 19: return watch_hi_p_addr_1; 
			case 20: return get_struct_reg(x_context); 
			case 26: return parity_err_diagnostic; 
			case 28: return get_struct_reg(tag_lo); 
			case 30: return error_epc; 

			default: return 0;
			}
		}

		void Set(const size_t register_index, const u64 value)
		{
			auto set_struct_reg = [] (auto& struc, const u64 value) -> void /* Non-UB type punning between a struct register and an u32/u64. */
			{
				if constexpr (sizeof struc == sizeof u64)
					struc = std::bit_cast<std::remove_reference_t<decltype(struc)>, u64>(value);
				else if constexpr (sizeof struc == sizeof u32)
					struc = std::bit_cast<std::remove_reference_t<decltype(struc)>, u32>(static_cast<u32>(value));
				else
					static_assert(false, "Incorrectly sized struct given.");
			};

			switch (register_index)
			{
			break; case 0: set_struct_reg(index, value & 0x800000CF); 
			break; case 1: random = value; 
			break; case 2: set_struct_reg(entry_lo_0, value & 0xCFFFFFFF); 
			break; case 3: set_struct_reg(entry_lo_1, value & 0xCFFFFFFF); 
			break; case 4: set_struct_reg(context, value & 0xFFFFFFF0); 
			break; case 5: set_struct_reg(page_mask, value & 0x01FFE000); 
			break; case 6: wired = value; 
			break; case 8: bad_v_addr = value; 
			break; case 9: count = value; 
			break; case 10: set_struct_reg(entry_hi, value & 0xFFFFE0FF); 
			break; case 11: compare = value; 
			break; case 12: set_struct_reg(status, value); 
			break; case 13: set_struct_reg(cause, value & 0xB000FF7C); 
			break; case 14: epc = value; 
			break; case 16: set_struct_reg(config, value & 0x7F00800F | 0xC6460); 
			break; case 17: LL_addr = value; 
			break; case 18: set_struct_reg(watch_lo, value & 0xFFFFFFFB); 
			break; case 19: watch_hi_p_addr_1 = value; 
			break; case 20: set_struct_reg(x_context, value & 0xFFFFFFFF'FFFFFFF0); 
			break; case 26: parity_err_diagnostic = value; 
			break; case 28: set_struct_reg(tag_lo, value & 0x0FFFFFC0); 
			break; case 30: error_epc = value; 
			}
		}
	} CP0_reg{};

	struct FPU_Control_31
	{
		void Set(const u32 data)
		{
			/* TODO */
			/* after updating RM... */
			const int new_rounding_mode = [&] {
				switch (RM)
				{
				break; case 0b00: return FE_TONEAREST;  /* RN */
				break; case 0b01: return FE_TOWARDZERO; /* RZ */
				break; case 0b10: return FE_UPWARD;     /* RP */
				break; case 0b11: return FE_DOWNWARD;   /* RM */
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
				return CP0_reg.status.FR ? FGR[index] : FGR[index] & 0xFFFFFFFF | FGR[index + 1] << 32; /* TODO: if index is odd, result is supposed to be undefined. If index == 31, then that is very bad */
			else if constexpr (std::is_same<FPU_NumericType, f32>::value)
				return std::bit_cast<f32, s32>(s32(FGR[index]));
			else if constexpr (std::is_same<FPU_NumericType, f64>::value)
				return CP0_reg.status.FR ? std::bit_cast<f64, s64>(FGR[index])
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