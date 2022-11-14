module VR4300:MMU;

import :Cache;
import :COP0;
import :Exceptions;
import :Operation;

import Logging;

namespace VR4300
{
	void TlbEntry::Read() const
	{
		cop0.entry_lo[0] = this->entry_lo[0];
		cop0.entry_lo[1] = this->entry_lo[1];
		cop0.entry_hi = std::bit_cast<COP0Registers::EntryHi>(
			std::bit_cast<u64>(this->entry_hi) & ~u64(this->page_mask));
		cop0.page_mask = this->page_mask;
	}


	void TlbEntry::Write()
	{
		this->entry_lo[0] = cop0.entry_lo[0];
		this->entry_lo[1] = cop0.entry_lo[1];
		this->entry_hi = std::bit_cast<COP0Registers::EntryHi>(
			std::bit_cast<u64>(cop0.entry_hi) & ~u64(cop0.page_mask));
		this->page_mask = cop0.page_mask;
		this->entry_hi.g = cop0.entry_lo[0].g & cop0.entry_lo[1].g;
		/* Compute things that speed up virtual-to-physical-address translation. */
		auto addr_offset_bit_length = page_size_to_addr_offset_bit_length[cop0.page_mask >> 13];
		u64 vpn2_mask = addressing_mode == AddressingMode::_32bit ? 0xFFFF'FFFF : 0xFF'FFFF'FFFF;
		vpn2_addr_mask = vpn2_mask << (addr_offset_bit_length + 1) & vpn2_mask;
		offset_addr_mask = (1 << addr_offset_bit_length) - 1;
		vpn2_shifted = entry_hi.vpn2 << (addr_offset_bit_length + 1);
	}


	u32 FetchInstruction(u64 virtual_address)
	{
		return ReadVirtual<s32, Alignment::Aligned, Memory::Operation::InstrFetch>(virtual_address);
	}


	void InitializeMMU()
	{
		for (TlbEntry& entry : tlb_entries) {
			entry.entry_hi.asid = 0xFF;
			entry.entry_hi.g = 1;
			entry.entry_hi.vpn2 = 0x07FF'FFFF;
			entry.vpn2_shifted = 0xFFFF'FFFF'FFFF'FFFF;
		}
	}


	template<std::signed_integral Int, Alignment alignment, Memory::Operation operation>
	Int ReadVirtual(u64 virtual_address)
	{
		/* For aligned accesses, check if the address is misaligned. No need to do it for instruction fetches.
		   The PC can be misaligned after an ERET instruction, but we manually check there if the PC read from the EPC register is misaligned. */
		if constexpr (sizeof(Int) > 1 && operation != Memory::Operation::InstrFetch) {
			if constexpr (alignment == Alignment::Aligned) {
				if (virtual_address & (sizeof(Int) - 1)) {
					SignalAddressErrorException<operation>(virtual_address);
					return {};
				}
			}
			else {
				/* For unaligned accesses, always read from the last boundary, with the number of bytes being sizeof Int.
				   The rest is taken care of by the function which handles the load instruction. */
				virtual_address &= ~(sizeof(Int) - 1);
			}
		}
		bool cacheable_area;
		u32 physical_address = active_virtual_to_physical_fun_read(virtual_address, cacheable_area);
		if (exception_has_occurred) {
			return {};
		}
		last_physical_address_on_load = physical_address;
		if (cacheable_area) { /* TODO: figure out some way to avoid this branch, if possible */
			/* cycle counter incremented in the function, depending on if cache hit/miss */
			return ReadCacheableArea<Int, operation>(physical_address);
		}
		else {
			p_cycle_counter += cache_miss_cycle_delay; /* using the same number here */
			return Memory::ReadPhysical<Int, operation>(physical_address);
		}
	}


