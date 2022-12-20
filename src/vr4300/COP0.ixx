export module VR4300:COP0;

import Util;

import <array>;
import <bit>;
import <cassert>;
import <concepts>;
import <cstring>;
import <random>;

namespace VR4300
{
	enum class Cop0Instruction {
		/* Move instructions */
		MTC0, MFC0, DMTC0, DMFC0,

		/* TLB instructions */
		TLBP, TLBR, TLBWI, TLBWR,

		/* Misc. instructions */
		ERET
	};

	void OnCountCompareMatchEvent();
	template<bool initial_add = false> void ReloadCountCompareEvent();

	/* COP0 instructions */
	template<Cop0Instruction> void Cop0Move(u32 rt, u32 rd);
	void TLBR();
	void TLBWI();
	void TLBWR();
	void TLBP();
	void ERET();

	constexpr uint cop0_index_index = 0;
	constexpr uint cop0_index_random = 1;
	constexpr uint cop0_index_entry_lo_0 = 2;
	constexpr uint cop0_index_entry_lo_1 = 3;
	constexpr uint cop0_index_context = 4;
	constexpr uint cop0_index_page_mask = 5;
	constexpr uint cop0_index_wired = 6;
	constexpr uint cop0_index_bad_v_addr = 8;
	constexpr uint cop0_index_count = 9;
	constexpr uint cop0_index_entry_hi = 10;
	constexpr uint cop0_index_compare = 11;
	constexpr uint cop0_index_status = 12;
	constexpr uint cop0_index_cause = 13;
	constexpr uint cop0_index_epc = 14;
	constexpr uint cop0_index_pr_id = 15;
	constexpr uint cop0_index_config = 16;
	constexpr uint cop0_index_ll_addr = 17;
	constexpr uint cop0_index_watch_lo = 18;
	constexpr uint cop0_index_watch_hi = 19;
	constexpr uint cop0_index_x_context = 20;
	constexpr uint cop0_index_parity_error = 26;
	constexpr uint cop0_index_cache_error = 27;
	constexpr uint cop0_index_tag_lo = 28;
	constexpr uint cop0_index_tag_hi = 29;
	constexpr uint cop0_index_error_epc = 30;

	/* TODO: for registers that contain only one field, make them simple u32/u64, not structs */
	struct Cop0Registers {
		struct { /* (0) */
			u32 value : 6; /* Index to the TLB entry affected by the TLB Read (TLBR) and TLB Write (TLBW) instructions. */
			u32 : 25;
			u32 p : 1; /* Shows the success (0) or failure (1) of the last TLB Probe (TLBP) instruction executed. */
		} index;

		u32 random; /* (1) 6 bits; Decremented every instruction, and specifies the entry in the TLB that is affected by the TLB Write instruction. */

		struct EntryLo { /* (2), (3); Used to rewrite the TLB or to check coincidence of a TLB entry when addresses are converted. */
			u32 g : 1; /* Global. If this bit is set in both EntryLo0 and EntryLo1, then the processor ignores the ASID during TLB lookup. */
			u32 v : 1; /* Valid. If this bit is set, it indicates that the TLB entry is valid; otherwise, a TLBL or TLBS miss occurs. */
			u32 d : 1; /* Dirty. If this bit is set, the page is marked as dirty, and therefore writable. */
			u32 c : 3; /* Specifies the TLB page attribute. */
			u32 pfn : 20; /* Page frame number -- the high-order bits of the physical address. */
			u32 : 6;
		} entry_lo[2];

		struct { /* (4) */
			u64 : 4;
			u64 bad_vpn2 : 19; /* The virtual page number divided by two, given during e.g. a TLB miss.  */
			u64 pte_base : 41; /* The base address of the page table entry, given during e.g. a TLB miss.  */
		} context;

		u32 page_mask; /* (5) Sets the page size for each TLB entry. Bits 0-12, 25-31 are zero. The rest:
			0 => 4 KB; 3 => 16 KB; 15 => 64 KB; 63 => 256 KB; 255 => 1 MB; 1023 => 4 MB; 4095 => 16 MB.
			Else, the operation of the TLB is undefined. As 0 represent 4 KB i.e. 12 bits, and bit 12 is 0,
			this register is actually used to mask a virtual addr to get its VPN2. */

		u32 wired; /* (6); Specifies the boundary between the "wired" and "random" entries of the TLB; wired entries cannot be overwritten by a TLBWR operation. */

		u32 cop0_unused_7; /* (7) */

