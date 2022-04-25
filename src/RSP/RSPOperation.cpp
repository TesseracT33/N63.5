module RSP:RSPOperation;

#include "../Utils/EnumerateTemplateSpecializations.h"

namespace RSP
{
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
		std::memcpy(&instr_code, imem.data() + pc, sizeof u32); /* TODO: can pc be misaligned? */
		instr_code = std::byteswap(instr_code);
		pc = (pc + 4) & 0xFFF;
		DecodeExecuteInstruction(instr_code);
	}


	template<std::integral Int>
	Int CPUReadMemory(const u32 addr)
	{
		/* CPU precondition; the address is always aligned */
		static constexpr std::array mem = { dmem.data(), imem.data() };
		Int ret;
		std::memcpy(&ret, mem[bool(addr & 0x1000)] + (addr & 0xFFF), sizeof Int);
		return std::byteswap(ret);
	}


	template<std::size_t number_of_bytes>
	void CPUWriteMemory(u32 addr, auto data)
	{
		/* CPU precondition; the address may be misaligned, but then, 'number_of_bytes' is set so that it the write goes only to the next boundary. */
		static constexpr std::array mem = { dmem.data(), imem.data() };
		data = std::byteswap(data);
		std::memcpy(mem[bool(addr & 0x1000)] + (addr & 0xFFF), &data, number_of_bytes);
	}


	template<std::integral Int>
	Int ReadDMEM(u32 addr)
	{
		addr &= 0xFFF;
		if constexpr (sizeof Int == 1)
		{
			return dmem[addr];
		}
		else
		{
			Int ret;
			/* The read may be misaligned; we must check if it goes out of bounds. */
			if (addr + sizeof Int <= 0x1000)
			{
				std::memcpy(&ret, dmem.data() + addr, sizeof Int);
			}
			else
			{
				std::memcpy(&ret, dmem.data() + addr, 0x1000 - addr);
				std::memcpy(&ret, dmem.data(), sizeof Int - (0x1000 - addr));
			}
			return std::byteswap(ret);
		}
	}


	template<std::size_t number_of_bytes>
	void WriteDMEM(u32 addr, auto data)
	{
		addr &= 0xFFF;
		if constexpr (number_of_bytes == 1)
		{
			dmem[addr] = u8(data);
		}
		else
		{
			const auto byteswapped_data = std::byteswap(data);
			/* The write may be misaligned; we must check if it goes out of bounds. */
			if (addr + number_of_bytes <= 0x1000)
			{
				std::memcpy(dmem.data() + addr, &byteswapped_data, number_of_bytes);
			}
			else
			{
				std::memcpy(dmem.data() + addr, &byteswapped_data, 0x1000 - addr);
				std::memcpy(dmem.data(), &byteswapped_data, number_of_bytes - (0x1000 - addr));
			}
		}
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
	ENUMERATE_TEMPLATE_SPECIALIZATIONS_READ(ReadDMEM, u32)
	ENUMERATE_TEMPLATE_SPECIALIZATIONS_WRITE(WriteDMEM, u32)
}