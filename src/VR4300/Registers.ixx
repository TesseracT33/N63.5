export module VR4300:Registers;

import :COP1;
import :MMU;
import :Operation;

import NumericalTypes;

import <array>;
import <bit>;
import <cfenv>;
import <concepts>;
import <limits>;
import <type_traits>;

namespace VR4300
{
	/* For (COP0) registers (structs) that, once they have been written to, need to tell the rest of the cpu about it. */
	template<typename T>
	concept notifies_cpu_on_write = requires(T t)
	{
		{ t.notify_cpu_after_write() } -> std::convertible_to<void>;
	};

	u64 PC{}; /* Program counter */

	u64 HI{}, LO{}; /* Contain the result of a double-word multiplication or division. */

	bool LL_bit{}; /* Read from / written to by load linked and store conditional instructions. */

	/* CPU general-purpose registers */
	struct
	{
		u64 Get(const size_t index) const { return GPR[index]; }
		void Set(const size_t index, const u64 data) { if (index != 0) GPR[index] = data; }
		u64 operator[](const size_t index) { return GPR[index]; } /* returns by value so that assignments have to made through function "Set". */
	private:
		std::array<u64, 32> GPR{};
	} GPR;

	/* COP0 registers. Used for exception handling and memory management. */
	struct
	{
		struct /* (0) */
		{
			u32 index : 6; /* Index to the TLB entry affected by the TLB Read (TLBR) and TLB Write (TLBW) instructions. */
			u32 : 25;
			u32 P : 1; /* Shows the success (0) or failure (1) of the last TLB Probe (TLBP) instruction executed. */
		} index{};

		struct /* (1) */
		{
			u32 random : 5; /* Decrements every instruction, and specifies the entry in the TLB that is affected by the TLB Write instruction. */
			u32 : 1; /* R/W, but has no function. */
			u32 : 26;
		} random{};

		struct /* (2), (3); Used to rewrite the TLB or to check coincidence of a TLB entry when addresses are converted. */
		{
			u32 G : 1; /* Global. If this bit is set in both EntryLo0 and EntryLo1, then the processor ignores the ASID during TLB lookup. */
			u32 V : 1; /* Valid. If this bit is set, it indicates that the TLB entry is valid; otherwise, a TLBL or TLBS miss occurs. */
			u32 D : 1; /* Dirty. If this bit is set, the page is marked as dirty, and therefore writable. */
			u32 C : 3; /* Specifies the TLB page attribute. */
			u32 PFN : 20; /* Page frame number; the high-order bits of the physical address. */
			u32 : 6;
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

		u32 wired : 6; /* (6); Specifies the boundary between the "wired" and "random" entries of the TLB; wired entries cannot be overwritten by a TLBWR operation. */

		u64 bad_v_addr; /* (8) */
		u32 count; /* (9) */

		struct /* (10) */
		{
			u64 ASID : 8; /* Address space ID field. Lets multiple processes share the TLB; virtual addresses for each process can be shared. */
			u64 padding_of_zeroes : 5; /* named because it needs to be set to zero in the TLBR instruction, as data is copied to here from a TLB entry that is not zero. */
			u64 VPN2 : 27; /* Virtual page number divided by two (maps to two pages). */
			u64 : 22;
			u64 R : 2; /* Region (00 => user; 01 => supervisor; 11 => kernel) used to match virtual address bits 63..62. */
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
			u32 DE : 1;
			u32 CE : 1;
			u32 CH : 1;
			u32 : 1;
			u32 SR : 1;
			u32 TS : 1;
			u32 BEV : 1;
			u32 : 1;
			u32 ITS : 1;
			u32 RE : 1; /* Reverse-Endian bit, enables reverse of system endianness in User mode (0: disabled; 1: reversed) */
			u32 FR : 1; /* Enables additional floating-point registers (0: 16 registers; 1: 32 registers) */
			u32 RP : 1; /* Enables low-power operation by reducing the internal clock frequency and the system interface clock frequency to one-quarter speed (0: normal; 1: low power mode) */
			u32 CU : 4; /* Controls the usability of each of the four coprocessor unit numbers (0: unusable; 1: usable)  */

			void notify_cpu_after_write()
			{
				AssignActiveVirtualToPhysicalFunctions();
				SetNewEndianness();
			}
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

		struct /* (15) */
		{
			u32 rev : 8; /* Processor revision number */
			u32 imp : 8 = 0x0B; /* Processor ID number */
			u32 : 16;
		} const pr_id{};

		struct /* (16) */
		{
			u32 K0 : 3; /* Sets coherency algorithm of kseg0 (010 => cache is not used; else => cache is used). */
			u32 CU : 1; /* RFU. However, can be read or written by software. */
			u32 : 11; /* Returns 11001000110 when read. */
			u32 BE : 1; /* Sets endianness (0 => little endian; 1 => big endian (default on cold reset). */
			u32 : 8; /* Returns 00000110 when read. */
			u32 EP : 4; /* Sets transfer data pattern (single/block write request) (0 => D (default on cold reset); 6 => DxxDxx (2 doublewords/six cycles). */
			u32 EC : 3; /* Operating frequency ratio (read-only). */
			u32 : 1; /* Returns 0 when read. */

			void notify_cpu_after_write()
			{
				SetNewEndianness();
			}
		} config{};

		u32 LL_addr; /* (17); Contains the physical address read by the most recent Load Linked instruction. */

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
			u32 p_state : 2; /* Specified the primary cache state (Data cache: 11 => valid; 00 => invalid) (Instruction cache: 10 => valid; 00 => invalid). All others: undefined. */
			u32 p_tag_lo : 20; /* Physical address bits 31..12. */
			u32 : 4;
		} tag_lo{}; /* Holds the primary cache tag for cache initialization, cache diagnostics, or cache error processing. The Tag registers are written by the CACHE and MTC0 instructions. */

