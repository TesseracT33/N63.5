export module VR4300:Registers;

import NumericalTypes;

import <array>;
import <bit>;
import <cfenv>;
import <concepts>;
import <limits>;
import <random>;
import <type_traits>;

namespace VR4300
{
	/* For (COP0) registers (structs) that, once they have been written to, need to tell the rest of the cpu about it. */
	template<typename T>
	concept reg_notifies_cpu_on_write = requires(T t) {
		{ t.NotifyCpuAfterWrite() } -> std::convertible_to<void>;
	};

	template<typename T>
	concept FPUNumericType =
		std::is_same_v<f32, typename std::remove_cv<T>::type> ||
		std::is_same_v<f64, typename std::remove_cv<T>::type> ||
		std::is_same_v<s32, typename std::remove_cv<T>::type> ||
		std::is_same_v<s64, typename std::remove_cv<T>::type>;


	void InitializeRegisters();


	u64 pc; /* Program counter */

	u64 hi_reg, lo_reg; /* Contain the result of a double-word multiplication or division. */

	bool ll_bit; /* Read from / written to by load linked and store conditional instructions. */

	/* CPU general-purpose registers */
	struct GPR
	{
		s64 Get(size_t index) const {
			return gpr[index];
		}
		void Set(size_t index, s64 data) {
			/* gpr[0] is hardwired to 0. Prefer setting it to zero every time over a branch checking if 'index' is zero. */
			gpr[index] = data;
			gpr[0] = 0;
		}
		s64 operator[](size_t index) /* returns by value so that assignments have to made through function "Set". */
		{
			return gpr[index];
		} 
	private:
		std::array<s64, 32> gpr{};
	} gpr;

	/* COP0 registers. Used for exception handling and memory management. */
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
	constexpr uint cop0_index_tag_lo = 28;
	constexpr uint cop0_index_tag_hi = 29;
	constexpr uint cop0_index_error_epc = 30;


	struct COP0Registers
	{
		struct /* (0) */
		{
			u32 value : 6; /* Index to the TLB entry affected by the TLB Read (TLBR) and TLB Write (TLBW) instructions. */
			u32 : 25;
			u32 p : 1; /* Shows the success (0) or failure (1) of the last TLB Probe (TLBP) instruction executed. */
		} index{};

		struct /* (1) */
		{
			u32 value : 5; /* Decrements every instruction, and specifies the entry in the TLB that is affected by the TLB Write instruction. */
			u32 : 1; /* R/W, but has no function. */
			u32 : 26;
		} random{};

		struct /* (2), (3); Used to rewrite the TLB or to check coincidence of a TLB entry when addresses are converted. */
		{
			u32 g : 1; /* Global. If this bit is set in both EntryLo0 and EntryLo1, then the processor ignores the ASID during TLB lookup. */
			u32 v : 1; /* Valid. If this bit is set, it indicates that the TLB entry is valid; otherwise, a TLBL or TLBS miss occurs. */
			u32 d : 1; /* Dirty. If this bit is set, the page is marked as dirty, and therefore writable. */
			u32 c : 3; /* Specifies the TLB page attribute. */
			u32 pfn : 20; /* Page frame number; the high-order bits of the physical address. */
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
			u32 value : 12; /* Sets the page size for each TLB entry. 0 => 4 KB; 3 => 16 KB; 15 => 64 KB; 63 => 256 KB; 255 => 1 MB; 1023 => 4 MB; 4095 => 16 MB. Else, the operation of the TLB is undefined. */
			u32 : 7;
		} page_mask{};

		struct WiredRegister /* (6); Specifies the boundary between the "wired" and "random" entries of the TLB; wired entries cannot be overwritten by a TLBWR operation. */
		{
			u32 value : 6;
			u32 : 26;

			void NotifyCpuAfterWrite();
		} wired{};

		struct /* (8) */
		{
			u64 value;
		} bad_v_addr{};

		struct /* (9); Increments every other PClock. When equal to the Compare register, interrupt bit IP(7) in the Cause register is set. */
		{ /* On real HW, the register is 32 bits. Here, we make it 64 bits and increment it every PCycle instead of every other PCycle.
			 When we read from it, we shift it right one bit and then return it. When we write to it, we set it to the data shifted left one bit. */
			u64 value;
		} count{};

		struct /* (10) */
		{
			u64 asid : 8; /* Address space ID field. Lets multiple processes share the TLB; virtual addresses for each process can be shared. */
			u64 padding_of_zeroes : 5; /* named because it needs to be set to zero in the TLBR instruction, as data is copied to here from a TLB entry that is not zero. */
			u64 vpn2 : 27; /* Virtual page number divided by two (maps to two pages). */
			u64 : 22;
			u64 r : 2; /* Region (00 => user; 01 => supervisor; 11 => kernel) used to match virtual address bits 63..62. */
		} entry_hi{};

