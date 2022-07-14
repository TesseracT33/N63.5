module RSP:Interface;

import :Operation;

import DebugOptions;
import Logging;
import MI;
import N64;
import RDRAM;
import Util;

#include "../Utils/EnumerateTemplateSpecializations.h"

namespace RSP
{
	template<std::integral Int>
	Int CPUReadRegister(u32 addr)
	{
		if (addr == sp_pc_addr) {
			return halted ? pc : Int(Random<s32>(0, 0xFFF));
		}
		else {
			u32 reg_offset = (addr & 0x1F) >> 2;

			switch (reg_offset & 7) {
			case DmaSpaddr:
				return regs.dma_spaddr;

			case DmaRamaddr:
				return regs.dma_ramaddr;

			case DmaRdlen:
				/* SP_DMA_WRLEN and SP_DMA_RDLEN both always returns the same data on read, relative to
				the current transfer, irrespective on the direction of the transfer. */
				if (dma_in_progress) {
					return in_progress_dma_type == DmaType::RdToSp
						? regs.dma_rdlen : regs.dma_wrlen;
				}
				else {
					return regs.dma_rdlen;
				}

			case DmaWrlen:
				if (dma_in_progress) {
					return in_progress_dma_type == DmaType::RdToSp
						? regs.dma_rdlen : regs.dma_wrlen;
				}
				else {
					return regs.dma_wrlen;
				}

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


	template<size_t number_of_bytes>
	void CPUWriteRegister(u32 addr, auto data)
	{
		if (addr == sp_pc_addr) {
			pc = data & 0xFFC;
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

			case DmaRdlen:
				if (dma_in_progress) {
					buffered_dma_rdlen = data & 0xFF8F'FFF8;
					dma_is_pending = true;
					regs.dma_full |= 1;
					regs.status |= 8; /* Mirrors dma_full.0 */
					init_pending_dma_fun_ptr = InitDMA<DmaType::RdToSp>;
				}
				else {
					regs.dma_rdlen = data & 0xFF8F'FFF8;
					InitDMA<DmaType::RdToSp>();
				}
				break;

			case DmaWrlen:
				if (dma_in_progress) {
					buffered_dma_wrlen = data & 0xFF8F'FFF8;
					dma_is_pending = true;
					regs.dma_full |= 1;
					regs.status |= 8; /* Mirrors dma_full.0 */
					init_pending_dma_fun_ptr = InitDMA<DmaType::SpToRd>;
				}
				else {
					regs.dma_wrlen = data & 0xFF8F'FFF8;
					InitDMA<DmaType::SpToRd>();
				}
				break;

			case Status: {
				if ((word & 1) && !(word & 2)) {
					/* CLR_HALT: Start running RSP code from the current RSP PC (clear the HALTED flag) */
					regs.status &= ~1;
					halted = false;
				}
				else if (!(word & 1) && (word & 2)) {
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
				if ((word & 8) && !(word & 0x10)) {
					/* 	CLR_INTR: Acknowledge a pending RSP MI interrupt. This must be done any time a RSP MI interrupt
					was generated, otherwise the interrupt line on the VR4300 will stay asserted. */
					MI::ClearInterruptFlag(MI::InterruptType::SP);
				}
				else if (!(word & 8) && (word & 0x10)) {
					/* 	SET_INTR: Manually trigger a RSP MI interrupt on the VR4300. It might be useful if the RSP wants to
					manually trigger a VR4300 interrupt at any point during its execution. */
					MI::SetInterruptFlag(MI::InterruptType::SP);
				}
				if ((word & 0x20) && !(word & 0x40)) {
					/* CLR_SSTEP: Disable single-step mode. */
					single_step_mode = false;
				}
				else if (!(word & 0x20) && (word & 0x40)) {
					/* 	SET_SSTEP: Enable single-step mode. When this mode is activated, the RSP auto-halts itself after every opcode that is run.
					The VR4300 can then trigger a new step by unhalting it. */
					single_step_mode = true;
				}
				if ((word & 0x80) && !(word & 0x100)) {
					/* CLR_INTBREAK: Disable the INTBREAK flag. When this flag is disabled, running a BREAK opcode will not generate any
					RSP MI interrupt, but it will still halt the RSP. */
					regs.status &= ~0x40;
				}
				else if (!(word & 0x80) && (word & 0x100)) {
					/* 	SET_INTBREAK: Enable the INTBREAK flag. When this flag is enabled, running a BREAK opcode will generate
					a RSP MI interrupt, in addition to halting the RSP. */
					regs.status |= 0x40;
				}
				/* 	CLR_SIG<n>/SET_SIG<n>: Set to 0 or 1 the 8 available bitflags that can be used as communication protocol between RSP and CPU. */
				s32 written_value_mask = 0x200;
				s32 status_mask = 0x80;
				for (int i = 0; i < 8; ++i) {
					if ((word & written_value_mask) && !(word & written_value_mask << 1)) {
						regs.status &= ~status_mask;
					}
					else if (!(word & written_value_mask) && (word & written_value_mask << 1)) {
						regs.status |= status_mask;
					}
					written_value_mask <<= 2;
					status_mask <<= 1;
				}
				break;
			}

			case DmaFull: /* read-only */
				break;

			case DmaBusy: /* read-only */
				break;

			case Semaphore:
				regs.semaphore = word;
				break;

			default:
				std::unreachable();
			}
		}
	}


	template<DmaType dma_type>
	void InitDMA()
	{
		static_assert(dma_type == DmaType::RdToSp || dma_type == DmaType::SpToRd);

		dma_in_progress = true;
		in_progress_dma_type = dma_type;
		regs.dma_busy |= 1;
		regs.status |= 4; /* Mirrors dma_busy.0 */

		s32 rows, bytes_per_row, skip;
		u8* src_ptr, * dst_ptr;
		if constexpr (dma_type == DmaType::RdToSp) {
			bytes_per_row = (regs.dma_rdlen & 0xFFF) + 1;
			rows = (regs.dma_rdlen >> 12 & 0xFF) + 1;
			skip = regs.dma_rdlen >> 20 & 0xFFF;
			src_ptr = RDRAM::GetPointerToMemory(regs.dma_ramaddr);
			dst_ptr = RSP::GetPointerToMemory(regs.dma_spaddr);
		}
		else {
			bytes_per_row = (regs.dma_wrlen & 0xFFF) + 1;
			rows = (regs.dma_wrlen >> 12 & 0xFF) + 1;
			skip = regs.dma_wrlen >> 20 & 0xFFF;
			src_ptr = RSP::GetPointerToMemory(regs.dma_spaddr);
			dst_ptr = RDRAM::GetPointerToMemory(regs.dma_ramaddr);
		}

		s32 num_bytes_until_rdram_end = RDRAM::GetNumberOfBytesUntilMemoryEnd(regs.dma_ramaddr);
		s32 num_bytes_until_spram_end = 0x1000 - (regs.dma_spaddr & 0xFFF);

		s32 requested_total_bytes = rows * bytes_per_row;

		static constexpr uint cpu_cycles_per_byte = 18; /* TODO: essentially no idea about this */
		uint cpu_cycles_until_finish = cpu_cycles_per_byte * requested_total_bytes;

		N64::EnqueueEvent(N64::Event::SpDmaFinish, cpu_cycles_until_finish);

		if (skip == 0) {
			s32 bytes_to_copy = std::min(requested_total_bytes,
				std::min(num_bytes_until_rdram_end, num_bytes_until_spram_end));
			std::memcpy(dst_ptr, src_ptr, bytes_to_copy);
		}
		else {
			s32 requested_total_bytes_incl_skip = requested_total_bytes + skip * (rows - 1);
			s32 bytes_to_copy_incl_skip = std::min(requested_total_bytes_incl_skip,
				std::min(num_bytes_until_rdram_end, num_bytes_until_spram_end));
			for (s32 i = 0; i < rows; ++i) {
				s32 bytes_to_copy_this_row = std::min(bytes_to_copy_incl_skip, bytes_per_row);
				std::memcpy(dst_ptr, src_ptr, bytes_to_copy_this_row);
				bytes_to_copy_incl_skip -= bytes_to_copy_this_row;
				if (bytes_to_copy_incl_skip <= skip) {
					break;
				}
				bytes_to_copy_incl_skip -= skip;
				src_ptr += bytes_to_copy_this_row + skip;
				dst_ptr += bytes_to_copy_this_row + skip;
			}
		}

		if constexpr (log_dma) {
			std::string output = [&] {
				if constexpr (dma_type == DmaType::RdToSp) {
					return std::format("From RDRAM ${:X} to RSP MEM ${:X}; ${:X} bytes",
						regs.dma_ramaddr, regs.dma_spaddr, requested_total_bytes);
				}
				else {
					return std::format("From RSP MEM ${:X} to RDRAM ${:X}; ${:X} bytes",
						regs.dma_spaddr, regs.dma_ramaddr, requested_total_bytes);
				}
			}();
			Logging::LogDMA(output);
		}
	}


	void NotifyDmaFinish()
	{
		if (dma_is_pending) {
			dma_is_pending = false;
			regs.dma_full &= ~1;
			regs.status &= ~8; /* Mirrors dma_full.0 */
			regs.dma_rdlen = buffered_dma_rdlen;
			regs.dma_wrlen = buffered_dma_wrlen;
			init_pending_dma_fun_ptr();
		}
		else {
			dma_in_progress = false;
			regs.dma_busy &= ~1;
			regs.status &= ~4; /* Mirrors dma_busy.0 */
			if (in_progress_dma_type == DmaType::RdToSp) {
				/* After the transfer is finished, the field RDLEN contains the value 0xFF8,
				COUNT is reset to 0, and SKIP is unchanged. */
				regs.dma_rdlen = 0xFF8 | regs.dma_rdlen & 0xFF80'0000;
			}
			else {
				regs.dma_wrlen = 0xFF8 | regs.dma_wrlen & 0xFF80'0000;
			}
		}
	}


	ENUMERATE_TEMPLATE_SPECIALIZATIONS_READ(CPUReadRegister, u32)
	ENUMERATE_TEMPLATE_SPECIALIZATIONS_WRITE(CPUWriteRegister, u32)
}