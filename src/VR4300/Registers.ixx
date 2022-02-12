export module VR4300:Registers;

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
		{ t.NotifyCpuAfterWrite() } -> std::convertible_to<void>;
	};

	template<typename T>
	concept FPU_NumericType =
		std::is_same_v<f32, typename std::remove_cv<T>::type> ||
		std::is_same_v<f64, typename std::remove_cv<T>::type> ||
		std::is_same_v<s32, typename std::remove_cv<T>::type> ||
		std::is_same_v<s64, typename std::remove_cv<T>::type>;

	u64 pc{}; /* Program counter */

	u64 hi_reg{}, lo_reg{}; /* Contain the result of a double-word multiplication or division. */

	bool LL_bit{}; /* Read from / written to by load linked and store conditional instructions. */

	/* CPU general-purpose registers */
	struct GPR
	{
		u64 Get(const size_t index) const { return gpr[index]; }
		void Set(const size_t index, const u64 data) { if (index != 0) gpr[index] = data; }
		u64 operator[](const size_t index) { return gpr[index]; } /* returns by value so that assignments have to made through function "Set". */
	private:
		std::array<u64, 32> gpr{};
	} gpr;

	/* COP0 registers. Used for exception handling and memory management. */
	struct COP0Registers
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

		struct StatusRegister /* (12) */
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
			/* The four flags below control the usability of each of the four coprocessor unit numbers (0: unusable; 1: usable)  */
			u32 CU0 : 1; /* Ignored by the N64; COP0 is always enabled. */
			u32 CU1 : 1; /* If cleared, all COP1 instructions throw exceptions. */
			u32 CU2 : 1; /* Ignored by the N64; there is no COP2. */
			u32 CU3 : 1; /* Ignored by the N64; there is no COP3. */

			void NotifyCpuAfterWrite();
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

		struct ConfigRegister /* (16) */
		{
			u32 K0 : 3; /* Sets coherency algorithm of kseg0 (010 => cache is not used; else => cache is used). */
			u32 CU : 1; /* RFU. However, can be read or written by software. */
			u32 : 11; /* Returns 11001000110 when read. */
			u32 BE : 1; /* Sets endianness (0 => little endian; 1 => big endian (default on cold reset). */
			u32 : 8; /* Returns 00000110 when read. */
			u32 EP : 4; /* Sets transfer data pattern (single/block write request) (0 => D (default on cold reset); 6 => DxxDxx (2 doublewords/six cycles). */
			u32 EC : 3; /* Operating frequency ratio (read-only). */
			u32 : 1; /* Returns 0 when read. */

			void NotifyCpuAfterWrite();
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


		u64 Get(const size_t register_index) const;
		void Set(const size_t register_index, const u64 value);
	} cop0_reg{};

	/* Floating point control register #31 */
	struct FCR31
	{
		void Set(const u32 data);
		u32 Get() const;

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
	} fcr31{};

	/* Floating point control registers. */
	struct FPUControl
	{
		u32 Get(const size_t index) const;
		void Set(const size_t index, const u32 data);
	} fpu_control;

	/* General-purpose floating point registers. */
	struct FGR
	{
		template<typename FPU_NumericType>
		FPU_NumericType Get(const size_t index) const;

		template<typename FPU_NumericType>
		void Set(const size_t index, const FPU_NumericType data);

	private:
		std::array<s64, 32> fgr{};
	} fgr;
}