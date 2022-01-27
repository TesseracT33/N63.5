export module VR4300:MMU;

import <array>;
import <bit>;
import <cassert>;
import <concepts>;
import <functional>;

import Memory;

import NumericalTypes;

namespace VR4300
{
	enum class MemoryAccessOperation { Read, Write };

	enum class MemoryAccessAlignment { Aligned, Unaligned };

	struct TLB_Entry
	{
		struct
		{
			u64     :  1;
			u64 V   :  1; /* Valid. Is this bit is set, it indicates that the TLB entry is valid; otherwise, a TLBL or TLBS miss occurs. */
			u64 D   :  1; /* Dirty. If this bit is set, the page is marked as writeable. */
			u64 C   :  3; /* Specifies the TLB page attribute (2 => do not access cache; else => access cache). */
			u64 PFN : 20; /* Page frame number; the high-order bits of the physical address. */
			u64     : 38;
		} lo_0{}, lo_1{};

		u64 ASID :  8; /* Address space ID field. Lets multiple processes share the TLB; virtual addresses for each process can be shared. */
		u64      :  4;
		u64 G    :  1; /* Global. If this bit is set, the processor ignores the ASID during TLB lookup. */
		u64 VPN2 : 27; /* Virtual page number divided by two (maps to two pages). */
		u64      : 22;
		u64 R    :  2; /* Region (00 => user; 01 => supervisor; 11 => kernel) used to match virtual address bits 63..62. */

		u64      : 13;
		u64 MASK : 12; /* Determines the virtual page size of the corresponding entry. */
		u64      : 39;
	};

	std::array<TLB_Entry, 32> TLB_entries{};

	template<MemoryAccessOperation operation> u32 VirtualToPhysicalAddressUserMode32(const u64 virt_addr);
	template<MemoryAccessOperation operation> u32 VirtualToPhysicalAddressUserMode64(const u64 virt_addr);
	template<MemoryAccessOperation operation> u32 VirtualToPhysicalAddressSupervisorMode32(const u64 virt_addr);
	template<MemoryAccessOperation operation> u32 VirtualToPhysicalAddressSupervisorMode64(const u64 virt_addr);
	template<MemoryAccessOperation operation> u32 VirtualToPhysicalAddressKernelMode32(const u64 virt_addr);
	template<MemoryAccessOperation operation> u32 VirtualToPhysicalAddressKernelMode64(const u64 virt_addr);
	template<MemoryAccessOperation operation> u32 VirtualToPhysicalAddressInvalid(const u64 virt_addr);

	typedef u32(*virtual_to_physical_address_fun_t)(const u64);

	virtual_to_physical_address_fun_t active_virtual_to_physical_fun_read = nullptr;
	virtual_to_physical_address_fun_t active_virtual_to_physical_fun_write = nullptr;

	constexpr std::array<std::array<virtual_to_physical_address_fun_t, 2>, 4> virtual_to_physical_fun_read_table = {{
		{VirtualToPhysicalAddressKernelMode32<MemoryAccessOperation::Read>, VirtualToPhysicalAddressKernelMode64<MemoryAccessOperation::Read>},
		{VirtualToPhysicalAddressSupervisorMode32<MemoryAccessOperation::Read>, VirtualToPhysicalAddressSupervisorMode64<MemoryAccessOperation::Read>},
		{VirtualToPhysicalAddressUserMode32<MemoryAccessOperation::Read>, VirtualToPhysicalAddressUserMode64<MemoryAccessOperation::Read>},
		{VirtualToPhysicalAddressInvalid<MemoryAccessOperation::Read>, VirtualToPhysicalAddressInvalid<MemoryAccessOperation::Read>}
	}};

	constexpr std::array<std::array<virtual_to_physical_address_fun_t, 2>, 4> virtual_to_physical_fun_write_table = {{
		{VirtualToPhysicalAddressKernelMode32<MemoryAccessOperation::Write>, VirtualToPhysicalAddressKernelMode64<MemoryAccessOperation::Write>},
		{VirtualToPhysicalAddressSupervisorMode32<MemoryAccessOperation::Write>, VirtualToPhysicalAddressSupervisorMode64<MemoryAccessOperation::Write>},
		{VirtualToPhysicalAddressUserMode32<MemoryAccessOperation::Write>, VirtualToPhysicalAddressUserMode64<MemoryAccessOperation::Write>},
		{VirtualToPhysicalAddressInvalid<MemoryAccessOperation::Write>, VirtualToPhysicalAddressInvalid<MemoryAccessOperation::Write>}
	}};

	void AssignActiveVirtualToPhysicalFunctions();

	template<MemoryAccessOperation operation>
	u32 VirtualToPhysicalAddress(const u64 virt_addr);

	template<std::integral T, MemoryAccessAlignment alignment = MemoryAccessAlignment::Aligned>
	T cpu_read_mem(const u64 address)
	{
		if constexpr (alignment == MemoryAccessAlignment::Aligned)
		{
			if constexpr (sizeof T > 1)
			{
				constexpr auto alignment_mask = sizeof T - 1;
				if (address & alignment_mask)
				{
					//AddressErrorException();
					return 0;
				}
			}
		}

		const u32 physical_address = std::invoke(active_virtual_to_physical_fun_read, address);
		bool error = false; /* error from translation */
		if (error)
			return 0;

		const T value = Memory::ReadPhysical<T>(physical_address);
		return value;
	}

	template<std::integral T, MemoryAccessAlignment alignment = MemoryAccessAlignment::Aligned>
	void cpu_write_mem(const u64 address, const T data)
	{

	}
}

