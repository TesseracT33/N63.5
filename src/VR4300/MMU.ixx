export module VR4300:MMU;

import <array>;
import <bit>;
import <cassert>;
import <concepts>;
import <functional>;

import Memory;
import MemoryAccess;
import MemoryUtils;
import NumericalTypes;

namespace VR4300
{
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

	template<MemoryAccess::Operation operation> u32 VirtualToPhysicalAddressUserMode32(const u64 virt_addr);
	template<MemoryAccess::Operation operation> u32 VirtualToPhysicalAddressUserMode64(const u64 virt_addr);
	template<MemoryAccess::Operation operation> u32 VirtualToPhysicalAddressSupervisorMode32(const u64 virt_addr);
	template<MemoryAccess::Operation operation> u32 VirtualToPhysicalAddressSupervisorMode64(const u64 virt_addr);
	template<MemoryAccess::Operation operation> u32 VirtualToPhysicalAddressKernelMode32(const u64 virt_addr);
	template<MemoryAccess::Operation operation> u32 VirtualToPhysicalAddressKernelMode64(const u64 virt_addr);
	template<MemoryAccess::Operation operation> u32 VirtualToPhysicalAddressInvalid(const u64 virt_addr);

	typedef u32(*VirtualToPhysicalAddressFun)(const u64);

	VirtualToPhysicalAddressFun active_virtual_to_physical_fun_read = nullptr;
	VirtualToPhysicalAddressFun active_virtual_to_physical_fun_write = nullptr;

	constexpr std::array<std::array<VirtualToPhysicalAddressFun, 2>, 4> virtual_to_physical_fun_read_table = {{
		{VirtualToPhysicalAddressKernelMode32<MemoryAccess::Operation::Read>, VirtualToPhysicalAddressKernelMode64<MemoryAccess::Operation::Read>},
		{VirtualToPhysicalAddressSupervisorMode32<MemoryAccess::Operation::Read>, VirtualToPhysicalAddressSupervisorMode64<MemoryAccess::Operation::Read>},
		{VirtualToPhysicalAddressUserMode32<MemoryAccess::Operation::Read>, VirtualToPhysicalAddressUserMode64<MemoryAccess::Operation::Read>},
		{VirtualToPhysicalAddressInvalid<MemoryAccess::Operation::Read>, VirtualToPhysicalAddressInvalid<MemoryAccess::Operation::Read>}
	}};

	constexpr std::array<std::array<VirtualToPhysicalAddressFun, 2>, 4> virtual_to_physical_fun_write_table = {{
		{VirtualToPhysicalAddressKernelMode32<MemoryAccess::Operation::Write>, VirtualToPhysicalAddressKernelMode64<MemoryAccess::Operation::Write>},
		{VirtualToPhysicalAddressSupervisorMode32<MemoryAccess::Operation::Write>, VirtualToPhysicalAddressSupervisorMode64<MemoryAccess::Operation::Write>},
		{VirtualToPhysicalAddressUserMode32<MemoryAccess::Operation::Write>, VirtualToPhysicalAddressUserMode64<MemoryAccess::Operation::Write>},
		{VirtualToPhysicalAddressInvalid<MemoryAccess::Operation::Write>, VirtualToPhysicalAddressInvalid<MemoryAccess::Operation::Write>}
	}};

	void AssignActiveVirtualToPhysicalFunctions();

	template<MemoryAccess::Operation operation>
	u32 VirtualToPhysicalAddress(const u64 virt_addr);

	template<std::integral T, MemoryAccess::Alignment alignment = MemoryAccess::Alignment::Aligned>
	T ReadVirtual(const u64 virtual_address)
	{
		const std::size_t number_of_bytes =
			MemoryUtils::GetNumberOfBytesToAccess<T, alignment>(virtual_address);
		if constexpr (sizeof T > 1 && alignment == MemoryAccess::Alignment::Aligned)
		{
			if (number_of_bytes != sizeof T)
			{
				//AddressErrorException();
				return 0;
			}
		}

		const u32 physical_address = std::invoke(active_virtual_to_physical_fun_read, virtual_address);
		bool error = false; /* todo: error from translation */
		if (error)
			return 0;

		if constexpr (sizeof T == 1)
			return T(Memory::ReadPhysical<1>(physical_address));
		else if constexpr (alignment == MemoryAccess::Alignment::Aligned)
			return T(Memory::ReadPhysical<sizeof T>(physical_address));
		else
		{
			/* This branch will be worth it; the fact that we can pass the number of bytes to access
			   as a template argument means that, among other things, memcpy will be optimized away
			   to 'mov' instructions, when we later go to actually access data. */
			switch (number_of_bytes)
			{
			case 1: return T(Memory::ReadPhysical<1>(physical_address));
			case 2: return T(Memory::ReadPhysical<2>(physical_address));
			case 3: return T(Memory::ReadPhysical<3>(physical_address));
			case 4: return T(Memory::ReadPhysical<4>(physical_address));
			case 5: return T(Memory::ReadPhysical<5>(physical_address));
			case 6: return T(Memory::ReadPhysical<6>(physical_address));
			case 7: return T(Memory::ReadPhysical<7>(physical_address));
			case 8: return T(Memory::ReadPhysical<8>(physical_address));
			default: assert(false); return T(0);
			}
		}
	}

	template<std::integral T, MemoryAccess::Alignment alignment = MemoryAccess::Alignment::Aligned>
	void WriteVirtual(const u64 virtual_address, const T data)
	{
		const std::size_t number_of_bytes =
			MemoryUtils::GetNumberOfBytesToAccess<T, alignment>(virtual_address);
		if constexpr (sizeof T > 1 && alignment == MemoryAccess::Alignment::Aligned)
		{
			if (number_of_bytes != sizeof T)
			{
				//AddressErrorException();
				return;
			}
		}

		const u32 physical_address = std::invoke(active_virtual_to_physical_fun_write, virtual_address);
		bool error = false; /* todo: error from translation */
		if (error)
			return;

		if constexpr (sizeof T == 1)
			Memory::WritePhysical<1>(physical_address, data);
		else if constexpr (alignment == MemoryAccess::Alignment::Aligned)
			Memory::WritePhysical<sizeof T>(physical_address, data);
		else
		{
			/* This branch will be worth it; the fact that we can pass the number of bytes to access
			   as a template argument means that, among other things, memcpy will be optimized away
			   to 'mov' instructions, when we later go to actually access data. */
			switch (number_of_bytes)
			{
			break; case 1: Memory::WritePhysical<1>(physical_address, data);
			break; case 2: Memory::WritePhysical<2>(physical_address, data);
			break; case 3: Memory::WritePhysical<3>(physical_address, data);
			break; case 4: Memory::WritePhysical<4>(physical_address, data);
			break; case 5: Memory::WritePhysical<5>(physical_address, data);
			break; case 6: Memory::WritePhysical<6>(physical_address, data);
			break; case 7: Memory::WritePhysical<7>(physical_address, data);
			break; case 8: Memory::WritePhysical<8>(physical_address, data);
			break; default: assert(false);
			}
		}
	}
}

