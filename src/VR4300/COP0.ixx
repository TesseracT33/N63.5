export module VR4300:COP0;

import NumericalTypes;

import <algorithm>;
import <bit>;
import <cassert>;
import <cstring>;

namespace VR4300
{
	enum class CP0_Instr
	{
		/* Move instructions */
		MTC0, MFC0, DMTC0, DMFC0,

		/* TLB instructions */
		TLBP, TLBR, TLBWI, TLBWR,

		/* Misc. instructions */
		ERET, CACHE
	};

	/* The CP0 'cause' register is set to these values during an exception (p. 172) */
	enum class CauseExcCode
	{
		Int, /* Interrupt */
		Mod, /* TLB Modification exception */
		TLBL, /* TLB Miss exception (load or instruction fetch) */
		TLBS, /* TLB Miss exception (store) */
		AdEL, /* Address Error exception (load of instruction fetch) */
		AdES, /* Address Error exception (store) */
		IBE, /* Bus Error exception (instruction fetch) */
		DBE, /* Bus Error exception (data reference: load or store) */
		Sys, /* Syscall exception */
		Bp, /* Breakpoint exception */
		RI, /* Reserved Instruction exception */
		CpU, /* Coprocessor Unusable exception */
		Ov, /* Arithmetic Overflow exception */
		Tr, /* Trap exception */
		FPE = 15, /* Floating-Point exception */
		WATCH = 23 /* Watch exception */
	};

	/* COP1 registers */
	struct
	{
		struct /* (0) */
		{
			u32 index : 6; /* Index to the TLB entry affected by the TLB Read (TLBR) and TLB Write (TLBW) instructions. */
			u32 : 25;
			u32 P : 1; /* Shows the success (0) or failure (1) of the last TLB Probe (TLBP) instruction executed. */
		} index{};

		/* TODO is actually six bits, but has range 0-31 */
		u32 random : 5; /* (1); Decrements every instruction, and specifies the entry in the TLB that is affected by the TLB Write instruction.  */

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
			auto get_struct_reg = [](const auto& struc) -> u64 /* Non-UB type punning between a struct register and an u32/u64. */
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
			auto set_struct_reg = [](auto& struc, const u64 value) -> void /* Non-UB type punning between a struct register and an u32/u64. */
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
			break; case 10: set_struct_reg(entry_hi, value & 0xC00000FFFFFFE0FF);
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

	/* COP0 instructions */
	template<CP0_Instr instr> void CP0_Move(const u32 instr_code);
	void TLBR();
	void TLBWI();
	void TLBWR();
	void TLBP();
	void ERET();
	void CACHE(const u32 instr_code);

	unsigned virt_addr_page_size_in_bits;
	u32 virt_addr_offset_mask;
	u32 virt_addr_VPN_mask;
	u32 virt_addr_shift_count;

	void SetPageMask()
	{
		switch (CP0_reg.page_mask.MASK)
		{
		case 0: virt_addr_offset_mask = 0xFFF; virt_addr_shift_count = 12; break;
		case 0b11: virt_addr_offset_mask = 0x3FFF; virt_addr_shift_count = 14; break;
		case 0b1111: virt_addr_offset_mask = 0xFFFF; virt_addr_shift_count = 16; break;
		case 0b111111: virt_addr_offset_mask = 0x3FFFF; virt_addr_shift_count = 18; break;
		case 0b11111111: virt_addr_offset_mask = 0xFFFFF; virt_addr_shift_count = 20; break;
		case 0b1111111111: virt_addr_offset_mask = 0x3FFFFF; virt_addr_shift_count = 22; break;
		case 0b111111111111: virt_addr_offset_mask = 0xFFFFFF; virt_addr_shift_count = 24; break;
		default: assert(false);
		}
		virt_addr_VPN_mask = ~virt_addr_offset_mask;
	}
}