module MIPS4300i;

namespace MIPS4300i
{
	void MTC0(const u32 instr_code)
	{
		const u8 rd = instr_code >> 11 & 0x1F;
		const u8 rt = instr_code >> 16 & 0x1F;
	}


	void MFC0(const u32 instr_code)
	{

	}


	void DMTC0(const u32 instr_code)
	{

	}


	void DMFC0(const u32 instr_code)
	{

	}


	void TLBR()
	{

	}


	void TLBWI()
	{

	}


	void TLBWR()
	{

	}


	void TLBP()
	{

	}


	void ERET()
	{

	}

}