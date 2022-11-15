export module VR4300:MMU;

import :COP0;
import :Operation;

import Memory;
import Util;

import <array>;
import <bit>;
import <cassert>;
import <concepts>;
import <type_traits>;

namespace VR4300
{
	/* Used for logging. Set when memory is read during an instruction fetch. */
	export u32 last_instr_fetch_phys_addr;

	using VirtualToPhysicalAddressFun = u32(*)(u64 /* in: v_addr */, bool& /* out: cached area? */);

	enum class AddressingMode {
		_32bit, _64bit
	} addressing_mode;

	struct TlbEntry {
		void Read() const;
		void Write();

		Cop0Registers::EntryLo entry_lo[2];
		Cop0Registers::EntryHi entry_hi;
		u32 page_mask; /* Determines the virtual page size of the corresponding entry. */

		u64 vpn2_addr_mask; /* Used to extract the VPN2 from a virtual address, given page_mask. */
		u64 vpn2_compare; /* entry_hi.vpn2, but shifted left according to page_mask. */
		u32 offset_addr_mask;  /* Used to extract the offset from a virtual address, given page_mask, i.e., the bits lower than those part of the VPN. */
	};

	template<Memory::Operation> u32 VirtualToPhysicalAddressUserMode32(u64, bool&);
	template<Memory::Operation> u32 VirtualToPhysicalAddressUserMode64(u64, bool&);
	template<Memory::Operation> u32 VirtualToPhysicalAddressSupervisorMode32(u64, bool&);
	template<Memory::Operation> u32 VirtualToPhysicalAddressSupervisorMode64(u64, bool&);
	template<Memory::Operation> u32 VirtualToPhysicalAddressKernelMode32(u64, bool&);
	template<Memory::Operation> u32 VirtualToPhysicalAddressKernelMode64(u64, bool&);
	template<Memory::Operation> u32 VirtualToPhysicalAddressTlb(u64);

	template<std::signed_integral Int, Alignment alignment = Alignment::Aligned, Memory::Operation operation = Memory::Operation::Read>
	Int ReadVirtual(u64 virtual_address);

	template<Alignment alignment = Alignment::Aligned>
	void WriteVirtual(u64 virtual_address, std::signed_integral auto data);

	u32 FetchInstruction(u64 virtual_address);
	void InitializeMMU();
	void SetActiveVirtualToPhysicalFunctions();

	u32 last_physical_address_on_load;

	std::array<TlbEntry, 32> tlb_entries;

	VirtualToPhysicalAddressFun active_virtual_to_physical_fun_read;
	VirtualToPhysicalAddressFun active_virtual_to_physical_fun_write;
}