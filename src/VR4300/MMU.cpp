module VR4300:MMU;

import :COP0;
import :Exceptions;
import :Registers;

import Logging;
import Memory;

#include "../debug/DebugOptions.h"

namespace VR4300
{
	void InitializeMMU()
	{
		for (TLB_Entry& entry : tlb_entries)
		{
			entry.entry_hi.asid = 0xFF;
			entry.entry_hi.g = 1;
			entry.entry_hi.vpn2 = 0x07FF'FFFF;
			entry.vpn2_shifted = 0xFFFF'FFFF'FFFF'FFFF;
		}
	}


	void SetActiveVirtualToPhysicalFunctions()
	{
		using enum AddressingMode;

		if (cop0_reg.status.ksu == 0b00 || cop0_reg.status.erl == 1 || cop0_reg.status.exl == 1) /* Kernel mode */
		{
			if (cop0_reg.status.kx == 0)
			{
				active_virtual_to_physical_fun_read = VirtualToPhysicalAddressKernelMode32<MemoryAccess::Operation::Read>;
				active_virtual_to_physical_fun_write = VirtualToPhysicalAddressKernelMode32<MemoryAccess::Operation::Write>;
				addressing_mode = _32bit;
			}
			else
			{
				active_virtual_to_physical_fun_read = VirtualToPhysicalAddressKernelMode64<MemoryAccess::Operation::Read>;
				active_virtual_to_physical_fun_write = VirtualToPhysicalAddressKernelMode64<MemoryAccess::Operation::Write>;
				addressing_mode = _64bit;
			}
		}
		else if (cop0_reg.status.ksu == 0b01) /* Supervisor mode */
		{
			if (cop0_reg.status.sx == 0)
			{
				active_virtual_to_physical_fun_read = VirtualToPhysicalAddressSupervisorMode32<MemoryAccess::Operation::Read>;
				active_virtual_to_physical_fun_write = VirtualToPhysicalAddressSupervisorMode32<MemoryAccess::Operation::Write>;
				addressing_mode = _32bit;
			}
			else
			{
				active_virtual_to_physical_fun_read = VirtualToPhysicalAddressSupervisorMode64<MemoryAccess::Operation::Read>;
				active_virtual_to_physical_fun_write = VirtualToPhysicalAddressSupervisorMode64<MemoryAccess::Operation::Write>;
				addressing_mode = _64bit;
			}
		}
		else if (cop0_reg.status.ksu == 0b10) /* User mode */
		{
			if (cop0_reg.status.ux == 0)
			{
				active_virtual_to_physical_fun_read = VirtualToPhysicalAddressUserMode32<MemoryAccess::Operation::Read>;
				active_virtual_to_physical_fun_write = VirtualToPhysicalAddressUserMode32<MemoryAccess::Operation::Write>;
				addressing_mode = _32bit;
			}
			else
			{
				active_virtual_to_physical_fun_read = VirtualToPhysicalAddressUserMode64<MemoryAccess::Operation::Read>;
				active_virtual_to_physical_fun_write = VirtualToPhysicalAddressUserMode64<MemoryAccess::Operation::Write>;
				addressing_mode = _64bit;
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
			SignalAddressErrorException<operation>(virt_addr);
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
			SignalAddressErrorException<operation>(virt_addr);
			return 0;
		}
	}


	template<MemoryAccess::Operation operation>
	u32 VirtualToPhysicalAddressSupervisorMode32(u64 virt_addr)
	{
		virt_addr &= 0xFFFFFFFF;
		if ((virt_addr & 1 << 31) && (virt_addr & 0b11 << 29) != 0b10 << 29) /* $8000'0000-$BFFF'FFFF; $E000'0000-$FFFF'FFFF */
		{
			SignalAddressErrorException<operation>(virt_addr);
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
				SignalAddressErrorException<operation>(virt_addr);
				return 0;
			}

		case 0x4:
			if (virt_addr <= 0x4000'00FF'FFFF'FFFF) 
			{
				return VirtualToPhysicalAddress<operation>(virt_addr);
			}
			else
			{
				SignalAddressErrorException<operation>(virt_addr);
				return 0;
			}

		case 0xF:
			if ((virt_addr & 0xFFFF'FFFF'E000'0000) == 0xFFFF'FFFF'C000'0000)  /* $FFFF'FFFF'C0000'0000 -- $FFFF'FFFF'DFFF'FFFF */
			{
				return VirtualToPhysicalAddress<operation>(virt_addr);
			}
			else /* $F000'0000'0000'0000 -- $FFFF'FFFF'BFFF'FFFF; $FFFF'FFFF'E000'0000 -- $FFFF'FFFF'FFFF'FFFF */
			{
				SignalAddressErrorException<operation>(virt_addr);
				return 0;
			}

		default:
			SignalAddressErrorException<operation>(virt_addr);
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
				SignalAddressErrorException<operation>(virt_addr);
				return 0;
			}

		case 0x4:
			if (virt_addr <= 0x4000'00FF'FFFF'FFFF) 
			{
				return VirtualToPhysicalAddress<operation>(virt_addr);
			}
			else
			{
				SignalAddressErrorException<operation>(virt_addr);
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
				SignalAddressErrorException<operation>(virt_addr);
				return 0;
			}

		case 0xC:
			if (virt_addr <= 0xC000'00FF'7FFF'FFFF) 
			{
				return VirtualToPhysicalAddress<operation>(virt_addr);
			}
			else
			{
				SignalAddressErrorException<operation>(virt_addr);
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
				SignalAddressErrorException<operation>(virt_addr);
				return 0;
			}

		default:
			SignalAddressErrorException<operation>(virt_addr);
			return 0;
		}
	}


	template<MemoryAccess::Operation operation>
	u32 VirtualToPhysicalAddress(u64 virt_addr)
	{
		if (addressing_mode == AddressingMode::_32bit)
			virt_addr &= 0xFFFF'FFFF;

		for (const TLB_Entry& entry : tlb_entries)
		{
			/* Compare the virtual page number (divided by two) (VPN2) of the entry with the VPN2 of the virtual address */
			if ((virt_addr & entry.address_vpn2_mask) != entry.vpn2_shifted)
				continue;

			/* If the global bit is clear, the entry's ASID (Address space ID field) must coincide with the one in the EntryHi register. */
			if (!entry.entry_hi.g && entry.entry_hi.asid != cop0_reg.entry_hi.asid)
				continue;

			/* The VPN maps to two (consecutive) pages; EntryLo0 for even virtual pages and EntryLo1 for odd virtual pages. */
			const bool entry_reg_offset = virt_addr & entry.address_vpn_even_odd_mask;

			if (!entry.entry_lo[entry_reg_offset].v) /* If the "Valid" bit is clear, it indicates that the TLB entry is invalid. */
			{
				SignalException<Exception::TLB_Invalid, operation>();
				tlb_failure.bad_virt_addr = virt_addr;
				tlb_failure.bad_vpn2 = entry.entry_hi.vpn2;
				tlb_failure.bad_asid = cop0_reg.entry_hi.asid;
				return 0;
			}
			if constexpr (operation == MemoryAccess::Operation::Write)
			{
				if (!entry.entry_lo[entry_reg_offset].d) /* If the "Dirty" bit is clear, writing is disallowed. */
				{
					SignalException<Exception::TLB_Modification, operation>();
					tlb_failure.bad_virt_addr = virt_addr;
					tlb_failure.bad_vpn2 = entry.entry_hi.vpn2;
					tlb_failure.bad_asid = cop0_reg.entry_hi.asid;
					return 0;
				}
			}

			const u32 physical_address = u32(entry.entry_lo[entry_reg_offset].pfn | virt_addr & entry.address_offset_mask);
			return physical_address;
		}

		if (addressing_mode == AddressingMode::_32bit)
			SignalException<Exception::TLB_Miss, operation>();
		else
			SignalException<Exception::XTLB_Miss, operation>();
		tlb_failure.bad_virt_addr = virt_addr;
		tlb_failure.bad_vpn2 = virt_addr >> (page_size_to_addr_offset_bit_length[cop0_reg.page_mask.value] + 1) & 0xFF'FFFF'FFFF;
		tlb_failure.bad_asid = cop0_reg.entry_hi.asid;
		return 0;
	}


	template<std::integral Int, MemoryAccess::Alignment alignment, MemoryAccess::Operation operation>
	Int ReadVirtual(u64 virtual_address)
	{
		/* For aligned accesses, check if the address is misaligned. There is no need to do this for */
		/* instruction fetches; we know that the PC will always be a multiple of four. */
		if constexpr (sizeof Int > 1 && operation != MemoryAccess::Operation::InstrFetch)
		{
			if constexpr (alignment == MemoryAccess::Alignment::Aligned)
			{
				if (virtual_address & (sizeof Int - 1))
				{
					SignalAddressErrorException<operation>(virtual_address);
					return Int(0);
				}
			}
			else
			{
				/* For unaligned accesses, always read from the last boundary, with the number of bytes being sizeof Int.
				   The rest is taken care of by the function which handles the load instruction. */
				virtual_address &= ~(sizeof Int - 1);
			}
		}
		const u32 physical_address = std::invoke(active_virtual_to_physical_fun_read, virtual_address);
		if (exception_has_occurred)
			return Int(0);

		if (store_physical_address_on_load) /* TODO: what if TLB miss? */
			cop0_reg.LL_addr.p_addr = physical_address;

		const Int value = Memory::ReadPhysical<Int, operation>(physical_address);
		return value;
	}


	template<std::integral Int, MemoryAccess::Alignment alignment>
	void WriteVirtual(const u64 virtual_address, const Int data)
	{
		if constexpr (sizeof Int > 1 && alignment == MemoryAccess::Alignment::Aligned)
		{
			if (virtual_address & (sizeof Int - 1))
			{
				SignalAddressErrorException<MemoryAccess::Operation::Write>(virtual_address);
				return;
			}
		}
		u32 physical_address = std::invoke(active_virtual_to_physical_fun_write, virtual_address);
		if (exception_has_occurred)
			return;

		/* Find out how many bytes to write. */
		if constexpr (sizeof Int == 1)
			Memory::WritePhysical<1>(physical_address, data);
		else if constexpr (alignment == MemoryAccess::Alignment::Aligned)
			Memory::WritePhysical<sizeof Int>(physical_address, data);
		else
		{
			/* The result will different from sizeof(Int) only for unaligned memory accesses. */
			const std::size_t number_of_bytes = [&] {
				if constexpr (alignment == MemoryAccess::Alignment::UnalignedLeft) /* Store (Double)Word Left */
					return sizeof Int - (physical_address & (sizeof Int - 1)); 
				else /* UnalignedRight; Store (Double)Word Right */
					return (physical_address & (sizeof Int - 1)) + 1;
			}();
			if constexpr (alignment == MemoryAccess::Alignment::UnalignedRight)
				physical_address &= ~(sizeof Int - 1);
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

	template u32 VirtualToPhysicalAddress<MemoryAccess::Operation::Read>(u64);
	template u32 VirtualToPhysicalAddress<MemoryAccess::Operation::Write>(u64);
}