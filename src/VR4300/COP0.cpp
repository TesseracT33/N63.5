module VR4300:COP0;

import :CPU;
import :Registers;
import :MMU;

import MemoryAccess;

namespace VR4300
{
	template<CP0_Instruction instr>
	void CP0_Move(const u32 instr_code)
	{
		using enum CP0_Instruction;

		const u8 rd = instr_code >> 11 & 0x1F;
		const u8 rt = instr_code >> 16 & 0x1F;

		if constexpr (instr == MTC0)
		{
			/* Move To System Control Coprocessor;
			   Loads the contents of the word of the general purpose register rt of the CPU
			   to the general purpose register rd of CP0. */
			COP0_reg.Set(rd, s32(GPR[rt]));
		}
		else if constexpr (instr == MFC0)
		{
			/* Move From System Control Coprocessor;
			   Loads the contents of the word of the general purpose register rd of CP0
			   to the general purpose register rt of the CPU. */
			GPR.Set(rt, s32(COP0_reg.Get(rd)));
		}
		else if constexpr (instr == DMTC0)
		{
			/* Doubleword Move To System Control Coprocessor;
			   Loads the contents of the doubleword of the general purpose register rt of the CPU
			   to the general purpose register rd of CP0. */
			COP0_reg.Set(rd, GPR[rt]);
		}
		else if constexpr (instr == DMFC0)
		{
			/* Doubleword Move From System Control Coprocessor;
			   Loads the contents of the doubleword of the general purpose register rd of CP0
			   to the general purpose register rt of the CPU. */
			GPR.Set(rt, COP0_reg.Get(rd)); /* TODO The operation of DMFC0 instruction on a 32-bit register of the CP0 is undefined */
		}
		else
		{
			static_assert(false, "\"CP0_Move\" template function called, but no matching move instruction was found.");
		}
	}


	void TLBP()
	{
		/* Translation Lookaside Buffer Probe;
		   Searches a TLB entry that matches with the contents of the entry Hi register and
		   sets the number of that TLB entry to the index register. If a TLB entry that
		   matches is not found, sets the most significant bit of the index register. */
		const auto TLB_index = std::find_if(std::begin(TLB_entries), std::end(TLB_entries),
			[&](const auto& entry) {
				return entry.ASID == COP0_reg.entry_hi.ASID && entry.VPN2 == COP0_reg.entry_hi.VPN2 && entry.R == COP0_reg.entry_hi.R;
			});

		if (TLB_index == std::end(TLB_entries))
		{
			COP0_reg.index.P = 1;
		}
		else
		{
			COP0_reg.index.index = std::distance(std::begin(TLB_entries), TLB_index);
			COP0_reg.index.P = 0;
		}
	}


	void TLBR()
	{
		/* Translation Lookaside Buffer Read;
		   The EntryHi and EntryLo registers are loaded with the contents of the TLB entry
		   pointed at by the contents of the Index register. The G bit (which controls ASID matching)
		   read from the TLB is written into both of the EntryLo0 and EntryLo1 registers. */
		const unsigned TLB_index = COP0_reg.index.index & 0x1F; /* bit 5 is not used */
		const std::byte* arr = (std::byte*)(&TLB_entries[TLB_index]);
		std::memcpy(&COP0_reg.entry_lo_0, arr, 4);
		std::memcpy(&COP0_reg.entry_lo_1, arr + 4, 4);
		std::memcpy(&COP0_reg.entry_hi, arr + 8, 4);
		std::memcpy(&COP0_reg.page_mask, arr + 12, 4);
		COP0_reg.entry_hi.padding_of_zeroes = 0; /* entry_hi, unlike an TLB entry, does not have the G bit, but this is copied in from the memcpy. */
		COP0_reg.entry_lo_0.G = COP0_reg.entry_lo_1.G = TLB_entries[TLB_index].G;
	}


	void TLBWI()
	{
		/* Translation Lookaside Buffer Write Index;
		   The TLB entry pointed at by the Index register is loaded with the contents of the
		   EntryHi and EntryLo registers. The G bit of the TLB is written with the logical
		   AND of the G bits in the EntryLo0 and EntryLo1 registers. */
		const unsigned TLB_index = COP0_reg.index.index & 0x1F; /* bit 5 is not used */
		std::byte* arr = (std::byte*)(&TLB_entries[TLB_index]);
		std::memcpy(arr, &COP0_reg.entry_lo_0, 4);
		std::memcpy(arr + 4, &COP0_reg.entry_lo_1, 4);
		std::memcpy(arr + 8, &COP0_reg.entry_hi, 4);
		std::memcpy(arr + 12, &COP0_reg.page_mask, 4);
		TLB_entries[TLB_index].G = COP0_reg.entry_lo_0.G && COP0_reg.entry_lo_1.G;
	}


	void TLBWR()
	{
		/* Translation Lookaside Buffer Write Random;
		   The TLB entry pointed at by the Random register is loaded with the contents of
		   the EntryHi and EntryLo registers. The G bit of the TLB is written with the logical
		   AND of the G bits in the EntryLo0 and EntryLo1 registers.
		   The 'wired' register determines which TLB entries cannot be overwritten. */
		const unsigned TLB_index = COP0_reg.random.random & 0x1F; /* bit 5 is not used */
		const unsigned TLB_wired_index = COP0_reg.wired & 0x1F;
		if (TLB_index < TLB_wired_index) /* TODO: <= ? */
			return;

		std::byte* arr = (std::byte*)(&TLB_entries[TLB_index]);
		std::memcpy(arr, &COP0_reg.entry_lo_0, 4);
		std::memcpy(arr + 4, &COP0_reg.entry_lo_1, 4);
		std::memcpy(arr + 8, &COP0_reg.entry_hi, 4);
		std::memcpy(arr + 12, &COP0_reg.page_mask, 4);
		TLB_entries[TLB_index].G = COP0_reg.entry_lo_0.G && COP0_reg.entry_lo_1.G;
	}


	void ERET()
	{
		/* Return From Exception;
		   Returns from an exception, interrupt, or error trap. */
	}


	void CACHE(const u32 instr_code)
	{
		/* Cache op;
		   Sign-extends the 16-bit offset to 32 bits and adds it to register base to
		   generate a virtual address. The virtual address is converted into a physical
		   address by using the TLB, and a cache operation indicated by a 5-bit sub op
		   code is executed to that address. */
		const s16 offset = instr_code & 0xFFFF;
		const u8 op = instr_code >> 16 & 0x1F;
		const u8 base = instr_code >> 21 & 0x1F;
		const u64 virt_addr = GPR[base] + offset;
		const u64 phys_addr = VirtualToPhysicalAddress<MemoryAccess::Operation::Read>(virt_addr);
	}


	template void CP0_Move<CP0_Instruction::MTC0>(const u32 instr_code);
	template void CP0_Move<CP0_Instruction::MFC0>(const u32 instr_code);
	template void CP0_Move<CP0_Instruction::DMTC0>(const u32 instr_code);
	template void CP0_Move<CP0_Instruction::DMFC0>(const u32 instr_code);
}