	void SetActiveVirtualToPhysicalFunctions()
	{
		using enum AddressingMode;
		using enum OperatingMode;

		if (cop0.status.ksu == 0 || cop0.status.erl == 1 || cop0.status.exl == 1) { /* Kernel mode */
			operating_mode = Kernel;
			if (cop0.status.kx == 0) {
				active_virtual_to_physical_fun_read = VirtualToPhysicalAddressKernelMode32<Memory::Operation::Read>;
				active_virtual_to_physical_fun_write = VirtualToPhysicalAddressKernelMode32<Memory::Operation::Write>;
				addressing_mode = _32bit;
			}
			else {
				active_virtual_to_physical_fun_read = VirtualToPhysicalAddressKernelMode64<Memory::Operation::Read>;
				active_virtual_to_physical_fun_write = VirtualToPhysicalAddressKernelMode64<Memory::Operation::Write>;
				addressing_mode = _64bit;
			}
		}
		else if (cop0.status.ksu == 1) { /* Supervisor mode */
			operating_mode = Supervisor;
			if (cop0.status.sx == 0) {
				active_virtual_to_physical_fun_read = VirtualToPhysicalAddressSupervisorMode32<Memory::Operation::Read>;
				active_virtual_to_physical_fun_write = VirtualToPhysicalAddressSupervisorMode32<Memory::Operation::Write>;
				addressing_mode = _32bit;
			}
			else {
				active_virtual_to_physical_fun_read = VirtualToPhysicalAddressSupervisorMode64<Memory::Operation::Read>;
				active_virtual_to_physical_fun_write = VirtualToPhysicalAddressSupervisorMode64<Memory::Operation::Write>;
				addressing_mode = _64bit;
			}
		}
		else if (cop0.status.ksu == 2) { /* User mode */
			operating_mode = User;
			if (cop0.status.ux == 0) {
				active_virtual_to_physical_fun_read = VirtualToPhysicalAddressUserMode32<Memory::Operation::Read>;
				active_virtual_to_physical_fun_write = VirtualToPhysicalAddressUserMode32<Memory::Operation::Write>;
				addressing_mode = _32bit;
			}
			else {
				active_virtual_to_physical_fun_read = VirtualToPhysicalAddressUserMode64<Memory::Operation::Read>;
				active_virtual_to_physical_fun_write = VirtualToPhysicalAddressUserMode64<Memory::Operation::Write>;
				addressing_mode = _64bit;
			}
		}
		else { /* Unknown?! */
			assert(false);
		}
	}


	template<Memory::Operation operation>
	u32 VirtualToPhysicalAddressUserMode32(u64 virt_addr, bool& cacheable_area)
	{
		if (virt_addr & 0x8000'0000) {
			SignalAddressErrorException<operation>(virt_addr);
			return 0;
		}
		else {
			cacheable_area = false;
			return VirtualToPhysicalAddressTlb<operation>(virt_addr);

		}
	}


	template<Memory::Operation operation>
	u32 VirtualToPhysicalAddressUserMode64(u64 virt_addr, bool& cacheable_area)
	{
		if (virt_addr < 0x100'0000'0000) {
			cacheable_area = false;
			return VirtualToPhysicalAddressTlb<operation>(virt_addr);
		}
		else {
			SignalAddressErrorException<operation>(virt_addr);
			return 0;
		}
	}


	template<Memory::Operation operation>
	u32 VirtualToPhysicalAddressSupervisorMode32(u64 virt_addr, bool& cacheable_area)
	{
		/* $8000'0000-$BFFF'FFFF; $E000'0000-$FFFF'FFFF */
		if ((virt_addr & 1 << 31) && (virt_addr & 0b11 << 29) != 0b10 << 29) {
			SignalAddressErrorException<operation>(virt_addr);
			return 0;
		}
		/* 0-$7FFF'FFFF; $C000'0000-$DFFF'FFFF */
		else  {
			cacheable_area = false;
			return VirtualToPhysicalAddressTlb<operation>(virt_addr);
		}
	}