		const u32 tag_hi = 0; /* (29); Always returns 0 when read. */
		u64 error_epc; /* (30) */


		u64 Get(const size_t register_index) const
		{
			auto get_struct_reg = [](const auto& structure) -> u64 /* Non-UB type punning between a struct register and an u32/u64. */
			{
				if constexpr (sizeof structure == sizeof u64)
					return std::bit_cast<u64, std::remove_reference_t<decltype(structure)>>(structure);
				else if constexpr (sizeof structure == sizeof u32)
					return static_cast<u64>(std::bit_cast<u32, std::remove_reference_t<decltype(structure)>>(structure));
				else
					static_assert(false, "Incorrectly sized struct given.");
			};

			switch (register_index)
			{
			case 0: return get_struct_reg(index);
			case 1: return get_struct_reg(random);
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
			case 15: return get_struct_reg(pr_id);
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
			auto set_struct_reg = [](auto& structure, const u64 value) -> void /* Non-UB type punning between a struct register and an u32/u64. */
			{
				if constexpr (sizeof structure == sizeof u64)
					structure = std::bit_cast<std::remove_reference_t<decltype(structure)>, u64>(value);
				else if constexpr (sizeof structure == sizeof u32)
					structure = std::bit_cast<std::remove_reference_t<decltype(structure)>, u32>(static_cast<u32>(value));
				else
					static_assert(false, "Incorrectly sized struct given.");

				/* For registers that, once they have been written to, need to tell the rest of the cpu about it. */
				if constexpr (notifies_cpu_on_write<decltype(structure)>)
					structure.notify_cpu_after_write();
			};

			switch (register_index)
			{
			break; case 0: set_struct_reg(index, value & 0x800000CF);
			break; case 1: set_struct_reg(random, value & 0x3F);
			break; case 2: set_struct_reg(entry_lo_0, value & 0xCFFFFFFF);
			break; case 3: set_struct_reg(entry_lo_1, value & 0xCFFFFFFF);
			break; case 4: set_struct_reg(context, value & 0xFFFFFFF0);
			break; case 5: set_struct_reg(page_mask, value & 0x01FFE000);
			break; case 6: wired = value;
			break; case 8: bad_v_addr = value;
			break; case 9: count = u32(value);
			break; case 10: set_struct_reg(entry_hi, value & 0xC00000FFFFFFE0FF);
			break; case 11: compare = u32(value);
			break; case 12: set_struct_reg(status, value);
			break; case 13: set_struct_reg(cause, value & 0xB000FF7C);
			break; case 14: epc = value;
			break; case 16: set_struct_reg(config, value & 0x7F00800F | 0xC6460);
			break; case 17: LL_addr = u32(value);
			break; case 18: set_struct_reg(watch_lo, value & 0xFFFFFFFB);
			break; case 19: watch_hi_p_addr_1 = value;
			break; case 20: set_struct_reg(x_context, value & 0xFFFFFFFF'FFFFFFF0);
			break; case 26: parity_err_diagnostic = value;
			break; case 28: set_struct_reg(tag_lo, value & 0x0FFFFFC0);
			break; case 30: error_epc = value;
			}
		}
	} COP0_reg{};

	/* Floating point control register #31 */
	struct
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

		u32 Get() const { return std::bit_cast<u32, std::remove_reference_t<decltype(*this)>>(*this); }

		u32 RM : 2;

		u32 flag_I : 1; /* Inexact Operation */
		u32 flag_U : 1; /* Underflow */
		u32 flag_O : 1; /* Overflow */
		u32 flag_Z : 1; /* Division by Zero */
		u32 flag_V : 1; /* Invalid Operation */

		u32 enable_I : 1;
		u32 enable_U : 1;
		u32 enable_O : 1;
		u32 enable_Z : 1;
		u32 enable_V : 1;

		u32 cause_I : 1;
		u32 cause_U : 1;
		u32 cause_O : 1;
		u32 cause_Z : 1;
		u32 cause_V : 1;
		u32 cause_E : 1; /* Unimplemented Operation */

		u32 : 5;
		u32 C : 1;
		u32 FS : 1;
		u32 : 7;
	} FCR31{};

	/* Floating point control registers. */
	struct
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

	/* General-purpose floating point registers. */
	struct
	{
		template<typename FPU_NumericType>
		FPU_NumericType Get(const size_t index) const
		{
			if constexpr (std::is_same<FPU_NumericType, s32>::value)
				return s32(FGR[index]);
			else if constexpr (std::is_same<FPU_NumericType, s64>::value)
				return COP0_reg.status.FR ? FGR[index] : FGR[index] & 0xFFFFFFFF | FGR[index + 1] << 32; /* TODO: if index is odd, result is supposed to be undefined. If index == 31, then that is very bad */
			else if constexpr (std::is_same<FPU_NumericType, f32>::value)
				return std::bit_cast<f32, s32>(s32(FGR[index]));
			else if constexpr (std::is_same<FPU_NumericType, f64>::value)
				return COP0_reg.status.FR ? std::bit_cast<f64, s64>(FGR[index])
				: std::bit_cast<f64, s64>(FGR[index] & 0xFFFFFFFF | FGR[index + 1] << 32);
			else
				static_assert(false);
		}

		template<typename FPU_NumericType>
		void Set(const size_t index, const FPU_NumericType data)
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
}