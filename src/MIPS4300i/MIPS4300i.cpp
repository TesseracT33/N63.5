module MIPS4300i;

namespace MIPS4300i
{
	void Run(const int cycles)
	{

	}

	void Reset()
	{

	}

	void ExecuteInstruction() /* todo: bad name for now */
	{
		const u32 instr_code = MMU::cpu_read_mem<u32>(PC);
		PC += 4;
		DecodeAndExecuteInstruction(instr_code);
	}
}