		u64 bad_v_addr; /* (8) Virtual address at which an address error occurred last or which failed in address translation. */

		u64 count; /* (9); Increments every other PClock. When equal to the Compare register, interrupt bit IP(7) in the Cause register is set. */
			 /* On real HW, the register is 32 bits. Here, we make it 64 bits and increment it every PCycle instead of every other PCycle.
			 When we read from it, we shift it right one bit and then return it. When we write to it, we set it to the data shifted left one bit. */

		struct EntryHi { /* (10) */
			u64 asid : 8; /* Address space ID field. Lets multiple processes share the TLB; virtual addresses for each process can be shared. */
			u64 : 4;
			u64 g : 1;
			u64 vpn2 : 27; /* Virtual page number divided by two (maps to two pages). */
			u64 : 22;
			u64 r : 2; /* Region (00 => user; 01 => supervisor; 11 => kernel) used to match virtual address bits 63..62. */
		} entry_hi;

		u64 compare; /* (11); When equal to the Count register, interrupt bit IP(7) in the Cause register is set. Writes to this register clear said interrupt. */
			/* On real HW, this register is 32 bits. Here, we make it 64 bits. See the description of the 'Count' register. */

		struct { /* (12) */
			u32 ie : 1; /* Specifies and indicates global interrupt enable (0: disable interrupts; 1: enable interrupts) */
			u32 exl : 1; /* Specifies and indiciates exception level (0: normal; 1: exception) */
			u32 erl : 1; /* Specifies and indiciates error level (0: normal; 1: error) */
			u32 ksu : 2; /* Specifies and indicates mode bits (00: kernel; 01: supervisor; 10: user) */
			u32 ux : 1; /* Enables 64-bit addressing and operations in User mode (0: 32-bit; 1: 64-bit) */
			u32 sx : 1; /* Enables 64-bit addressing and operations in Supervisor mode (0: 32-bit; 1: 64-bit) */
			u32 kx : 1; /* Enables 64-bit addressing in Kernel mode (0: 32-bit; 1: 64-bit) */
			u32 im : 8; /* Interrupt Mask fields (0: disabled; 1: enabled). Software interrupt (IM0-IM1); External normal interrupts (IM2-IM6); Timer interrupt (IM7). */
			u32 : 2;
			u32 ch : 1; /* CP0 condition bit */
			u32 : 1;
			u32 sr : 1; /* Indicates whether a soft reset or NMI has occurred */
			u32 ts : 1; /* Indicates that TLB shutdown has occurred (read-only) */
			u32 bev : 1; /* Controls the location of TLB miss and general purpose exception vectors (0: normal; 1: bootstrap) */
			u32 : 1;
			u32 its : 1; /* Enables instruction trace support */
			u32 re : 1; /* Reverse-Endian bit, enables reverse of system endianness in User mode (0: disabled; 1: reversed) */
			u32 fr : 1; /* Enables additional floating-point registers (0: 16 registers; 1: 32 registers) */
			u32 rp : 1; /* Enables low-power operation by reducing the internal clock frequency and the system interface clock frequency to one-quarter speed (0: normal; 1: low power mode) */
			/* The four flags below control the usability of each of the four coprocessor unit numbers (0: unusable; 1: usable)  */
			u32 cu0 : 1; /* COP0 is enabled. If cleared, COP0 instructions throw exceptions */
			u32 cu1 : 1; /* COP1 is enabled. If cleared, COP1 instructions throw exceptions */
			u32 cu2 : 1; /* COP2 is enabled. If cleared, COP2 instructions throw exceptions */
			u32 cu3 : 1; /* COP3 is enabled. If cleared, COP3 instructions throw exceptions */
		} status;

		struct { /* (13) */
			u32 : 2;
			u32 exc_code : 5; /* Exception code field; written to when an exception is signaled. */
			u32 : 1;
			u32 ip : 8; /* Interrupt is pending (0: no interrupt; 1: interrupt pending). Software interrupt (IM0-IM1); External normal interrupts (IM2-IM6); Timer interrupt (IM7). */
			u32 : 12;
			u32 ce : 2; /* Coprocessor unit number referenced when a Coprocessor Unusable exception has occurred. */
			u32 : 1;
			u32 bd : 1; /* Indicates whether the last exception occurred has been executed in a branch delay slot (0: normal; 1: delay slot). */
		} cause;

		u64 epc; /* (14) Contains the address at which processing resumes after an exception has been serviced. */

