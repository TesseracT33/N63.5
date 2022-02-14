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
			s32 value : 6; /* Index to the TLB entry affected by the TLB Read (TLBR) and TLB Write (TLBW) instructions. */
			s32 : 25;
			s32 p : 1; /* Shows the success (0) or failure (1) of the last TLB Probe (TLBP) instruction executed. */
		} index{};

		struct /* (1) */
		{
			s32 value : 5; /* Decrements every instruction, and specifies the entry in the TLB that is affected by the TLB Write instruction. */
			s32 : 1; /* R/W, but has no function. */
			s32 : 26;
		} random{};

		struct /* (2), (3); Used to rewrite the TLB or to check coincidence of a TLB entry when addresses are converted. */
		{
			s32 g : 1; /* Global. If this bit is set in both EntryLo0 and EntryLo1, then the processor ignores the ASID during TLB lookup. */
			s32 v : 1; /* Valid. If this bit is set, it indicates that the TLB entry is valid; otherwise, a TLBL or TLBS miss occurs. */
			s32 d : 1; /* Dirty. If this bit is set, the page is marked as dirty, and therefore writable. */
			s32 c : 3; /* Specifies the TLB page attribute. */
			s32 pfn : 20; /* Page frame number; the high-order bits of the physical address. */
			s32 : 6;
		} entry_lo_0{}, entry_lo_1{};

		struct /* (4) */
		{
			u64 : 4;
			u64 bad_vpn2 : 19;
			u64 pte_base : 41;
		} context{};

		struct /* (5) */
		{
			s32 : 13;
			s32 mask : 12; /* Sets the page size for each TLB entry. 0 => 4 KB; 3 => 16 KB; 15 => 64 KB; 63 => 256 KB; 255 => 1 MB; 1023 => 4 MB; 4095 => 16 MB. Else, the operation of the TLB is undefined. */
			s32 : 7;
		} page_mask{};

		struct /* (6); Specifies the boundary between the "wired" and "random" entries of the TLB; wired entries cannot be overwritten by a TLBWR operation. */
		{
			s32 value : 6;
			s32 : 26;
		} wired{};

		struct /* (8) */
		{
			u64 value;
		} bad_v_addr{};

		struct /* (9); Increments every other PClock. When equal to the Compare register, interrupt bit IP(7) in the Cause register is set. */
		{
			s32 value; 
		} count{};

		struct /* (10) */
		{
			u64 asid : 8; /* Address space ID field. Lets multiple processes share the TLB; virtual addresses for each process can be shared. */
			u64 padding_of_zeroes : 5; /* named because it needs to be set to zero in the TLBR instruction, as data is copied to here from a TLB entry that is not zero. */
			u64 vpn2 : 27; /* Virtual page number divided by two (maps to two pages). */
			u64 : 22;
			u64 r : 2; /* Region (00 => user; 01 => supervisor; 11 => kernel) used to match virtual address bits 63..62. */
		} entry_hi{}; /* TODO can be 32 bits large with different lengths of VPN2 etc */

		struct CompareRegister /* (11); When equal to the Count register, interrupt bit IP(7) in the Cause register is set. Writes to this register clear said interrupt. */
		{
			s32 value;

			void NotifyCpuAfterWrite();
		} compare{};

		struct StatusRegister /* (12) */
		{
			s32 ie : 1; /* Specifies and indicates global interrupt enable (0: disable interrupts; 1: enable interrupts) */
			s32 exl : 1; /* Specifies and indiciates exception level (0: normal; 1: exception) */
			s32 erl : 1; /* Specifies and indiciates error level (0: normal; 1: error) */
			s32 ksu : 2; /* Specifies and indicates mode bits (00: kernel; 01: supervisor; 10: user) */
			s32 ux : 1; /* Enables 64-bit addressing and operations in User mode (0: 32-bit; 1: 64-bit) */
			s32 sx : 1; /* Enables 64-bit addressing and operations in Supervisor mode (0: 32-bit; 1: 64-bit) */
			s32 kx : 1; /* Enables 64-bit addressing in Kernel mode (0: 32-bit; 1: 64-bit) */
			s32 im : 8; /* Interrupt Mask fields (0: disabled; 1: enabled). Software interrupt (IM0-IM1); External normal interrupts (IM2-IM6); Timer interrupt (IM7). */
			s32 de : 1;
			s32 ce : 1;
			s32 ch : 1;
			s32 : 1;
			s32 sr : 1;
			s32 ts : 1;
			s32 bev : 1;
			s32 : 1;
			s32 its : 1;
			s32 re : 1; /* Reverse-Endian bit, enables reverse of system endianness in User mode (0: disabled; 1: reversed) */
			s32 fr : 1; /* Enables additional floating-point registers (0: 16 registers; 1: 32 registers) */
			s32 rp : 1; /* Enables low-power operation by reducing the internal clock frequency and the system interface clock frequency to one-quarter speed (0: normal; 1: low power mode) */
			/* The four flags below control the usability of each of the four coprocessor unit numbers (0: unusable; 1: usable)  */
			s32 cu0 : 1; /* Ignored by the N64; COP0 is always enabled. */
			s32 cu1 : 1; /* If cleared, all COP1 instructions throw exceptions. */
			s32 cu2 : 1; /* Ignored by the N64; there is no COP2. */
			s32 cu3 : 1; /* Ignored by the N64; there is no COP3. */

			void NotifyCpuAfterWrite();
		} status{};

		struct CauseRegister /* (13) */
		{
			s32 : 2;
			s32 exc_code : 5; /* Exception code field; written to when an exception is signaled. */
			s32 : 1;
			s32 ip : 8; /* Interrupt is pending (0: no interrupt; 1: interrupt pending). Software interrupt (IM0-IM1); External normal interrupts (IM2-IM6); Timer interrupt (IM7). */
			s32 : 12;
			s32 ce : 2; /* Coprocessor unit number referenced when a Coprocessor Unusable exception has occurred. */
			s32 : 1;
			s32 bd : 1; /* Indicates whether the last exception occurred has been executed in a branch delay slot (0: normal; 1: delay slot). */

			void NotifyCpuAfterWrite();
		} cause{};

		struct /* (14) */
		{
			u64 value; /* Contains the address at which processing resumes after an exception has been serviced. */
		} epc{};

		struct /* (15) */
		{
			s32 rev : 8; /* Processor revision number */
			s32 imp : 8 = 0x0B; /* Processor ID number */
			s32 : 16;
		} const pr_id{};

		struct ConfigRegister /* (16) */
		{
			s32 k0 : 3; /* Sets coherency algorithm of kseg0 (010 => cache is not used; else => cache is used). */
			s32 cu : 1; /* RFU. However, can be read or written by software. */
		private:
			s32 padding_0 : 11 = 0b11001000110; /* Returns 11001000110 when read. */
		public:
			s32 be : 1; /* Sets endianness (0 => little endian; 1 => big endian (default on cold reset). */
		private:
			s32 padding_1 : 8 = 0b00000110; /* Returns 00000110 when read. */
		public:
			s32 ep : 4; /* Sets transfer data pattern (single/block write request) (0 => D (default on cold reset); 6 => DxxDxx (2 doublewords/six cycles). */
			s32 ec : 3; /* Operating frequency ratio (read-only). */
			s32 : 1; /* Returns 0 when read. */

			void NotifyCpuAfterWrite();
		} config{};

		struct /* (17); Contains the physical address read by the most recent Load Linked instruction. */
		{
			s32 p_addr;
		} LL_addr{};

		struct /* (18) */
		{
			s32 w : 1;
			s32 r : 1;
			s32 : 1;
			s32 p_addr_0 : 29;
		} watch_lo{};

		struct /* (19) */
		{
			s32 p_addr_1 : 4;
			s32 : 28;
		} watch_hi{};

		struct /* (20) */
		{
			u64 : 4;
			u64 bad_vpn2 : 27;
			u64 r : 2;
			u64 pte_base : 31;
		} x_context{};

		struct /* (26) */
		{
			s32 diagnostic : 8;
			s32 : 24;
		} parity_error{};

		struct /* (27); Always returns 0 when read. */
		{ 
			s32 value = 0;
		} const cache_error;

		struct /* (28) */
		{
			s32 : 6;
			s32 p_state : 2; /* Specified the primary cache state (Data cache: 11 => valid; 00 => invalid) (Instruction cache: 10 => valid; 00 => invalid). All others: undefined. */
			s32 p_tag_lo : 20; /* Physical address bits 31..12. */
			s32 : 4;
		} tag_lo{}; /* Holds the primary cache tag for cache initialization, cache diagnostics, or cache error processing. The Tag registers are written by the CACHE and MTC0 instructions. */

		struct /* (29); Always returns 0 when read. */
		{
			s32 value = 0;
		} const tag_hi;

		struct /* (30) */
		{
			u64 value;
		} error_epc{};

		u64 Get(const size_t register_index) const;
		void Set(const size_t register_index, const u64 value);
	} cop0_reg{};

	/* Floating point control register #31 */
	struct FCR31
	{
		s32 rm : 2;

		s32 flag_I : 1; /* Inexact Operation */
		s32 flag_U : 1; /* Underflow */
		s32 flag_O : 1; /* Overflow */
		s32 flag_Z : 1; /* Division by Zero */
		s32 flag_V : 1; /* Invalid Operation */

		s32 enable_I : 1;
		s32 enable_U : 1;
		s32 enable_O : 1;
		s32 enable_Z : 1;
		s32 enable_V : 1;

		s32 cause_I : 1;
		s32 cause_U : 1;
		s32 cause_O : 1;
		s32 cause_Z : 1;
		s32 cause_V : 1;
		s32 cause_E : 1; /* Unimplemented Operation */

		s32 : 5;
		s32 c : 1;
		s32 fs : 1;
		s32 : 7;

		void Set(const s32 data);
		s32 Get() const;
	} fcr31{};

	/* Floating point control registers. Only #0 and #31 are "valid", and #0 is read-only. */
	struct FPUControl
	{
		s32 Get(const size_t index) const;
		void Set(const size_t index, const s32 data);
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