module VR4300:MMU;

import :COP0;
import :Exceptions;

import Memory;

namespace VR4300
{
	void AssignActiveVirtualToPhysicalFunctions()
	{
		active_virtual_to_physical_fun_read = virtual_to_physical_fun_read_table[CP0_reg.status.KSU][CP0_reg.status.UX];
		active_virtual_to_physical_fun_write = virtual_to_physical_fun_write_table[CP0_reg.status.KSU][CP0_reg.status.UX];
	}

	template<MemoryAccessOperation operation>
	u32 VirtualToPhysicalAddressUserMode32(const u64 virt_addr)
	{
		if (virt_addr < 0x80000000)
		{
			return VirtualToPhysicalAddress<operation>(virt_addr);
		}
		else
		{
			AddressErrorException();
			return 0;
		}
	}

	template<MemoryAccessOperation operation>
	u32 VirtualToPhysicalAddressUserMode64(const u64 virt_addr)
	{
		if (virt_addr < 0x100'00000000)
		{
			return VirtualToPhysicalAddress<operation>(virt_addr);
		}
		else
		{
			AddressErrorException();
			return 0;
		}
	}

	template<MemoryAccessOperation operation>
	u32 VirtualToPhysicalAddressSupervisorMode32(const u64 virt_addr)
	{
		switch (virt_addr >> 28)
		{
		case 0x0: case 0x1: case 0x2: case 0x3: case 0x4: case 0x5: case 0x6: case 0x7: case 0xC: case 0xD:
			return VirtualToPhysicalAddress<operation>(virt_addr);

		default:
			AddressErrorException();
			return 0;
		}
	}

	template<MemoryAccessOperation operation>
	u32 VirtualToPhysicalAddressSupervisorMode64(const u64 virt_addr)
	{
		switch (virt_addr >> 60)
		{
		case 0x0:
			if (virt_addr <= 0x0000'00FF'FFFF'FFFF) 
			{
				return VirtualToPhysicalAddress<operation>(virt_addr);
			}
			else
			{
				AddressErrorException();
				return 0;
			}

		case 0x4:
			if (virt_addr <= 0x4000'00FF'FFFF'FFFF) 
			{
				return VirtualToPhysicalAddress<operation>(virt_addr);
			}
			else
			{
				AddressErrorException();
				return 0;
			}

		case 0xF:
			if ((virt_addr & 0xFFFF'FFFF'E000'0000) == 0xFFFF'FFFF'C000'0000)  /* $FFFF'FFFF'C0000'0000 -- $FFFF'FFFF'DFFF'FFFF */
			{
				return VirtualToPhysicalAddress<operation>(virt_addr);
			}
			else /* $F000'0000'0000'0000 -- $FFFF'FFFF'BFFF'FFFF; $FFFF'FFFF'E000'0000 -- $FFFF'FFFF'FFFF'FFFF */
			{
				AddressErrorException();
				return 0;
			}

		default:
			AddressErrorException();
			return 0;
		}
	}

	template<MemoryAccessOperation operation>
	u32 VirtualToPhysicalAddressKernelMode32(const u64 virt_addr)
	{
		if ((virt_addr & 0xC000'0000) == 0x8000'0000) /* TODO: currently, caching is not emulated; this does not take into account uncached vs. cacheable segments */
		{
			return virt_addr & 0x1FFF'FFFF; /* TLB unmapped */
		}
		else
		{
			return VirtualToPhysicalAddress<operation>(virt_addr); /* TLB mapped */
		}
	}

	template<MemoryAccessOperation operation>
	u32 VirtualToPhysicalAddressKernelMode64(const u64 virt_addr)
	{
		switch (virt_addr >> 60)
		{
		case 0x0:
			if (virt_addr <= 0x0000'00FF'FFFF'FFFF) 
			{
				return VirtualToPhysicalAddress<operation>(virt_addr);
			}
			else
			{
				AddressErrorException();
				return 0;
			}

		case 0x4:
			if (virt_addr <= 0x4000'00FF'FFFF'FFFF) 
			{
				return VirtualToPhysicalAddress<operation>(virt_addr);
			}
			else
			{
				AddressErrorException();
				return 0;
			}

		case 0x8: case 0x9: case 0xA: case 0xB:
			if ((virt_addr & 0x07FF'FFFF'0000'0000) == 0) 
			{ /* tlb unmapped */
				return virt_addr & 0xFFFF'FFFF;
				/* TODO: caching is currently not emulated. The below decoding would take care of that. */
				//if ((virt_addr & 0x9800'0000'0000'0000) == 0x9000'0000'0000'0000)
				//	return 0; /* uncached*/
				//else
				//	return 0; /* cacheable */
			}
			else
			{
				AddressErrorException();
				return 0;
			}

		case 0xC:
			if (virt_addr <= 0xC000'00FF'7FFF'FFFF) 
			{
				return VirtualToPhysicalAddress<operation>(virt_addr);
			}
			else
			{
				AddressErrorException();
				return 0;
			}

		case 0xF:
			if (virt_addr >= 0xFFFF'FFFF'C000'0000)
			{
				return VirtualToPhysicalAddress<operation>(virt_addr); /* tlb mapped */
			}
			/* TODO: currently, cacheing is not emulated. */
			//else if (virt_addr >= 0xFFFF'FFFF'A000'0000)
			//{
			//	return 0; /* unmapped, uncacheable */
			//}
			else if (virt_addr >= 0xFFFF'FFFF'8000'0000)
			{
				return virt_addr & 0x1FFF'FFFF; /* unmapped, cacheable */
			}
			else [[unlikely]]
			{
				AddressErrorException();
				return 0;
			}

		default:
			AddressErrorException();
			return 0;
		}
	}

	template<MemoryAccessOperation operation>
	u32 VirtualToPhysicalAddressInvalid(const u64 virt_addr)
	{
		assert(false); /* CP0 status KSU == 0b11 */
		return 0;
	}

	template<MemoryAccessOperation operation>
	u32 VirtualToPhysicalAddress(const u64 virt_addr)
	{
		/* TODO If there are two or more TLB entries that coincide, the TLB operation is not
correctly executed. In this case, the TLB-Shutdown (TS) bit of the status register
is set to 1, and then the TLB cannot be used. */
		const u32 addr_VPN2 = (virt_addr >> virt_addr_shift_count & virt_addr_VPN_mask) >> 1;

		for (const TLB_Entry& entry : TLB_entries)
		{
			if (entry.VPN2 != addr_VPN2 || (!entry.G && entry.ASID != CP0_reg.entry_hi.ASID))
				continue;

			/* found match */
			if (!entry.lo_0.V)
			{
				TLB_InvalidException();
				break;
			}
			if constexpr (operation == MemoryAccessOperation::Write)
			{
				if (!entry.lo_0.D)
				{
					TLB_ModException();
					break;
				}
			}
			const u32 addr_offset = virt_addr & virt_addr_offset_mask;
			const u32 phys_addr = addr_offset | entry.lo_0.PFN << virt_addr_shift_count;
			/* TODO 'C' (0-7) is the page coherency attribute. Cache is not used if C == 2, else, it is used. */
			return phys_addr;
		}

		TLB_MissException<operation>(virt_addr); /* todo: distinguish between 32 and 64 bit */
		return 0;
	}

	template u32 VirtualToPhysicalAddressUserMode32<MemoryAccessOperation::Read>(const u64 virt_addr);
	template u32 VirtualToPhysicalAddressUserMode32<MemoryAccessOperation::Write>(const u64 virt_addr);
	template u32 VirtualToPhysicalAddressUserMode64<MemoryAccessOperation::Read>(const u64 virt_addr);
	template u32 VirtualToPhysicalAddressUserMode64<MemoryAccessOperation::Write>(const u64 virt_addr);
	template u32 VirtualToPhysicalAddressSupervisorMode32<MemoryAccessOperation::Read>(const u64 virt_addr);
	template u32 VirtualToPhysicalAddressSupervisorMode32<MemoryAccessOperation::Write>(const u64 virt_addr);
	template u32 VirtualToPhysicalAddressSupervisorMode64<MemoryAccessOperation::Read>(const u64 virt_addr);
	template u32 VirtualToPhysicalAddressSupervisorMode64<MemoryAccessOperation::Write>(const u64 virt_addr);
	template u32 VirtualToPhysicalAddressKernelMode32<MemoryAccessOperation::Read>(const u64 virt_addr);
	template u32 VirtualToPhysicalAddressKernelMode32<MemoryAccessOperation::Write>(const u64 virt_addr);
	template u32 VirtualToPhysicalAddressKernelMode64<MemoryAccessOperation::Read>(const u64 virt_addr);
	template u32 VirtualToPhysicalAddressKernelMode64<MemoryAccessOperation::Write>(const u64 virt_addr);
	template u32 VirtualToPhysicalAddressInvalid<MemoryAccessOperation::Write>(const u64 virt_addr);
	template u32 VirtualToPhysicalAddressInvalid<MemoryAccessOperation::Read>(const u64 virt_addr);

	template u32 VirtualToPhysicalAddress<MemoryAccessOperation::Read>(const u64 virt_addr);
	template u32 VirtualToPhysicalAddress<MemoryAccessOperation::Write>(const u64 virt_addr);
}