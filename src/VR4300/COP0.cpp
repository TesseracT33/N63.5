module VR4300:COP0;

import :CPU;
import :Exceptions;
import :Operation; 
import :Registers;
import :MMU;

import DebugOptions;
import MemoryAccess;

namespace VR4300
{
	constexpr std::array cop0_reg_str_repr = {
		"INDEX", "RANDOM", "ENTRY_LO_0", "ENTRY_LO_1", "CONTEXT", "PAGE_MASK", "WIRED", "COP0_7", "BAD_V_ADDR",
		"COUNT", "ENTRY_HI", "COMPARE", "STATUS", "CAUSE", "EPC", "PR_ID", "CONFIG", "LL_ADDR", "WATCH_LO",
		"WATCH_HI", "X_CONTEXT", "COP0_21", "COP0_22", "COP0_23", "COP0_24", "COP0_25", "PARITY_ERROR",
		"COP0_27", "TAG_LO", "TAG_HI", "ERROR_EPC"
	};
	static_assert(cop0_reg_str_repr.size() == 31);


	template<COP0Instruction instr>
	void COP0Move(const u32 instr_code)
	{
		using enum COP0Instruction;

		const auto rd = instr_code >> 11 & 0x1F;
		const auto rt = instr_code >> 16 & 0x1F;

		if constexpr (log_cpu_instructions)
		{
			current_instr_log_output = std::format("{} {}, {}", current_instr_name, rt, cop0_reg_str_repr[rd]);
		}

		if constexpr (instr == MTC0)
		{
			/* Move To System Control Coprocessor;
			   Loads the contents of the word of the general purpose register rt of the CPU
			   to the general purpose register rd of CP0. */
			cop0_reg.Set(rd, s32(gpr[rt]));
		}
		else if constexpr (instr == MFC0)
		{
			/* Move From System Control Coprocessor;
			   Loads the contents of the word of the general purpose register rd of CP0
			   to the general purpose register rt of the CPU. */
			gpr.Set(rt, s32(cop0_reg.Get(rd)));
		}
		else if constexpr (instr == DMTC0)
		{
			/* Doubleword Move To System Control Coprocessor;
			   Loads the contents of the doubleword of the general purpose register rt of the CPU
			   to the general purpose register rd of CP0. */
			cop0_reg.Set(rd, gpr[rt]);
		}
		else if constexpr (instr == DMFC0)
		{
			/* Doubleword Move From System Control Coprocessor;
			   Loads the contents of the doubleword of the general purpose register rd of CP0
			   to the general purpose register rt of the CPU. */
			gpr.Set(rt, cop0_reg.Get(rd)); /* TODO The operation of DMFC0 instruction on a 32-bit register of the CP0 is undefined */
		}
		else
		{
			static_assert(instr != instr, "\"CP0_Move\" template function called, but no matching move instruction was found.");
		}

		AdvancePipeline<1>();
	}


	void TLBP()
	{
		/* Translation Lookaside Buffer Probe;
		   Searches a TLB entry that matches with the contents of the entry Hi register and
		   sets the number of that TLB entry to the index register. If a TLB entry that
		   matches is not found, sets the most significant bit of the index register. */
		if constexpr (log_cpu_instructions)
		{
			current_instr_log_output = current_instr_name;
		}

		const auto tlb_index = std::find_if(tlb_entries.begin(), tlb_entries.end(),
			[](const auto& entry) {
				return entry.entry_hi.asid == cop0_reg.entry_hi.asid &&
					entry.entry_hi.vpn2 == cop0_reg.entry_hi.vpn2 &&
					entry.entry_hi.r == cop0_reg.entry_hi.r;
			});
		if (tlb_index == tlb_entries.end())
		{
			cop0_reg.index.p = 1;
		}
		else
		{
			cop0_reg.index.p = 0;
			cop0_reg.index.value = std::distance(tlb_entries.begin(), tlb_index);
		}

		AdvancePipeline<1>();
	}


	void TLBR()
	{
		/* Translation Lookaside Buffer Read;
		   The EntryHi and EntryLo registers are loaded with the contents of the TLB entry
		   pointed at by the contents of the Index register. The G bit (which controls ASID matching)
		   read from the TLB is written into both of the EntryLo0 and EntryLo1 registers. */
		if constexpr (log_cpu_instructions)
		{
			current_instr_log_output = current_instr_name;
		}

		const auto tlb_index = cop0_reg.index.value & 0x1F; /* bit 5 is not used */
		std::memcpy(&cop0_reg.entry_lo_0, &tlb_entries[tlb_index].entry_lo[0], 4);
		std::memcpy(&cop0_reg.entry_lo_1, &tlb_entries[tlb_index].entry_lo[1], 4);
		std::memcpy(&cop0_reg.entry_hi  , &tlb_entries[tlb_index].entry_hi   , 8);
		std::memcpy(&cop0_reg.page_mask , &tlb_entries[tlb_index].page_mask  , 4);
		cop0_reg.entry_hi.padding_of_zeroes = 0; /* entry_hi, unlike an TLB entry, does not have the G bit, but this is copied in from the memcpy. */
		cop0_reg.entry_lo_0.g = cop0_reg.entry_lo_1.g = tlb_entries[tlb_index].entry_hi.g;
		AdvancePipeline<1>();
	}


