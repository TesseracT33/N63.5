module MIPS4300i;

namespace MIPS4300i
{
	void MFHI(const u32 instr_code)
	{
		/* Move From HI;
		   Transfers the contents of special register HI to register rd. */
		const u8 rd = instr_code >> 11 & 0x1F;
		//GPR.Set(rd, HI);
	}


	void MFLO(const u32 instr_code)
	{
		/* Move From LO;
		   Transfers the contents of special register LO to register rd. */
		const u8 rd = instr_code >> 11 & 0x1F;
		//GPR.Set(rd, LO);
	}


	void MTHI(const u32 instr_code)
	{
		/* Move To HI;
		   Transfers the contents of register rs to special register HI. */
		const u8 rs = instr_code >> 21 & 0x1F;
		//HI = GPR.Get(rs);
	}


	void MTLO(const u32 instr_code)
	{
		/* Move To LO;
		   Transfers the contents of register rs to special register LO. */
		const u8 rs = instr_code >> 21 & 0x1F;
		//LO = GPR.Get(rs);
	}


	void CACHE(const u32 instr_code)
	{

	}


	void SYNC(const u32 instr_code)
	{
		/* Synchronize;
		   Completes the load/store instruction currently in the pipeline before the new
		   load/store instruction is executed. */

		   /* TODO */
	}


	void SYSCALL(const u32 instr_code)
	{
		/* System Call;
		   Generates a system call exception and transfers control to the exception processing program. */

		   /* TODO */
	}


	void BREAK(const u32 instr_code)
	{
		/* Breakpoint;
		   Generates a breakpoint exception and transfers control to the exception processing program. */

		   /* TODO */
	}


	void AddressErrorException()
	{

	}

	void IntegerOverflowException()
	{

	}

	void TrapException()
	{

	}
}