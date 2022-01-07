export module VR4300:MMU;

import <array>;
import <concepts>;

import NumericalTypes;

namespace VR4300
{
	enum class ReadFromNextBoundary
	{
		Yes, No
	};

	enum class WriteToNextBoundary
	{
		Yes, No
	};

	struct TLB_Entry
	{
		struct
		{
			u64 : 1;
			u64 V : 1;
			u64 D : 1;
			u64 C : 3;
			u64 PFN : 20;
			u64 : 38;
		} lo_0{}, lo_1{};

		u64 ASID : 8;
		u64 : 4;
		u64 G : 1;
		u64 VPN2 : 27;
		u64 : 22;
		u64 R : 2;

		u64 : 13;
		u64 MASK : 12;
		u64 : 39;
	};

	std::array<TLB_Entry, 32> TLB_entries{};

	u64 ReadVirtualUserMode32(const u64 virt_addr);
	u64 ReadVirtualUserMode64(const u64 virt_addr);
	u64 ReadVirtualSupervisorMode32(const u64 virt_addr);
	u64 ReadVirtualSupervisorMode64(const u64 virt_addr);
	u64 ReadVirtualKernelMode32(const u64 virt_addr);
	u64 ReadVirtualKernelMode64(const u64 virt_addr);
	u64 VirtualToPhysicalAddress(const u32 virt_addr);

	template<std::integral T, ReadFromNextBoundary read_from_next_boundary = ReadFromNextBoundary::No>
	T cpu_read_mem(u64 address)
	{
		return T();
	}

	template<std::integral T, WriteToNextBoundary write_to_next_boundary = WriteToNextBoundary::No>
	void cpu_write_mem(u64 address, T data)
	{

	}
}