		struct CompareRegister /* (11); When equal to the Count register, interrupt bit IP(7) in the Cause register is set. Writes to this register clear said interrupt. */
		{ /* On real HW, this register is 32 bits. Here, we make it 64 bits. See the description of the 'Count' register. */
			u64 value;

			void NotifyCpuAfterWrite();
		} compare{};

		struct StatusRegister /* (12) */
		{
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
			u32 cu0 : 1; /* Ignored by the N64; COP0 is always enabled. */
			u32 cu1 : 1; /* If cleared, all COP1 instructions throw exceptions. */
			u32 cu2 : 1; /* Ignored by the N64; there is no COP2. */
			u32 cu3 : 1; /* Ignored by the N64; there is no COP3. */

			void NotifyCpuAfterWrite();
		} status{};

		struct CauseRegister /* (13) */
		{
			u32 : 2;
			u32 exc_code : 5; /* Exception code field; written to when an exception is signaled. */
			u32 : 1;
			u32 ip : 8; /* Interrupt is pending (0: no interrupt; 1: interrupt pending). Software interrupt (IM0-IM1); External normal interrupts (IM2-IM6); Timer interrupt (IM7). */
			u32 : 12;
			u32 ce : 2; /* Coprocessor unit number referenced when a Coprocessor Unusable exception has occurred. */
			u32 : 1;
			u32 bd : 1; /* Indicates whether the last exception occurred has been executed in a branch delay slot (0: normal; 1: delay slot). */

			void NotifyCpuAfterWrite();
		} cause{};

		struct /* (14) */
		{
			u64 value; /* Contains the address at which processing resumes after an exception has been serviced. */
		} epc{};

		struct /* (15) */
		{
			u32 rev : 8 = 0x22; /* Processor revision number */
			u32 imp : 8 = 0x0B; /* Processor ID number */
			u32 : 16;
		} const pr_id{};

		struct ConfigRegister /* (16) */
		{
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
		} config{};

		struct /* (17); Contains the physical address read by the most recent Load Linked instruction. */
		{
			u32 p_addr;
		} ll_addr{};

		struct /* (18) */
		{
			u32 w : 1;
			u32 r : 1;
			u32 : 1;
			u32 p_addr_0 : 29;
		} watch_lo{};

		struct /* (19) */
		{
			u32 p_addr_1 : 4;
			u32 : 28;
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
			u32 diagnostic : 8;
			u32 : 24;
		} parity_error{};

		struct /* (27); Always returns 0 when read. */
		{
			u32 value = 0;
		} const cache_error;

		struct /* (28) */
		{
			u32 : 6;
			u32 p_state : 2; /* Specified the primary cache state (Data cache: 11 => valid; 00 => invalid) (Instruction cache: 10 => valid; 00 => invalid). All others: undefined. */
			u32 p_tag_lo : 20; /* Physical address bits 31..12. */
			u32 : 4;
		} tag_lo{}; /* Holds the primary cache tag for cache initialization, cache diagnostics, or cache error processing. The Tag registers are written by the CACHE and MTC0 instructions. */

		struct /* (29); Always returns 0 when read. */
		{
			u32 value = 0;
		} const tag_hi;

		struct /* (30) */
		{
			u64 value;
		} error_epc{};

		u64 Get(size_t register_index) const;
		void Set(size_t register_index, u64 value);
		void SetRaw(size_t register_index, u64 value);
	} cop0_reg{};

	/* Floating point control register #31 */
	struct FCR31
	{
		u32 rm : 2; /* Rounding mode */

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
		u32 c : 1; /* Condition bit; set/cleared by the Compare instruction (or CTC1). */
		u32 fs : 1; /* TODO */
		u32 : 7;

		void Set(u32 data);
		u32 Get() const;
	} fcr31{};

	/* Floating point control registers. Only #0 and #31 are "valid", and #0 is read-only. */
	struct FPUControl
	{
		u32 Get(size_t index) const;
		void Set(size_t index, u32 data);
	} fpu_control;

	/* General-purpose floating point registers. */
	struct FGR
	{
		template<typename FPUNumericType>
		FPUNumericType Get(size_t index) const;

		template<typename FPUNumericType>
		void Set(size_t index, FPUNumericType data);

	private:
		std::array<s64, 32> fpr{};
	} fpr;


	/* Used to generate random numbers in the interval [wired, 31], when the 'random' register is read. */
	class RandomGenerator
	{
		std::random_device rd;  // Will be used to obtain a seed for the random number engine
		std::mt19937 gen{ rd() }; // Standard mersenne_twister_engine seeded with rd()
		std::uniform_int_distribution<u32> distrib{ 0, 0x1F };
	public:
		u32 Generate()
		{
			return distrib(gen);
		}
		void SetLowerBound(const auto min)
		{
			distrib = { min, 0x1F };
		}
	} random_generator{};
}