	template<Memory::Operation operation>
	u32 VirtualToPhysicalAddressSupervisorMode64(u64 virt_addr, bool& cacheable_area)
	{
		switch (virt_addr >> 60) {
		case 0x0:
			if (virt_addr <= 0x0000'00FF'FFFF'FFFF) {
				cacheable_area = false;
				return VirtualToPhysicalAddressTlb<operation>(virt_addr);
			}
			else {
				SignalAddressErrorException<operation>(virt_addr);
				return 0;
			}

		case 0x4:
			if (virt_addr <= 0x4000'00FF'FFFF'FFFF) {
				cacheable_area = false;
				return VirtualToPhysicalAddressTlb<operation>(virt_addr);
			}
			else {
				SignalAddressErrorException<operation>(virt_addr);
				return 0;
			}

		case 0xF:
			/* $FFFF'FFFF'C0000'0000 -- $FFFF'FFFF'DFFF'FFFF */
			if ((virt_addr & 0xFFFF'FFFF'E000'0000) == 0xFFFF'FFFF'C000'0000) {
				cacheable_area = false;
				return VirtualToPhysicalAddressTlb<operation>(virt_addr);
			}
			/* $F000'0000'0000'0000 -- $FFFF'FFFF'BFFF'FFFF; $FFFF'FFFF'E000'0000 -- $FFFF'FFFF'FFFF'FFFF */
			else  {
				SignalAddressErrorException<operation>(virt_addr);
				return 0;
			}

		default:
			SignalAddressErrorException<operation>(virt_addr);
			return 0;
		}
	}


	template<Memory::Operation operation>
	u32 VirtualToPhysicalAddressKernelMode32(u64 virt_addr, bool& cacheable_area)
	{
		//if (virt_addr < 0xFFFF'FFFF'0000'0000) { /* TODO: same for user and supervisor modes? */
		//	SignalAddressErrorException<operation>(virt_addr);
		//	return 0;
		//}
		if ((virt_addr & 0xE000'0000) == 0x8000'0000) {
			/* $8000'0000-$9FFF'FFFF; TLB unmapped; cacheable */
			cacheable_area = true;
			return virt_addr & 0x1FFF'FFFF;
		}
		else if ((virt_addr & 0xE000'0000) == 0xA000'0000) {
			/* $A000'0000-$BFFF'FFFF; TLB unmapped; uncacheable */
			cacheable_area = false;
			return virt_addr & 0x1FFF'FFFF;
		}
		else {
			cacheable_area = false;
			return VirtualToPhysicalAddressTlb<operation>(s32(virt_addr)); /* TLB mapped */
		}
	}


	template<Memory::Operation operation>
	u32 VirtualToPhysicalAddressKernelMode64(u64 virt_addr, bool& cacheable_area)
	{
		switch (virt_addr >> 60 & 0xF) {
		case 0x0:
			if (virt_addr <= 0x0000'00FF'FFFF'FFFF) {
				cacheable_area = false;
				return VirtualToPhysicalAddressTlb<operation>(virt_addr);
			}
			else {
				SignalAddressErrorException<operation>(virt_addr);
				return 0;
			}

		case 0x4:
			if (virt_addr <= 0x4000'00FF'FFFF'FFFF) {
				cacheable_area = false;
				return VirtualToPhysicalAddressTlb<operation>(virt_addr);
			}
			else {
				SignalAddressErrorException<operation>(virt_addr);
				return 0;
			}

		case 0x8: case 0x9: case 0xA: case 0xB:
			if ((virt_addr & 0x07FF'FFFF'0000'0000) == 0) { /* tlb unmapped */
				cacheable_area = (virt_addr & 0x9800'0000'0000'0000) != 0x9000'0000'0000'0000;
				u64 phys_addr = virt_addr & 0xFFFF'FFFF;
				return phys_addr;
			}
			else {
				SignalAddressErrorException<operation>(virt_addr);
				return 0;
			}

		case 0xC:
			if (virt_addr <= 0xC000'00FF'7FFF'FFFF) {
				cacheable_area = false;
				return VirtualToPhysicalAddressTlb<operation>(virt_addr);
			}
			else {
				SignalAddressErrorException<operation>(virt_addr);
				return 0;
			}

		case 0xF:
			if (virt_addr >= 0xFFFF'FFFF'0000'0000) {
				return VirtualToPhysicalAddressKernelMode32<operation>(virt_addr, cacheable_area);
			}
			else {
				SignalAddressErrorException<operation>(virt_addr);
				return 0;
			}

		default:
			SignalAddressErrorException<operation>(virt_addr);
			return 0;
		}
	}


	template<Memory::Operation operation>
	u32 VirtualToPhysicalAddressTlb(u64 virt_addr)
	{
		if (addressing_mode == AddressingMode::_32bit) {
			virt_addr &= 0xFFFF'FFFF;
		}
		auto OnBadEntry = [&](const TlbEntry& entry) {
			address_failure.bad_virt_addr = virt_addr;
			address_failure.bad_vpn2 = entry.entry_hi.vpn2;
			address_failure.bad_asid = entry.entry_hi.asid;
		};
		for (const TlbEntry& entry : tlb_entries) {
			/* Compare the virtual page number (divided by two; VPN2) of the entry with the VPN2 of the virtual address */
			if ((virt_addr & entry.vpn2_addr_mask) != entry.vpn2_shifted) continue;
			/* If the global bit is clear, the entry's ASID (Address space ID field) must coincide with the one in the EntryHi register. */
			if (!entry.entry_hi.g && entry.entry_hi.asid != cop0.entry_hi.asid) continue;
			/* Bits 62-63 of vaddr must match the entry's region */      /* TODO: also checked in 32-bit mode? */
			if (virt_addr >> 62 != entry.entry_hi.r) continue; 
			/* The VPN maps to two (consecutive) pages; EntryLo0 for even virtual pages and EntryLo1 for odd virtual pages. */
			bool vpn_odd = (virt_addr & (entry.offset_addr_mask + 1)) != 0;
			auto entry_lo = entry.entry_lo[vpn_odd];
			if (!entry_lo.v) { /* If the "Valid" bit is clear, it indicates that the TLB entry is invalid. */
				SignalException<Exception::TlbInvalid, operation>();
				OnBadEntry(entry);
				return 0;
			}
			if constexpr (operation == Memory::Operation::Write) {
				if (!entry_lo.d) { /* If the "Dirty" bit is clear, writing is disallowed. */
					SignalException<Exception::TlbModification, operation>();
					OnBadEntry(entry);
					return 0;
				}
			}
			/* TLB hit */
			return virt_addr & entry.offset_addr_mask | entry_lo.pfn << 12 & ~entry.offset_addr_mask;
		}
		/* TLB miss */
		if (addressing_mode == AddressingMode::_32bit) SignalException<Exception::TlbMiss, operation>();
		else                                           SignalException<Exception::XtlbMiss, operation>();
		address_failure.bad_virt_addr = virt_addr;
		address_failure.bad_vpn2 = virt_addr >> page_size_to_addr_offset_bit_length[cop0.page_mask >> 13] & 0xFF'FFFF'FFFF;
		address_failure.bad_asid = cop0.entry_hi.asid;
		return 0;
	}


	template<Alignment alignment>
	void WriteVirtual(u64 virtual_address, std::signed_integral auto data)
	{
		if constexpr (sizeof(data) > 1 && alignment == Alignment::Aligned) {
			if (virtual_address & (sizeof(data) - 1)) {
				SignalAddressErrorException<Memory::Operation::Write>(virtual_address);
				return;
			}
		}
		bool cacheable_area;
		u32 physical_address = active_virtual_to_physical_fun_write(virtual_address, cacheable_area);
		if (exception_has_occurred) {
			return;
		}

#define WRITE(NUM_BYTES) { \
		if (cacheable_area) WriteCacheableArea<NUM_BYTES>(physical_address, data); \
		else { p_cycle_counter += cache_miss_cycle_delay; Memory::WritePhysical<NUM_BYTES>(physical_address, data); } }

		if constexpr (sizeof(data) == 1 || alignment == Alignment::Aligned) {
			WRITE(sizeof(data));
		}
		else {
			/* Find out how many bytes to write. The result will differ from sizeof(Int) only for unaligned memory accesses. */
			size_t num_bytes = [&] {
				if constexpr (alignment == Alignment::UnalignedLeft) { /* Store (Double)Word Left */
					return sizeof(data) - (physical_address & (sizeof(data) - 1));
				}
				else { /* UnalignedRight; Store (Double)Word Right */
					return (physical_address & (sizeof(data) - 1)) + 1;
				}
			}();
			if constexpr (alignment == Alignment::UnalignedRight) {
				physical_address &= ~(sizeof(data) - 1);
			}
			/* This branch will be worth it; the fact that we can pass the number of bytes to access
			   as a template argument means that, among other things, memcpy will be optimized away
			   to a single or just a few 'mov' instructions, when we later go to actually access data. */
			switch (num_bytes) {
			case 1: WRITE(1); break;
			case 2: if constexpr (sizeof(data) >= 2) WRITE(2); break;
			case 3: if constexpr (sizeof(data) >= 3) WRITE(3); break;
			case 4: if constexpr (sizeof(data) >= 4) WRITE(4); break;
			case 5: if constexpr (sizeof(data) >= 5) WRITE(5); break;
			case 6: if constexpr (sizeof(data) >= 6) WRITE(6); break;
			case 7: if constexpr (sizeof(data) >= 7) WRITE(7); break;
			case 8: if constexpr (sizeof(data) >= 8) WRITE(8); break;
			default: std::unreachable();
			}
		}
	}


	template s8 ReadVirtual<s8, Alignment::Aligned>(u64);
	template s16 ReadVirtual<s16, Alignment::Aligned>(u64);
	template s32 ReadVirtual<s32, Alignment::Aligned>(u64);
	template s64 ReadVirtual<s64, Alignment::Aligned>(u64);
	template s32 ReadVirtual<s32, Alignment::UnalignedLeft>(u64);
	template s64 ReadVirtual<s64, Alignment::UnalignedLeft>(u64);
	template s32 ReadVirtual<s32, Alignment::UnalignedRight>(u64);
	template s64 ReadVirtual<s64, Alignment::UnalignedRight>(u64);

	template void WriteVirtual<Alignment::Aligned>(u64, s8);
	template void WriteVirtual<Alignment::Aligned>(u64, s16);
	template void WriteVirtual<Alignment::Aligned>(u64, s32);
	template void WriteVirtual<Alignment::Aligned>(u64, s64);
	template void WriteVirtual<Alignment::UnalignedLeft>(u64, s32);
	template void WriteVirtual<Alignment::UnalignedLeft>(u64, s64);
	template void WriteVirtual<Alignment::UnalignedRight>(u64, s32);
	template void WriteVirtual<Alignment::UnalignedRight>(u64, s64);

	template u32 VirtualToPhysicalAddressUserMode32<Memory::Operation::Read>(u64, bool&);
	template u32 VirtualToPhysicalAddressUserMode32<Memory::Operation::Write>(u64, bool&);
	template u32 VirtualToPhysicalAddressUserMode64<Memory::Operation::Read>(u64, bool&);
	template u32 VirtualToPhysicalAddressUserMode64<Memory::Operation::Write>(u64, bool&);
	template u32 VirtualToPhysicalAddressSupervisorMode32<Memory::Operation::Read>(u64, bool&);
	template u32 VirtualToPhysicalAddressSupervisorMode32<Memory::Operation::Write>(u64, bool&);
	template u32 VirtualToPhysicalAddressSupervisorMode64<Memory::Operation::Read>(u64, bool&);
	template u32 VirtualToPhysicalAddressSupervisorMode64<Memory::Operation::Write>(u64, bool&);
	template u32 VirtualToPhysicalAddressKernelMode32<Memory::Operation::Read>(u64, bool&);
	template u32 VirtualToPhysicalAddressKernelMode32<Memory::Operation::Write>(u64, bool&);
	template u32 VirtualToPhysicalAddressKernelMode64<Memory::Operation::Read>(u64, bool&);
	template u32 VirtualToPhysicalAddressKernelMode64<Memory::Operation::Write>(u64, bool&);

	template u32 VirtualToPhysicalAddressTlb<Memory::Operation::Read>(u64);
	template u32 VirtualToPhysicalAddressTlb<Memory::Operation::Write>(u64);
}