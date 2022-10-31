module RSP:Interface;

import :Operation;

import DebugOptions;
import Logging;
import MI;
import RDRAM;
import Scheduler;

namespace RSP
{
	template<DmaType dma_type>
	void InitDMA()
	{
		static_assert(dma_type == DmaType::RdToSp || dma_type == DmaType::SpToRd);

		dma_in_progress = true;
		in_progress_dma_type = dma_type;
		sp.dma_busy |= 1;
		sp.status.dma_busy = 1; /* Mirrors dma_busy.0 */

		s32 rows, bytes_per_row, skip;
		u8* src_ptr, * dst_ptr;
		if constexpr (dma_type == DmaType::RdToSp) {
			/* The number of bytes is RDLEN plus 1, rounded up to 8 bytes.
			Since the lower three bits of RDLEN are always 0, this effectively results
			in the number of bytes always being rounded up to the next 8-byte multiple. */
			bytes_per_row = (sp.dma_rdlen & 0xFF8) + 8;
			rows = (sp.dma_rdlen >> 12 & 0xFF) + 1;
			skip = sp.dma_rdlen >> 20 & 0xFFF;
			src_ptr = RDRAM::GetPointerToMemory(sp.dma_ramaddr);
			dst_ptr = RSP::GetPointerToMemory(sp.dma_spaddr);
		}
		else {
			bytes_per_row = (sp.dma_wrlen & 0xFF8) + 8;
			rows = (sp.dma_wrlen >> 12 & 0xFF) + 1;
			skip = sp.dma_wrlen >> 20 & 0xFFF;
			src_ptr = RSP::GetPointerToMemory(sp.dma_spaddr);
			dst_ptr = RDRAM::GetPointerToMemory(sp.dma_ramaddr);
		}

		s32 num_bytes_until_rdram_end = RDRAM::GetNumberOfBytesUntilMemoryEnd(sp.dma_ramaddr);
		s32 num_bytes_until_spram_end = 0x1000 - (sp.dma_spaddr & 0xFFF);

		s32 requested_total_bytes = rows * bytes_per_row;
		s32 bytes_to_copy = std::min(requested_total_bytes,
			std::min(num_bytes_until_rdram_end, num_bytes_until_spram_end));

		/* The speed of transfer is about 3.7 bytes per VR4300 (PClock) cycle (plus some small fixed overhead). */
		static constexpr uint cpu_cycles_per_byte = 4;
		uint cpu_cycles_until_finish = cpu_cycles_per_byte * requested_total_bytes;

		Scheduler::AddEvent(Scheduler::EventType::SpDmaFinish, cpu_cycles_until_finish, OnDmaFinish);

		/* The DMA engine allows to transfer multiple "rows" of data in RDRAM, separated by a "skip" value. This allows for instance to transfer
		a rectangular portion of a larger image, by specifying the size of each row of the selection portion, the number of rows, and a "skip" value
		that corresponds to the bytes between the end of a row and the beginning of the following one. Notice that this applies only to RDRAM: accesses in IMEM/DMEM are always linear. */
		if (skip == 0) {
			std::memcpy(dst_ptr, src_ptr, bytes_to_copy);
		}
		else {
			for (s32 i = 0; i < rows; ++i) {
				s32 bytes_to_copy_this_row = std::min(bytes_to_copy, bytes_per_row);
				std::memcpy(dst_ptr, src_ptr, bytes_to_copy_this_row);
				bytes_to_copy -= bytes_to_copy_this_row;
				src_ptr += bytes_to_copy_this_row;
				dst_ptr += bytes_to_copy_this_row;
				if constexpr (dma_type == DmaType::RdToSp) {
					src_ptr += skip;
				}
				else {
					dst_ptr += skip;
				}
			}
		}

		if constexpr (log_dma) {
			std::string_view rsp_mem_bank = sp.dma_spaddr & 0x1000
				? "IMEM" : "DMEM";
			std::string output = [&] {
				if constexpr (dma_type == DmaType::RdToSp) {
					return std::format("From RDRAM ${:X} to RSP {} ${:X}; ${:X} bytes",
						sp.dma_ramaddr & 0xFF'FFFF, rsp_mem_bank, sp.dma_spaddr & 0xFFF, requested_total_bytes);
				}
				else {
					return std::format("From RSP {} ${:X} to RDRAM ${:X}; ${:X} bytes",
						rsp_mem_bank, sp.dma_spaddr & 0xFFF, sp.dma_ramaddr & 0xFF'FFFF, requested_total_bytes);
				}
			}();
			LogDma(output);
		}
	}


	void OnDmaFinish()
	{
		if (dma_is_pending) {
			dma_is_pending = false;
			sp.dma_full &= ~1;
			sp.status.dma_full = 0; /* Mirrors dma_full.0 */
			if (in_progress_dma_type == DmaType::RdToSp) {
				sp.dma_rdlen = buffered_dma_rdlen;
				sp.dma_wrlen = buffered_dma_rdlen;
			}
			else {
				sp.dma_rdlen = buffered_dma_wrlen;
				sp.dma_wrlen = buffered_dma_wrlen;
			}
			init_pending_dma_fun_ptr();
		}
		else {
			dma_in_progress = false;
			sp.dma_busy &= ~1;
			sp.status.dma_busy = 0; /* Mirrors dma_busy.0 */
			if (in_progress_dma_type == DmaType::RdToSp) {
				/* After the transfer is finished, the fields RDLEN and WRLEN contains the value 0xFF8,
				COUNT is reset to 0, and SKIP is unchanged. */
				sp.dma_rdlen = 0xFF8 | sp.dma_rdlen & 0xFF80'0000;
				sp.dma_wrlen = 0xFF8 | sp.dma_rdlen & 0xFF80'0000;
			}
			else {
				sp.dma_rdlen = 0xFF8 | sp.dma_wrlen & 0xFF80'0000;
				sp.dma_wrlen = 0xFF8 | sp.dma_wrlen & 0xFF80'0000;
			}
		}

		MI::SetInterruptFlag(MI::InterruptType::SP);
	}


	s32 ReadReg(u32 addr)
	{
		if (addr == sp_pc_addr) {
			// TODO: return random number if !halted, else pc
			//return halted ? pc : Int(Random<s32>(0, 0xFFF));
			return pc;
		}
		else {
			static_assert(sizeof(sp) >> 2 == 8);
			u32 offset = addr >> 2 & 7;

			switch (offset & 7) {
			case DmaSpaddr:
				return sp.dma_spaddr;

			case DmaRamaddr:
				return sp.dma_ramaddr;

			case DmaRdlen:
				/* SP_DMA_WRLEN and SP_DMA_RDLEN both always returns the same data on read, relative to
				the current transfer, irrespective on the direction of the transfer. */
				return ~7 & [&] {
					if (dma_in_progress) {
						return in_progress_dma_type == DmaType::RdToSp
							? sp.dma_rdlen : sp.dma_wrlen;
					}
					else {
						return sp.dma_rdlen;
					}
				}();

			case DmaWrlen:
				return ~7 & [&] {
					if (dma_in_progress) {
						return in_progress_dma_type == DmaType::RdToSp
							? sp.dma_rdlen : sp.dma_wrlen;
					}
					else {
						return sp.dma_wrlen;
					}
				}();

			case Status:
				return std::bit_cast<u32>(sp.status);

			case DmaFull:
				return sp.dma_full;

			case DmaBusy:
				return sp.dma_busy;

			case Semaphore: {
				auto ret = sp.semaphore;
				sp.semaphore = 1;
				return ret;
			}

			default:
				std::unreachable();
			}
		}
	}


	void WriteReg(u32 addr, s32 data)
	{
		if (addr == sp_pc_addr) {
			pc = data & 0xFFC;
		}
		else {
			static_assert(sizeof(sp) >> 2 == 8);
			u32 offset = addr >> 2 & 7;

			switch (offset) {
			case DmaSpaddr:
				sp.dma_spaddr = data & 0x03FF'FFF8;
				break;

			case DmaRamaddr:
				sp.dma_ramaddr = data &= 0x03FF'FFF8;
				break;

			case DmaRdlen:
				if (dma_in_progress) {
					buffered_dma_rdlen = data & 0xFF8F'FFFF;
					dma_is_pending = true;
					sp.dma_full |= 1;
					sp.status.dma_full = 1; /* Mirrors dma_full.0 */
					init_pending_dma_fun_ptr = InitDMA<DmaType::RdToSp>;
				}
				else {
					sp.dma_rdlen = data & 0xFF8F'FFFF;
					InitDMA<DmaType::RdToSp>();
				}
				break;

			case DmaWrlen:
				if (dma_in_progress) {
					buffered_dma_wrlen = data & 0xFF8F'FFFF;
					dma_is_pending = true;
					sp.dma_full |= 1;
					sp.status.dma_full = 1; /* Mirrors dma_full.0 */
					init_pending_dma_fun_ptr = InitDMA<DmaType::SpToRd>;
				}
				else {
					sp.dma_wrlen = data & 0xFF8F'FFFF;
					InitDMA<DmaType::SpToRd>();
				}
				break;

			case Status: {
				if ((data & 1) && !(data & 2)) {
					/* CLR_HALT: Start running RSP code from the current RSP PC (clear the HALTED flag) */
					sp.status.halted = 0;
				}
				else if (!(data & 1) && (data & 2)) {
					/* 	SET_HALT: Pause running RSP code (set the HALTED flag) */
					sp.status.halted = 1;
				}
				if (data & 4) {
					/* CLR_BROKE: Clear the BROKE flag, that is automatically set every time a BREAK opcode is run.
					This flag has no effect on the running/idle state of the RSP; it is just a latch
					that remembers whether a BREAK opcode was ever run. */
					sp.status.broke = 0;
				}
				if ((data & 8) && !(data & 0x10)) {
					/* 	CLR_INTR: Acknowledge a pending RSP MI interrupt. This must be done any time a RSP MI interrupt
					was generated, otherwise the interrupt line on the VR4300 will stay asserted. */
					MI::ClearInterruptFlag(MI::InterruptType::SP);
				}
				else if (!(data & 8) && (data & 0x10)) {
					/* 	SET_INTR: Manually trigger a RSP MI interrupt on the VR4300. It might be useful if the RSP wants to
					manually trigger a VR4300 interrupt at any point during its execution. */
					MI::SetInterruptFlag(MI::InterruptType::SP);
				}
				if ((data & 0x20) && !(data & 0x40)) {
					/* CLR_SSTEP: Disable single-step mode. */
					sp.status.sstep = 0;
				}
				else if (!(data & 0x20) && (data & 0x40)) {
					/* 	SET_SSTEP: Enable single-step mode. When this mode is activated, the RSP auto-halts itself after every opcode that is run.
					The VR4300 can then trigger a new step by unhalting it. */
					sp.status.sstep = 1;
				}
				if ((data & 0x80) && !(data & 0x100)) {
					/* CLR_INTBREAK: Disable the INTBREAK flag. When this flag is disabled, running a BREAK opcode will not generate any
					RSP MI interrupt, but it will still halt the RSP. */
					sp.status.intbreak = 0;
				}
				else if (!(data & 0x80) && (data & 0x100)) {
					/* 	SET_INTBREAK: Enable the INTBREAK flag. When this flag is enabled, running a BREAK opcode will generate
					a RSP MI interrupt, in addition to halting the RSP. */
					sp.status.intbreak = 1;
				}
				/* 	CLR_SIG<n>/SET_SIG<n>: Set to 0 or 1 the 8 available bitflags that can be used as communication protocol between RSP and CPU. */
				u32 written_value_mask = 0x200;
				u32 status_mask = 1;
				for (int i = 0; i < 8; ++i) {
					if ((data & written_value_mask) && !(data & written_value_mask << 1)) {
						sp.status.sig &= ~status_mask;
					}
					else if (!(data & written_value_mask) && (data & written_value_mask << 1)) {
						sp.status.sig |= status_mask;
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
				sp.semaphore = 0; /* goes against n64brew, but is according to ares */
				break;

			default:
				std::unreachable();
			}
		}
	}
}