export module VR4300:MMU;

import <array>;
import <bit>;
import <cassert>;
import <concepts>;
import <functional>;
import <type_traits>;

import MemoryAccess;
import NumericalTypes;

namespace VR4300
{
	typedef u32(*VirtualToPhysicalAddressFun)(u64); /* Used in both 32-bit and 64-bit mode */

	enum class AddressingMode { _32bit, _64bit } addressing_mode;

	struct TLB_Entry
	{
		struct
		{
			u32     :  1;
			u32 v   :  1; /* Valid. Is this bit is set, it indicates that the TLB entry is valid; otherwise, a TLBL or TLBS miss occurs. */
			u32 d   :  1; /* Dirty. If this bit is set, the page is marked as writeable. */
			u32 c   :  3; /* Specifies the TLB page attribute (2 => do not access cache; else => access cache). */
			u32 pfn : 20; /* Page frame number; the high-order bits of the physical address. */
			u32     :  6;
		} entry_lo[2]{};

		struct
		{
			u64 asid :  8; /* Address space ID field. Lets multiple processes share the TLB; virtual addresses for each process can be shared. */
			u64      :  4;
			u64 g    :  1; /* Global. If this bit is set, the processor ignores the ASID during TLB lookup. */
			u64 vpn2 : 27; /* Virtual page number divided by two (maps to two pages). */
			u64      : 22;
			u64 r    :  2; /* Region (00 => user; 01 => supervisor; 11 => kernel) used to match virtual address bits 63..62. */
		} entry_hi{};

		struct
		{
			u32       : 13;
			u32 value : 12; /* Determines the virtual page size of the corresponding entry. */
			u32       :  7;
		} page_mask{};

		u64 vpn2_shifted;
		u64 address_vpn2_mask;
		u64 address_offset_mask;
		u64 address_vpn_even_odd_mask;
	};

	struct TLB_Failure
	{
		u64 bad_virt_addr;
		u64 bad_vpn2;
		u64 bad_asid;
	} tlb_failure;

	/* Given a TLB entry page size, how many bits is the virtual/physical address offset? */
	constexpr std::array page_size_to_addr_offset_bit_length = [] {
		std::array<u8, 4096> table{};
		for (int i = 0; i < table.size(); i++) {
			table[i] = [&] {
				if (i == 0) return 12;
				if (i <= 3) return 14;
				if (i <= 15) return 16;
				if (i <= 63) return 18;
				if (i <= 255) return 20;
				if (i <= 1023) return 22;
				return 24;
			}();
		}
		return table;
	}();

	void InitializeMMU();

	template<MemoryAccess::Operation operation> u32 VirtualToPhysicalAddressUserMode32(u64);
	template<MemoryAccess::Operation operation> u32 VirtualToPhysicalAddressUserMode64(u64);
	template<MemoryAccess::Operation operation> u32 VirtualToPhysicalAddressSupervisorMode32(u64);
	template<MemoryAccess::Operation operation> u32 VirtualToPhysicalAddressSupervisorMode64(u64);
	template<MemoryAccess::Operation operation> u32 VirtualToPhysicalAddressKernelMode32(u64);
	template<MemoryAccess::Operation operation> u32 VirtualToPhysicalAddressKernelMode64(u64);

	template<MemoryAccess::Operation operation>
	u32 VirtualToPhysicalAddress(u64 virt_addr);

	void SetActiveVirtualToPhysicalFunctions();

	template<
		std::integral Int,
		MemoryAccess::Alignment alignment = MemoryAccess::Alignment::Aligned,
		MemoryAccess::Operation operation = MemoryAccess::Operation::Read>
	Int ReadVirtual(u64 virtual_address);

	template<std::integral Int, MemoryAccess::Alignment alignment = MemoryAccess::Alignment::Aligned>
	void WriteVirtual(const u64 virtual_address, const Int data);

	u32 FetchInstruction(u64 virtual_address);

	std::array<TLB_Entry, 32> tlb_entries{};

	VirtualToPhysicalAddressFun active_virtual_to_physical_fun_read = nullptr;
	VirtualToPhysicalAddressFun active_virtual_to_physical_fun_write = nullptr;

	bool store_physical_address_on_load = false; /* For LL and LLD instructions; the phys addr is stored to the LLAddr register. */

	/* Used for logging. Set when memory is read during an instruction fetch. */
	export u32 last_instr_fetch_phys_addr;
}