		struct { /* (15) */
			u32 rev : 8 = 0x22; /* Processor revision number */
			u32 imp : 8 = 0x0B; /* Processor ID number */
			u32 : 16;
		} const pr_id;

		struct { /* (16) */
			u32 k0 : 3; /* Sets coherency algorithm of kseg0 (010 => cache is not used; else => cache is used). */
			u32 cu : 1; /* RFU. However, can be read or written by software. */
		private:
			u32 padding_0 : 11 = 0b11001000110; /* Returns 11001000110 when read. */
		public:
			u32 be : 1; /* Sets endianness (0 => little endian; 1 => big endian (default on cold reset). */
		private:
			u32 padding_1 : 8 = 0b00000110; /* Returns 00000110 when read. */
		public:
			u32 ep : 4; /* Sets transfer data pattern (single/block write request) (0 => D (default on cold reset); 6 => DxxDxx (2 doublewords/six cycles). */
			u32 ec : 3; /* Operating frequency ratio (read-only). */
			u32 : 1; /* Returns 0 when read. */
		} config;

		u32 ll_addr; /* (17); Contains the physical address read by the most recent Load Linked instruction. */

		struct { /* (18) */
			u32 w : 1;
			u32 r : 1;
			u32 : 1;
			u32 p_addr_0 : 29;
		} watch_lo;

		struct { /* (19) */
			u32 p_addr_1 : 4;
			u32 : 28;
		} watch_hi;

		struct { /* (20) Similar to the 'context' register, but used in 64-bit mode. */
			u64 : 4;
			u64 bad_vpn2 : 27;
			u64 r : 2;
			u64 pte_base : 31;
		} x_context;

		u32 cop0_unused_21; /* (21) */
		u32 cop0_unused_22; /* (22) */
		u32 cop0_unused_23; /* (23) */
		u32 cop0_unused_24; /* (24) */
		u32 cop0_unused_25; /* (25) */

		struct { /* (26) */
			u32 diagnostic : 8;
			u32 : 24;
		} parity_error;

		const u32 cache_error = 0; /* (27); Always returns 0 when read. */

		struct { /* (28) */
			u32 : 6;
			u32 pstate : 2; /* Specified the primary cache state (Data cache: 11 => valid; 00 => invalid) (Instruction cache: 10 => valid; 00 => invalid). All others: undefined. */
			u32 ptag : 20; /* Physical address bits 31..12. */
			u32 : 4;
		} tag_lo; /* Holds the primary cache tag for cache initialization, cache diagnostics, or cache error processing. The Tag registers are written by the CACHE and MTC0 instructions. */

		const u32 tag_hi = 0; /* (29); Always returns 0 when read. */

		u64 error_epc; /* (30) */

		u32 cop0_unused_31; /* (31) */

		u64 Get(size_t reg_index) const;
		template<bool raw = false> void Set(size_t reg_index, std::integral auto value);
		void SetRaw(size_t reg_index, auto value) { Set<true>(reg_index, value); }
		void OnWriteToCause();
		void OnWriteToCompare();
		void OnWriteToCount();
		void OnWriteToStatus();
		void OnWriteToWired();
	} cop0;

	/* Used to generate random numbers in the interval [wired, 31], when the 'random' register is read. */
	class RandomGenerator
	{
		std::random_device rd;  // Will be used to obtain a seed for the random number engine
		std::mt19937 gen{ rd() }; // Standard mersenne_twister_engine seeded with rd()
		std::uniform_int_distribution<u32> distrib{ 0, 31 };
	public:
		u32 Generate()
		{
			return distrib(gen);
		}
		void SetRange(auto wired)
		{ /* If WIRED > 31, RANDOM can be [0, 63] */
			if (wired <= 31) distrib = { wired, 31 };
			else             distrib = { 0, 63 };
		}
	} random_generator;

	constexpr std::array cop0_reg_str_repr = {
		"INDEX", "RANDOM", "ENTRY_LO_0", "ENTRY_LO_1", "CONTEXT", "PAGE_MASK", "WIRED", "COP0_7", "BAD_V_ADDR",
		"COUNT", "ENTRY_HI", "COMPARE", "STATUS", "CAUSE", "EPC", "PR_ID", "CONFIG", "LL_ADDR", "WATCH_LO",
		"WATCH_HI", "X_CONTEXT", "COP0_21", "COP0_22", "COP0_23", "COP0_24", "COP0_25", "PARITY_ERROR",
		"COP0_27", "TAG_LO", "TAG_HI", "ERROR_EPC"
	};
	static_assert(cop0_reg_str_repr.size() == 31);
}