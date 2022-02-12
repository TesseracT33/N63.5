module VR4300:MMU;

import :COP0;
import :Exceptions;
import :Registers;

import Memory;

namespace VR4300
{
	void SetActiveVirtualToPhysicalFunctions()
	{
		if (cop0_reg.status.KSU == 0b00 || cop0_reg.status.ERL == 1 || cop0_reg.status.EXL == 1) /* Kernel mode */
		{
			if (cop0_reg.status.KX == 0)
			{
				active_virtual_to_physical_fun_read = VirtualToPhysicalAddressKernelMode32<MemoryAccess::Operation::Read>;
				active_virtual_to_physical_fun_write = VirtualToPhysicalAddressKernelMode32<MemoryAccess::Operation::Write>;
			}
			else
			{
				active_virtual_to_physical_fun_read = VirtualToPhysicalAddressKernelMode64<MemoryAccess::Operation::Read>;
				active_virtual_to_physical_fun_write = VirtualToPhysicalAddressKernelMode64<MemoryAccess::Operation::Write>;
			}
		}
		else if (cop0_reg.status.KSU == 0b01) /* Supervisor mode */
		{
			if (cop0_reg.status.SX == 0)
			{
				active_virtual_to_physical_fun_read = VirtualToPhysicalAddressSupervisorMode32<MemoryAccess::Operation::Read>;
				active_virtual_to_physical_fun_write = VirtualToPhysicalAddressSupervisorMode32<MemoryAccess::Operation::Write>;
			}
			else
			{
				active_virtual_to_physical_fun_read = VirtualToPhysicalAddressSupervisorMode64<MemoryAccess::Operation::Read>;
				active_virtual_to_physical_fun_write = VirtualToPhysicalAddressSupervisorMode64<MemoryAccess::Operation::Write>;
			}
		}
		else if (cop0_reg.status.KSU == 0b10) /* User mode */
		{
			if (cop0_reg.status.UX == 0)
			{
				active_virtual_to_physical_fun_read = VirtualToPhysicalAddressUserMode32<MemoryAccess::Operation::Read>;
				active_virtual_to_physical_fun_write = VirtualToPhysicalAddressUserMode32<MemoryAccess::Operation::Write>;
			}
			else
			{
				active_virtual_to_physical_fun_read = VirtualToPhysicalAddressUserMode64<MemoryAccess::Operation::Read>;
				active_virtual_to_physical_fun_write = VirtualToPhysicalAddressUserMode64<MemoryAccess::Operation::Write>;
			}
		}
		else /* Unknown?! */
		{
			assert(false);
		}
	}


	template<MemoryAccess::Operation operation>
	u32 VirtualToPhysicalAddressUserMode32(u64 virt_addr)
	{
		virt_addr &= 0xFFFFFFFF;
		if (virt_addr < 0x80000000)
		{
			return VirtualToPhysicalAddress<operation>(virt_addr);
		}
		else
		{
			SignalException<Exception::AddressError, operation>();
			return 0;
		}
	}


	template<MemoryAccess::Operation operation>
	u32 VirtualToPhysicalAddressUserMode64(u64 virt_addr)
	{
		if (virt_addr < 0x100'00000000)
		{
			return VirtualToPhysicalAddress<operation>(virt_addr);
		}
		else
		{
			SignalException<Exception::AddressError, operation>();
			return 0;
		}
	}


	template<MemoryAccess::Operation operation>
	u32 VirtualToPhysicalAddressSupervisorMode32(u64 virt_addr)
	{
		virt_addr &= 0xFFFFFFFF;
		if ((virt_addr & 1 << 31) && (virt_addr & 0b11 << 29) != 0b10 << 29) /* $8000'0000-$BFFF'FFFF; $E000'0000-$FFFF'FFFF */
		{
			SignalException<Exception::AddressError, operation>();
			return 0;
		}
		else /* 0-$7FFF'FFFF; $C000'0000-$DFFF'FFFF */
		{
			return VirtualToPhysicalAddress<operation>(virt_addr);
		}
	}


	template<MemoryAccess::Operation operation>
	u32 VirtualToPhysicalAddressSupervisorMode64(u64 virt_addr)
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
				SignalException<Exception::AddressError, operation>();
				return 0;
			}

