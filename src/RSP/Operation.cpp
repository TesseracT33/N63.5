module RSP:Operation;

#include "../Utils/EnumerateTemplateSpecializations.h"

namespace RSP
{
	constexpr std::array memory_ptrs = { dmem.data(), imem.data() };


	void Run(const unsigned cycles_to_run)
	{
		p_cycle_counter = 0;

		while (p_cycle_counter < cycles_to_run)
		{
			if (jump_is_pending)
			{
				if (instructions_until_jump-- == 0)
				{
					pc = addr_to_jump_to;
					jump_is_pending = false;
				}
			}
			FetchDecodeExecuteInstruction();
		}
	}


	void FetchDecodeExecuteInstruction()
	{
		u32 instr_code;
		std::memcpy(&instr_code, imem.data() + pc, sizeof(u32)); /* TODO: can pc be misaligned? */
		instr_code = std::byteswap(instr_code);
		pc = (pc + 4) & 0xFFF;
		DecodeExecuteInstruction(instr_code);
	}


	template<std::integral Int>
	Int CPUReadMemory(const u32 addr)
	{
		/* CPU precondition; the address is always aligned */
		Int ret;
		std::memcpy(&ret, memory_ptrs[bool(addr & 0x1000)] + (addr & 0xFFF), sizeof(Int));
		return std::byteswap(ret);
	}


	template<std::size_t number_of_bytes>
	void CPUWriteMemory(const u32 addr, auto data)
	{
		/* CPU precondition; the address may be misaligned, but then, 'number_of_bytes' is set so that it the write goes only to the next boundary. */
		data = std::byteswap(data);
		std::memcpy(memory_ptrs[bool(addr & 0x1000)] + (addr & 0xFFF), &data, number_of_bytes);
	}


	template<std::integral Int>
	Int CPUReadRegister(u32 addr)
	{
		return Int(0);
	}


	template<std::size_t number_of_bytes>
	void CPUWriteRegister(u32 addr, auto data)
	{

	}


	template<std::integral Int>
	Int ReadDMEM(const u32 addr)
	{ 
		/* Addr may be misaligned and the read can go out of bounds */
		Int ret;
		for (std::size_t i = 0; i < sizeof(Int); ++i)
		{
			*((u8*)(&ret) + sizeof(Int) - i - 1) = dmem[(addr + i) & 0xFFF];
		}
		return ret;
	}


	template<std::integral Int>
	void WriteDMEM(const u32 addr, const Int data)
	{
		/* Addr may be misaligned and the write can go out of bounds */
		for (std::size_t i = 0; i < sizeof(Int); ++i)
		{
			dmem[(addr + i) & 0xFFF] = *((u8*)(&data) + sizeof(Int) - i - 1);
		}
	}


	u8* GetPointerToMemory(const u32 addr)
	{
		return memory_ptrs[bool(addr & 0x1000)] + (addr & 0xFFF);
	}


	template<u64 number_of_cycles>
	void AdvancePipeline()
	{
		p_cycle_counter++;
	}


	void PrepareJump(const u32 target_address)
	{
		jump_is_pending = true;
		instructions_until_jump = 1;
		addr_to_jump_to = target_address;
	}


	template void AdvancePipeline<1>();


	ENUMERATE_TEMPLATE_SPECIALIZATIONS_READ(CPUReadMemory, u32)
	ENUMERATE_TEMPLATE_SPECIALIZATIONS_WRITE(CPUWriteMemory, u32)
	ENUMERATE_TEMPLATE_SPECIALIZATIONS_READ(CPUReadRegister, u32)
	ENUMERATE_TEMPLATE_SPECIALIZATIONS_WRITE(CPUWriteRegister, u32)


	template u8 ReadDMEM<u8>(u32);
	template s8 ReadDMEM<s8>(u32);
	template u16 ReadDMEM<u16>(u32);
	template s16 ReadDMEM<s16>(u32);
	template u32 ReadDMEM<u32>(u32);
	template s32 ReadDMEM<s32>(u32);
	template u64 ReadDMEM<u64>(u32);
	template s64 ReadDMEM<s64>(u32);

	template void WriteDMEM<s8>(u32, s8);
	template void WriteDMEM<s16>(u32, s16);
	template void WriteDMEM<s32>(u32, s32);
}