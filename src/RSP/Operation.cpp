module RSP:Operation;

import DebugOptions;
import DMA;
import MI;

#include "../Utils/EnumerateTemplateSpecializations.h"

namespace RSP
{
	void Run(const uint cycles_to_run)
	{
		if (halted) {
			return;
		}

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

			if (single_step_mode) {
				return;
			}
		}
	}


	void PowerOn()
	{
		halted = jump_is_pending = single_step_mode = false;
		pc = 0;
		dmem.fill(0);
		imem.fill(0);
		std::memset(&regs, 0, sizeof(decltype(regs)));
	}


	void FetchDecodeExecuteInstruction()
	{
		if constexpr (log_rsp_instructions)
		{
			current_instr_pc = pc;
		}
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
		if (addr == sp_pc_addr) {
			return pc;
		}
		else {
			u32 reg_offset = (addr & 0x1F) >> 2;

			switch (reg_offset & 7) {
			case DmaSpaddr:
				return regs.dma_spaddr;

			case DmaRamaddr:
				return regs.dma_ramaddr;

			case DmaRdlen:
				return regs.dma_rdlen;

			case DmaWrlen:
				return regs.dma_wrlen;

			case Status:
				return regs.status;

			case DmaFull:
				return regs.dma_full;

			case DmaBusy:
				return regs.dma_busy;

			case Semaphore: {
				auto ret = regs.semaphore;
				regs.semaphore |= 1;
				return ret;
			}

			default:
				std::unreachable();
			}
		}
	}


	template<std::size_t number_of_bytes>
	void CPUWriteRegister(u32 addr, auto data)
	{
		if (addr == sp_pc_addr) {
			pc = data & 0xFFF;
		}
		else {
			u32 reg_offset = (addr & 0x1F) >> 2;

			/* Force 'data' to be a word */
			s32 word = s32(data);

			switch (reg_offset & 7) {
			case DmaSpaddr:
				regs.dma_spaddr = data &= ~7;
				break;

			case DmaRamaddr:
				regs.dma_ramaddr = data &= ~7;
				break;

			case DmaRdlen: {
				regs.dma_rdlen = data &= 0xFF8F'FFF8;
				auto bytes_per_row = (regs.dma_rdlen & 0xFFF) + 1;
				auto rows = (regs.dma_rdlen >> 12 & 0xFF) + 1;
				auto skip = regs.dma_rdlen >> 20 & 0xFFF;
				DMA::Init<DMA::Type::SP, DMA::Location::RDRAM, DMA::Location::SPRAM>(
					rows, bytes_per_row, skip, regs.dma_ramaddr, regs.dma_spaddr);
				regs.status |= 4;
				regs.dma_busy |= 1;
				break;
			}

			case DmaWrlen: {
				regs.dma_wrlen = data &= 0xFF8F'FFF8;
				auto bytes_per_row = (regs.dma_wrlen & 0xFFF) + 1;
				auto rows = (regs.dma_wrlen >> 12 & 0xFF) + 1;
				auto skip = regs.dma_wrlen >> 20 & 0xFFF;
				DMA::Init<DMA::Type::SP, DMA::Location::SPRAM, DMA::Location::RDRAM>(
					rows, bytes_per_row, skip, regs.dma_spaddr, regs.dma_ramaddr);
				regs.status |= 4;
				regs.dma_busy |= 1;
				break;
			}

			case Status: {
				if (word & 1) {
					/* CLR_HALT: Start running RSP code from the current RSP PC (clear the HALTED flag) */
					regs.status &= ~1;
					halted = false;
				}
				else if (word & 2) {
					/* 	SET_HALT: Pause running RSP code (set the HALTED flag) */
					regs.status |= 1;
					halted = true;
				}
				if (word & 4) {
					/* CLR_BROKE: Clear the BROKE flag, that is automatically set every time a BREAK opcode is run.
					This flag has no effect on the running/idle state of the RSP; it is just a latch
					that remembers whether a BREAK opcode was ever run. */
					regs.status &= ~2;
				}
				if (word & 8) {
					/* 	CLR_INTR: Acknowledge a pending RSP MI interrupt. This must be done any time a RSP MI interrupt
					was generated, otherwise the interrupt line on the VR4300 will stay asserted. */
					MI::ClearInterruptFlag<MI::InterruptType::SP>();
				}
				else if (word & 0x10) {
					/* 	SET_INTR: Manually trigger a RSP MI interrupt on the VR4300. It might be useful if the RSP wants to
					manually trigger a VR4300 interrupt at any point during its execution. */
					MI::SetInterruptFlag<MI::InterruptType::SP>();
				}
				if (word & 0x20) {
					/* CLR_SSTEP: Disable single-step mode. */
					single_step_mode = false;
				}
				else if (word & 0x40) {
					/* 	SET_SSTEP: Enable single-step mode. When this mode is activated, the RSP auto-halts itself after every opcode that is run.
					The VR4300 can then trigger a new step by unhalting it. */
					single_step_mode = true;
				}
				if (word & 0x80) {
					/* CLR_INTBREAK: Disable the INTBREAK flag. When this flag is disabled, running a BREAK opcode will not generate any
					RSP MI interrupt, but it will still halt the RSP. */
					regs.status &= ~0x40;
				}
				else if (word & 0x100) {
					/* 	SET_INTBREAK: Enable the INTBREAK flag. When this flag is enabled, running a BREAK opcode will generate
					a RSP MI interrupt, in addition to halting the RSP. */
					regs.status |= 0x40;
				}
				/* 	CLR_SIG<n>/SET_SIG<n>: Set to 0 or 1 the 8 available bitflags that can be used as communication protocol between RSP and CPU. */
				s32 written_value_mask = 0x200;
				s32 status_mask = 0x80;
				for (int i = 0; i < 8; ++i) {
					if (word & written_value_mask) {
						regs.status &= ~status_mask;
					}
					else if (word & written_value_mask << 1) {
						regs.status |= status_mask;
					}
					written_value_mask <<= 2;
					status_mask <<= 1;
				}
				break;
			}

			case DmaFull:
				regs.dma_full = word; /* bit 0 is mirror of bit 3 in status */
				regs.status &= ~8;
				regs.status |= (regs.dma_full & 1) << 3;
				break;

			case DmaBusy:
				regs.dma_busy = word; /* bit 0 is mirror of bit 2 in status */
				regs.status &= ~4;
				regs.status |= (regs.dma_busy & 1) << 2;
				break;

			case Semaphore:
				regs.semaphore = word;
				break;

			default:
				std::unreachable();
			}
		}
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