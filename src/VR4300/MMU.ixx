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

	struct TLB_Entry
	{
		/* EntryLo0, EntryLo1 */
		struct
		{
			u64     :  1;
			u64 v   :  1; /* Valid. Is this bit is set, it indicates that the TLB entry is valid; otherwise, a TLBL or TLBS miss occurs. */
			u64 d   :  1; /* Dirty. If this bit is set, the page is marked as writeable. */
			u64 c   :  3; /* Specifies the TLB page attribute (2 => do not access cache; else => access cache). */
			u64 pfn : 20; /* Page frame number; the high-order bits of the physical address. */
			u64     : 38;
		} lo_0{}, lo_1{};
		/* EntryHi */
		u64 asid :  8; /* Address space ID field. Lets multiple processes share the TLB; virtual addresses for each process can be shared. */
		u64      :  4;
		u64 g    :  1; /* Global. If this bit is set, the processor ignores the ASID during TLB lookup. */
		u64 vpn2 : 27; /* Virtual page number divided by two (maps to two pages). */
		u64      : 22;
		u64 r    :  2; /* Region (00 => user; 01 => supervisor; 11 => kernel) used to match virtual address bits 63..62. */
		/* PageMask */
		u64      : 13;
		u64 mask : 12; /* Determines the virtual page size of the corresponding entry. */
		u64      : 39;
	};

	std::array<TLB_Entry, 32> TLB_entries{};

	template<MemoryAccess::Operation operation> u32 VirtualToPhysicalAddressUserMode32(u64);
	template<MemoryAccess::Operation operation> u32 VirtualToPhysicalAddressUserMode64(u64);
	template<MemoryAccess::Operation operation> u32 VirtualToPhysicalAddressSupervisorMode32(u64);
	template<MemoryAccess::Operation operation> u32 VirtualToPhysicalAddressSupervisorMode64(u64);
	template<MemoryAccess::Operation operation> u32 VirtualToPhysicalAddressKernelMode32(u64);
	template<MemoryAccess::Operation operation> u32 VirtualToPhysicalAddressKernelMode64(u64);

	template<MemoryAccess::Operation operation>
	u32 VirtualToPhysicalAddress(const u64 virt_addr);

	void SetActiveVirtualToPhysicalFunctions();

	template<
		std::integral Int,
		MemoryAccess::Alignment alignment = MemoryAccess::Alignment::Aligned,
		MemoryAccess::Operation operation = MemoryAccess::Operation::Read>
	Int ReadVirtual(u64 virtual_address);

	template<std::integral Int, MemoryAccess::Alignment alignment = MemoryAccess::Alignment::Aligned>
	void WriteVirtual(const u64 virtual_address, const Int data);

	u32 FetchInstruction(u64 virtual_address);

	VirtualToPhysicalAddressFun active_virtual_to_physical_fun_read = nullptr;
	VirtualToPhysicalAddressFun active_virtual_to_physical_fun_write = nullptr;

	struct TLB_Failure
	{
		u64 bad_virt_addr;
		u32 bad_vpn2;
		u32 bad_asid;

	} tlb_failure;

	/* Used for logging. Set when memory is read during an instruction fetch. */
	u32 last_physical_address_on_instr_fetch;
}

