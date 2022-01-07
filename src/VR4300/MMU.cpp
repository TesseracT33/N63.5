module VR4300:MMU;

import :Exceptions;

import Memory;

namespace VR4300
{
	u64 ReadVirtualUserMode32(const u64 virt_addr)
	{
		if (virt_addr >= 0x80000000)
		{
			AddressErrorException();
			return 0;
		}
		const u32 phys_addr = VirtualToPhysicalAddress(virt_addr);
		return Memory::ReadPhysical(phys_addr);
	}

	u64 ReadVirtualUserMode64(const u64 virt_addr)
	{
		if (virt_addr >= 0x100'00000000)
		{
			AddressErrorException();
			return 0;
		}
		const u32 phys_addr = VirtualToPhysicalAddress(virt_addr);
		return Memory::ReadPhysical(phys_addr);
	}

	u64 ReadVirtualSupervisorMode32(const u64 virt_addr)
	{
		switch (virt_addr)
		{
		case 0x0: case 0x1: case 0x2: case 0x3: case 0x4: case 0x5: case 0x6: case 0x7: case 0xC: case 0xD:
			break;

		default:
			AddressErrorException();
		}
		return u32();
	}

	u64 ReadVirtualSupervisorMode64(const u64 virt_addr)
	{
		switch (virt_addr)
		{
		case 0x0: case 0x1: case 0x2: case 0x3: case 0x4: case 0x5: case 0x6: case 0x7: case 0xC: case 0xD:
			break;

		default:
			AddressErrorException();
		}
		return u32();
	}

	u64 ReadVirtualKernelMode32(const u64 virt_addr)
	{
		switch (virt_addr >> 28)
		{
		case 0x8: case 0x9: /* 0x80000000-0x9FFFFFFF -- KSEG0. Kernel segment 0. Unmapped, cacheable. */
			break;

		case 0xA: case 0xB: /* 0xA0000000-0xBFFFFFFF -- KSEG1. Kernel segment 1. Unmapped, uncached. */
			break;

		case 0xC: case 0xD: /* 0xC0000000-0xDFFFFFFF -- KSSEG. Kernel supervisor segment. TLB mapped. */
			break;

		case 0xE: case 0xF: /* 0xE0000000-0xFFFFFFFF -- KSEG3. Kernel segment 3. TLB mapped. */
			break;

		default: /* 0x00000000-0x7FFFFFFF -- KUSEG. User segment. TLB mapped. */
			break;
		}
		return u32();
	}

	u64 ReadVirtualKernelMode64(const u64 virt_addr)
	{
		return u32();
	}

	u64 VirtualToPhysicalAddress(const u32 virtual_addr)
	{
		return u64();
	}
}