	void TLBWI()
	{
		/* Translation Lookaside Buffer Write Index;
		   The TLB entry pointed at by the Index register is loaded with the contents of the
		   EntryHi and EntryLo registers. The G bit of the TLB is written with the logical
		   AND of the G bits in the EntryLo0 and EntryLo1 registers. */
		if constexpr (log_cpu_instructions)
		{
			current_instr_log_output = current_instr_name;
		}

		const auto tlb_index = cop0_reg.index.value & 0x1F; /* bit 5 is not used */
		std::memcpy(&tlb_entries[tlb_index].entry_lo[0], &cop0_reg.entry_lo_0, 4);
		std::memcpy(&tlb_entries[tlb_index].entry_lo[1], &cop0_reg.entry_lo_1, 4);
		std::memcpy(&tlb_entries[tlb_index].entry_hi   , &cop0_reg.entry_hi  , 8);
		std::memcpy(&tlb_entries[tlb_index].page_mask  , &cop0_reg.page_mask , 4);
		tlb_entries[tlb_index].entry_hi.g = cop0_reg.entry_lo_0.g && cop0_reg.entry_lo_1.g;
		/* Compute things that will make the virtual-to-physical-address process faster. */
		const auto addr_offset_bit_length = page_size_to_addr_offset_bit_length[cop0_reg.page_mask.value];
		tlb_entries[tlb_index].address_vpn2_mask = 0xFF'FFFF'FFFF << addr_offset_bit_length & 0xFF'FFFF'FFFF;
		tlb_entries[tlb_index].address_offset_mask = (1 << addr_offset_bit_length) - 1;
		tlb_entries[tlb_index].address_vpn_even_odd_mask = tlb_entries[tlb_index].address_offset_mask + 1;
		tlb_entries[tlb_index].vpn2_shifted = tlb_entries[tlb_index].entry_hi.vpn2 << addr_offset_bit_length;

		AdvancePipeline<1>();
	}


	void TLBWR()
	{
		/* Translation Lookaside Buffer Write Random;
		   The TLB entry pointed at by the Random register is loaded with the contents of
		   the EntryHi and EntryLo registers. The G bit of the TLB is written with the logical
		   AND of the G bits in the EntryLo0 and EntryLo1 registers.
		   The 'wired' register determines which TLB entries cannot be overwritten. */
		if constexpr (log_cpu_instructions)
		{
			current_instr_log_output = current_instr_name;
		}

		const auto tlb_index = cop0_reg.random.value & 0x1F; /* bit 5 is not used */
		const auto tlb_wired_index = cop0_reg.wired.value & 0x1F;
		if (tlb_index < tlb_wired_index) /* TODO: <= ? */
			return;
		std::memcpy(&tlb_entries[tlb_index].entry_lo[0], &cop0_reg.entry_lo_0, 4);
		std::memcpy(&tlb_entries[tlb_index].entry_lo[1], &cop0_reg.entry_lo_1, 4);
		std::memcpy(&tlb_entries[tlb_index].entry_hi   , &cop0_reg.entry_hi  , 8);
		std::memcpy(&tlb_entries[tlb_index].page_mask  , &cop0_reg.page_mask , 4);
		tlb_entries[tlb_index].entry_hi.g = cop0_reg.entry_lo_0.g && cop0_reg.entry_lo_1.g;

		const auto addr_offset_bit_length = page_size_to_addr_offset_bit_length[cop0_reg.page_mask.value];
		tlb_entries[tlb_index].address_vpn2_mask = 0xFF'FFFF'FFFF << addr_offset_bit_length & 0xFF'FFFF'FFFF;
		tlb_entries[tlb_index].address_offset_mask = (1 << addr_offset_bit_length) - 1;
		tlb_entries[tlb_index].address_vpn_even_odd_mask = tlb_entries[tlb_index].address_offset_mask + 1;
		tlb_entries[tlb_index].vpn2_shifted = tlb_entries[tlb_index].entry_hi.vpn2 << addr_offset_bit_length;

		AdvancePipeline<1>();
	}


	void ERET()
	{
		/* Return From Exception;
		   Returns from an exception, interrupt, or error trap. */
		if constexpr (log_cpu_instructions)
		{
			current_instr_log_output = current_instr_name;
		}

		if (cop0_reg.status.erl == 0)
		{
			pc = cop0_reg.epc.value;
			cop0_reg.status.exl = 0;
		}
		else
		{
			pc = cop0_reg.error_epc.value;
			cop0_reg.status.erl = 0;
		}
		ll_bit = 0;

		/* Check if the pc is misaligned, and if so, signal an exception right away.
		   Then, there is no need to check if the pc is misaligned every time an instruction is fetched
		   (this is the only place where the pc can be set to anything, since epc and error_epc are writeable?). */
		if (pc & 3)
			SignalAddressErrorException<MemoryAccess::Operation::InstrFetch>(pc);

		AdvancePipeline<1>();
	}


	void CACHE(const u32 instr_code)
	{
		/* Cache op;
		   Sign-extends the 16-bit offset to 32 bits and adds it to register base to
		   generate a virtual address. The virtual address is converted into a physical
		   address by using the TLB, and a cache operation indicated by a 5-bit sub op
		   code is executed to that address. */
		/* Currently not emulated. */
		if constexpr (log_cpu_instructions)
		{
			current_instr_log_output = current_instr_name;
		}
		AdvancePipeline<1>();
	}


	template void COP0Move<COP0Instruction::MTC0>(u32);
	template void COP0Move<COP0Instruction::MFC0>(u32);
	template void COP0Move<COP0Instruction::DMTC0>(u32);
	template void COP0Move<COP0Instruction::DMFC0>(u32);
}