		case 0x4:
			if (virt_addr <= 0x4000'00FF'FFFF'FFFF) 
			{
				return VirtualToPhysicalAddress<operation>(virt_addr);
			}
			else
			{
				SignalException<Exception::AddressError, operation>();
				return 0;
			}

		case 0xF:
			if ((virt_addr & 0xFFFF'FFFF'E000'0000) == 0xFFFF'FFFF'C000'0000)  /* $FFFF'FFFF'C0000'0000 -- $FFFF'FFFF'DFFF'FFFF */
			{
				return VirtualToPhysicalAddress<operation>(virt_addr);
			}
			else /* $F000'0000'0000'0000 -- $FFFF'FFFF'BFFF'FFFF; $FFFF'FFFF'E000'0000 -- $FFFF'FFFF'FFFF'FFFF */
			{
				SignalException<Exception::AddressError, operation>();
				return 0;
			}

		default:
			SignalException<Exception::AddressError, operation>();
			return 0;
		}
	}

	template<MemoryAccess::Operation operation>
	u32 VirtualToPhysicalAddressKernelMode32(u64 virt_addr)
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


	template<MemoryAccess::Operation operation>
	u32 VirtualToPhysicalAddressKernelMode64(u64 virt_addr)
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
				SignalException<Exception::AddressError, operation>();
				return 0;
			}

		case 0x4:
			if (virt_addr <= 0x4000'00FF'FFFF'FFFF) 
			{
				return VirtualToPhysicalAddress<operation>(virt_addr);
			}
			else
			{
				SignalException<Exception::AddressError, operation>();
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
				SignalException<Exception::AddressError, operation>();
				return 0;
			}

		case 0xC:
			if (virt_addr <= 0xC000'00FF'7FFF'FFFF) 
			{
				return VirtualToPhysicalAddress<operation>(virt_addr);
			}
			else
			{
				SignalException<Exception::AddressError, operation>();
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
				SignalException<Exception::AddressError, operation>();
				return 0;
			}

		default:
			SignalException<Exception::AddressError, operation>();
			return 0;
		}
	}


	template<MemoryAccess::Operation operation>
	u32 VirtualToPhysicalAddress(const u64 virt_addr)
	{
		/* Used for extracting the VPN from the virtual address;
		   given the page mask register in a TLB entry, tells by how many positions the virtual address needs to be shifted to the left. */
		static constexpr std::array page_mask_to_vaddr_VPN_shift_count = [] {
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

		/* Used for extracting the offset from the virtual address;
		   given the page mask register in a TLB entry, tells what the virtual address need to be masked with. */
		static constexpr std::array page_mask_to_vaddr_offset_mask = [] {
			std::array<u32, 4096> table{};
			for (int i = 0; i < table.size(); i++) {
				table[i] = [&] {
					if (i == 0) return 0xFFF;
					if (i <= 3) return 0x3FFF;
					if (i <= 15) return 0xFFFF;
					if (i <= 63) return 0x3FFFF;
					if (i <= 255) return 0xFFFFF;
					if (i <= 1023) return 0x3FFFFF;
					return 0xFFFFFF;
				}();
			}
			return table;
		}();

		/* TODO If there are two or more TLB entries that coincide, the TLB operation is not
correctly executed. In this case, the TLB-Shutdown (TS) bit of the status register
is set to 1, and then the TLB cannot be used. */

		u32 addr_VPN2 = 0;

		for (const TLB_Entry& entry : TLB_entries)
		{
			const u32 addr_VPN = virt_addr >> page_mask_to_vaddr_VPN_shift_count[entry.MASK] & 0xFFF'FFFF; /* VPN is at most 28 bits */
			addr_VPN2 = addr_VPN >> 1; /* VPN divided by two */

			/* For a TLB hit to occur, the virtual page number of the virtual address must coincide with the one in the TLB entry. */
			if (entry.VPN2 != addr_VPN2 || (!entry.G && entry.ASID != cop0_reg.entry_hi.ASID))
				continue;

			/* The VPN maps to two (consecutive) pages; EntryLo0 for even virtual pages and EntryLo1 for odd virtual pages. */
			const auto& entry_reg = (addr_VPN & 1) ? entry.lo_1 : entry.lo_0;

			if (!entry_reg.V) /* If the "Valid" bit is clear, it indicates that the TLB entry is invalid. */
			{
				SignalException<Exception::TLB_Invalid, operation>();
				return 0;
			}
			if constexpr (operation == MemoryAccess::Operation::Write)
			{
				if (!entry_reg.D) /* If the "Dirty" bit is clear, writing is disallowed. */
				{
					SignalException<Exception::TLB_Modification, operation>();
					return 0;
				}
			}
			const u32 virt_addr_offset = virt_addr & page_mask_to_vaddr_offset_mask[entry.MASK];
			const u32 phys_addr = u32(virt_addr_offset | entry_reg.PFN << page_mask_to_vaddr_VPN_shift_count[entry.MASK]);
			/* TODO 'C' (0-7) is the page coherency attribute. Cache is not used if C == 2, else, it is used. */
			return phys_addr;
		}

		SignalException<Exception::TLB_Miss, operation>(); /* todo: distinguish between 32 and 64 bit */
		return 0;
	}


	template<std::integral Int, MemoryAccess::Alignment alignment, MemoryAccess::Operation operation>
	Int ReadVirtual(u64 virtual_address)
	{
		if constexpr (sizeof Int > 1)
		{
			if constexpr (alignment == MemoryAccess::Alignment::Aligned)
			{
				if (virtual_address & (sizeof Int - 1))
				{
					SignalException<Exception::AddressError, operation>();
					return Int(0);
				}
			}
			else
			{
				/* For unaligned accesses, always read from the last boundary, with the number of bytes being sizeof T.
				   Shifting/masking is done by the function which handles the load instruction. */
				virtual_address &= ~(sizeof Int - 1);
			}
		}
		const u32 physical_address = std::invoke(active_virtual_to_physical_fun_read, virtual_address);
		if (exception_has_occurred)
			return Int(0);

		const Int value = Memory::ReadPhysical<Int>(physical_address);
		const Int byteswapped_value = Memory::Byteswap(value); /* If the host is big endian, does nothing */
		return byteswapped_value;
	}


	template<std::integral Int, MemoryAccess::Alignment alignment>
	void WriteVirtual(const u64 virtual_address, const Int data)
	{
		if constexpr (sizeof Int > 1 && alignment == MemoryAccess::Alignment::Aligned)
		{
			if (virtual_address & (sizeof Int - 1))
			{
				SignalException<Exception::AddressError, MemoryAccess::Operation::Write>();
				return;
			}
		}
		const u32 physical_address = std::invoke(active_virtual_to_physical_fun_write, virtual_address);
		if (exception_has_occurred)
			return;

		const Int byteswapped_data = Memory::Byteswap(data); /* If the host is big endian, does nothing */

		if constexpr (sizeof Int == 1)
			Memory::WritePhysical<1>(physical_address, byteswapped_data);
		else if constexpr (alignment == MemoryAccess::Alignment::Aligned)
			Memory::WritePhysical<sizeof Int>(physical_address, byteswapped_data);
		else
		{
			/* The result will different from sizeof(T) only for unaligned memory accesses. */
			const std::size_t number_of_bytes = [&] {
				if constexpr (alignment == MemoryAccess::Alignment::UnalignedLeft) /* Store (Double)Word Left */
					return sizeof Int - (physical_address & (sizeof Int - 1)); 
				else if constexpr (alignment == MemoryAccess::Alignment::UnalignedRight) /* Store (Double)Word Right */
					return (physical_address & (sizeof Int - 1)) + 1;
				else
					static_assert(false);
			}();
			/* This branch will be worth it; the fact that we can pass the number of bytes to access
			   as a template argument means that, among other things, memcpy will be optimized away
			   to 'mov' instructions, when we later go to actually access data. */
			switch (number_of_bytes)
			{
			break; case 1: Memory::WritePhysical<1>(physical_address, byteswapped_data);
			break; case 2: Memory::WritePhysical<2>(physical_address, byteswapped_data);
			break; case 3: Memory::WritePhysical<3>(physical_address, byteswapped_data);
			break; case 4: Memory::WritePhysical<4>(physical_address, byteswapped_data);
			break; case 5: Memory::WritePhysical<5>(physical_address, byteswapped_data);
			break; case 6: Memory::WritePhysical<6>(physical_address, byteswapped_data);
			break; case 7: Memory::WritePhysical<7>(physical_address, byteswapped_data);
			break; case 8: Memory::WritePhysical<8>(physical_address, byteswapped_data);
			break; default: assert(false);
			}
		}
	}


	u32 FetchInstruction(u64 virtual_address)
	{
		return ReadVirtual<u32, MemoryAccess::Alignment::Aligned, MemoryAccess::Operation::InstrFetch>(virtual_address);
	}


	template u8 ReadVirtual<u8, MemoryAccess::Alignment::Aligned>(u64);
	template s8 ReadVirtual<s8, MemoryAccess::Alignment::Aligned>(u64);
	template u16 ReadVirtual<u16, MemoryAccess::Alignment::Aligned>(u64);
	template s16 ReadVirtual<s16, MemoryAccess::Alignment::Aligned>(u64);
	template u32 ReadVirtual<u32, MemoryAccess::Alignment::Aligned>(u64);
	template s32 ReadVirtual<s32, MemoryAccess::Alignment::Aligned>(u64);
	template u64 ReadVirtual<u64, MemoryAccess::Alignment::Aligned>(u64);
	template s64 ReadVirtual<s64, MemoryAccess::Alignment::Aligned>(u64);
	template u8 ReadVirtual<u8, MemoryAccess::Alignment::UnalignedLeft>(u64);
	template s8 ReadVirtual<s8, MemoryAccess::Alignment::UnalignedLeft>(u64);
	template u16 ReadVirtual<u16, MemoryAccess::Alignment::UnalignedLeft>(u64);
	template s16 ReadVirtual<s16, MemoryAccess::Alignment::UnalignedLeft>(u64);
	template u32 ReadVirtual<u32, MemoryAccess::Alignment::UnalignedLeft>(u64);
	template s32 ReadVirtual<s32, MemoryAccess::Alignment::UnalignedLeft>(u64);
	template u64 ReadVirtual<u64, MemoryAccess::Alignment::UnalignedLeft>(u64);
	template s64 ReadVirtual<s64, MemoryAccess::Alignment::UnalignedLeft>(u64);
	template u8 ReadVirtual<u8, MemoryAccess::Alignment::UnalignedRight>(u64);
	template s8 ReadVirtual<s8, MemoryAccess::Alignment::UnalignedRight>(u64);
	template u16 ReadVirtual<u16, MemoryAccess::Alignment::UnalignedRight>(u64);
	template s16 ReadVirtual<s16, MemoryAccess::Alignment::UnalignedRight>(u64);
	template u32 ReadVirtual<u32, MemoryAccess::Alignment::UnalignedRight>(u64);
	template s32 ReadVirtual<s32, MemoryAccess::Alignment::UnalignedRight>(u64);
	template u64 ReadVirtual<u64, MemoryAccess::Alignment::UnalignedRight>(u64);
	template s64 ReadVirtual<s64, MemoryAccess::Alignment::UnalignedRight>(u64);

	template void WriteVirtual<u8, MemoryAccess::Alignment::Aligned>(const u64, const u8);
	template void WriteVirtual<s8, MemoryAccess::Alignment::Aligned>(const u64, const s8);
	template void WriteVirtual<u16, MemoryAccess::Alignment::Aligned>(const u64, const u16);
	template void WriteVirtual<s16, MemoryAccess::Alignment::Aligned>(const u64, const s16);
	template void WriteVirtual<u32, MemoryAccess::Alignment::Aligned>(const u64, const u32);
	template void WriteVirtual<s32, MemoryAccess::Alignment::Aligned>(const u64, const s32);
	template void WriteVirtual<u64, MemoryAccess::Alignment::Aligned>(const u64, const u64);
	template void WriteVirtual<s64, MemoryAccess::Alignment::Aligned>(const u64, const s64);
	template void WriteVirtual<u8, MemoryAccess::Alignment::UnalignedLeft>(const u64, const u8);
	template void WriteVirtual<s8, MemoryAccess::Alignment::UnalignedLeft>(const u64, const s8);
	template void WriteVirtual<u16, MemoryAccess::Alignment::UnalignedLeft>(const u64, const u16);
	template void WriteVirtual<s16, MemoryAccess::Alignment::UnalignedLeft>(const u64, const s16);
	template void WriteVirtual<u32, MemoryAccess::Alignment::UnalignedLeft>(const u64, const u32);
	template void WriteVirtual<s32, MemoryAccess::Alignment::UnalignedLeft>(const u64, const s32);
	template void WriteVirtual<u64, MemoryAccess::Alignment::UnalignedLeft>(const u64, const u64);
	template void WriteVirtual<s64, MemoryAccess::Alignment::UnalignedLeft>(const u64, const s64);
	template void WriteVirtual<u8, MemoryAccess::Alignment::UnalignedRight>(const u64, const u8);
	template void WriteVirtual<s8, MemoryAccess::Alignment::UnalignedRight>(const u64, const s8);
	template void WriteVirtual<u16, MemoryAccess::Alignment::UnalignedRight>(const u64, const u16);
	template void WriteVirtual<s16, MemoryAccess::Alignment::UnalignedRight>(const u64, const s16);
	template void WriteVirtual<u32, MemoryAccess::Alignment::UnalignedRight>(const u64, const u32);
	template void WriteVirtual<s32, MemoryAccess::Alignment::UnalignedRight>(const u64, const s32);
	template void WriteVirtual<u64, MemoryAccess::Alignment::UnalignedRight>(const u64, const u64);
	template void WriteVirtual<s64, MemoryAccess::Alignment::UnalignedRight>(const u64, const s64);

	template u32 VirtualToPhysicalAddressUserMode32<MemoryAccess::Operation::Read>(const u64);
	template u32 VirtualToPhysicalAddressUserMode32<MemoryAccess::Operation::Write>(const u64);
	template u32 VirtualToPhysicalAddressUserMode64<MemoryAccess::Operation::Read>(const u64);
	template u32 VirtualToPhysicalAddressUserMode64<MemoryAccess::Operation::Write>(const u64);
	template u32 VirtualToPhysicalAddressSupervisorMode32<MemoryAccess::Operation::Read>(const u64);
	template u32 VirtualToPhysicalAddressSupervisorMode32<MemoryAccess::Operation::Write>(const u64);
	template u32 VirtualToPhysicalAddressSupervisorMode64<MemoryAccess::Operation::Read>(const u64);
	template u32 VirtualToPhysicalAddressSupervisorMode64<MemoryAccess::Operation::Write>(const u64);
	template u32 VirtualToPhysicalAddressKernelMode32<MemoryAccess::Operation::Read>(const u64);
	template u32 VirtualToPhysicalAddressKernelMode32<MemoryAccess::Operation::Write>(const u64);
	template u32 VirtualToPhysicalAddressKernelMode64<MemoryAccess::Operation::Read>(const u64);
	template u32 VirtualToPhysicalAddressKernelMode64<MemoryAccess::Operation::Write>(const u64);
	template u32 VirtualToPhysicalAddressInvalid<MemoryAccess::Operation::Write>(const u64);
	template u32 VirtualToPhysicalAddressInvalid<MemoryAccess::Operation::Read>(const u64);

	template u32 VirtualToPhysicalAddress<MemoryAccess::Operation::Read>(const u64);
	template u32 VirtualToPhysicalAddress<MemoryAccess::Operation::Write>(const u64);
}