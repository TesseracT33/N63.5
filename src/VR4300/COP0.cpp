module VR4300;

namespace VR4300
{
	template<CP0_Instr instr>
	void CP0_Move(const u32 instr_code)
	{
		const u8 rd = instr_code >> 11 & 0x1F;
		const u8 rt = instr_code >> 16 & 0x1F;

		if constexpr (instr == CP0_Instr::MTC0)
		{
			/* Move To System Control Coprocessor;
			   Loads the contents of the word of the general purpose register rt of the CPU
			   to the general purpose register rd of CP0. */
			CP0_GPR.Set(rd, GPR[rt] & 0xFFFFFFFF);
		}
		else if constexpr (instr == CP0_Instr::MFC0)
		{
			/* Move From System Control Coprocessor;
			   Loads the contents of the word of the general purpose register rd of CP0
			   to the general purpose register rt of the CPU. */
			GPR.Set(rt, s64(CP0_GPR.Get(rd)));
		}
		else if constexpr (instr == CP0_Instr::DMTC0)
		{
			/* Doubleword Move To System Control Coprocessor;
			   Loads the contents of the doubleword of the general purpose register rt of the CPU
			   to the general purpose register rd of CP0. */
			CP0_GPR.Set(rd, GPR[rt]);
		}
		else if constexpr (instr == CP0_Instr::DMFC0)
		{
			/* Doubleword Move From System Control Coprocessor;
			   Loads the contents of the doubleword of the general purpose register rd of CP0
			   to the general purpose register rt of the CPU. */
			GPR.Set(rt, CP0_GPR.Get(rd)); /* TODO The operation of DMFC0 instruction on a 32-bit register of the CP0 is undefined */
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
	}


	void TLBR()
	{
		/* Translation Lookaside Buffer Read;
		   The EntryHi and EntryLo registers are loaded with the contents of the TLB entry
		   pointed at by the contents of the Index register. The G bit (which controls ASID matching)
		   read from the TLB is written into both of the EntryLo0 and EntryLo1 registers. */
	}


	void TLBWI()
	{
		/* Translation Lookaside Buffer Write Index;
		   The TLB entry pointed at by the Index register is loaded with the contents of the
		   EntryHi and EntryLo registers. The G bit of the TLB is written with the logical
		   AND of the G bits in the EntryLo0 and EntryLo1 registers. */

	}


	void TLBWR()
	{
		/* Translation Lookaside Buffer Write Random;
		   The TLB entry pointed at by the Random register is loaded with the contents of
		   the EntryHi and EntryLo registers. The G bit of the TLB is written with the logical
		   AND of the G bits in the EntryLo0 and EntryLo1 registers. */
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
	}


	template void CP0_Move<CP0_Instr::MTC0>(const u32 instr_code);
	template void CP0_Move<CP0_Instr::MFC0>(const u32 instr_code);
	template void CP0_Move<CP0_Instr::DMTC0>(const u32 instr_code);
	template void CP0_Move<CP0_Instr::DMFC0>(const u32 instr_code);
}