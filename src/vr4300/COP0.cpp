module VR4300:COP0;

import :COP1;
import :CPU;
import :Exceptions;
import :Operation;
import :MMU;

import DebugOptions;
import Memory;
import Scheduler;

namespace VR4300
{
	void COP0Registers::OnWriteToCause()
	{
		CheckInterrupts();
	}


	void COP0Registers::OnWriteToCompare()
	{
		cause.ip &= ~0x80; /* TODO: not sure if anything else needs to be done? */
		ReloadCountCompareEvent();
	}


	void COP0Registers::OnWriteToCount()
	{
		ReloadCountCompareEvent();
	}


	void COP0Registers::OnWriteToStatus()
	{
		SetActiveVirtualToPhysicalFunctions();
		CheckInterrupts();
	}


	void COP0Registers::OnWriteToWired()
	{
		random_generator.SetRange(wired);
	}


	u64 COP0Registers::Get(size_t reg_index) const
	{
		auto StructToInt = [](auto struct_) {
			     if constexpr (sizeof(struct_) == 4) return std::bit_cast<u32>(struct_);
			else if constexpr (sizeof(struct_) == 8) return std::bit_cast<u64>(struct_);
			else static_assert(AlwaysFalse<sizeof(struct_)>, "Struct must be either 4 or 8 bytes.");
		};

		switch (reg_index & 31) {
		case cop0_index_index: return StructToInt(index);
		case cop0_index_random: return random_generator.Generate(); /* Generate a random number in the interval [wired, 31] */
		case cop0_index_entry_lo_0: return StructToInt(entry_lo_0);
		case cop0_index_entry_lo_1: return StructToInt(entry_lo_1);
		case cop0_index_context: return StructToInt(context);
		case cop0_index_page_mask: return page_mask;
		case cop0_index_wired: return wired;
		case cop0_index_bad_v_addr: return bad_v_addr;
		case cop0_index_count: return u32(count >> 1); /* See the declaration of 'count' */
		case cop0_index_entry_hi: return StructToInt(entry_hi);
		case cop0_index_compare: return u32(compare >> 1); /* See the declaration of 'compare' */
		case cop0_index_status: return StructToInt(status);
		case cop0_index_cause: return StructToInt(cause);
		case cop0_index_epc: return epc;
		case cop0_index_pr_id: return StructToInt(pr_id);
		case cop0_index_config: return StructToInt(config);
		case cop0_index_ll_addr: return ll_addr;
		case cop0_index_watch_lo: return StructToInt(watch_lo);
		case cop0_index_watch_hi: return StructToInt(watch_hi);
		case cop0_index_x_context: return StructToInt(x_context);
		case cop0_index_parity_error: return StructToInt(parity_error);
		case cop0_index_cache_error: return cache_error;
		case cop0_index_tag_lo: return StructToInt(tag_lo);
		case cop0_index_tag_hi: return tag_hi;
		case cop0_index_error_epc: return error_epc;
		case  7: return cop0_unused_7;
		case 21: return cop0_unused_21;
		case 22: return cop0_unused_22;
		case 23: return cop0_unused_23;
		case 24: return cop0_unused_24;
		case 25: return cop0_unused_25;
		case 31: return cop0_unused_31;
		default: return 0;
		}
	}


	template<bool raw>
	void COP0Registers::Set(size_t reg_index, auto value)
	{
		auto IntToStruct = [](auto& struct_, auto value) {
			/* The operation of DMFC0 instruction on a 32-bit register of the CP0 is undefined.
				Here: simply write to the lower 32 bits. */
			static_assert(sizeof(struct_) == 4 || sizeof(struct_) == 8);
			static_assert(sizeof(value) == 4 || sizeof(value) == 8);
			static constexpr auto num_bytes_to_write = std::min(sizeof(struct_), sizeof(value));
			std::memcpy(&struct_, &value, num_bytes_to_write);
		};

		auto IntToStructMasked = [](auto& struct_, auto value, auto mask) {
			using StructT = std::remove_reference_t<decltype(struct_)>;
			static_assert(sizeof(struct_) == 4 || sizeof(struct_) == 8);
			static_assert(sizeof(value) == 4 || sizeof(value) == 8);
			value &= mask;
			if constexpr (sizeof(struct_) == 4) {
				u32 prev_struct = std::bit_cast<u32>(struct_);
				u32 new_struct = u32(value | prev_struct & ~mask);
				struct_ = std::bit_cast<StructT>(new_struct);
			}
			else {
				u64 prev_struct = std::bit_cast<u64>(struct_);
				u64 new_struct = value | prev_struct & ~mask;
				struct_ = std::bit_cast<StructT>(new_struct);
			}
		};

		switch (reg_index) { /* Masks are used for bits that are non-writeable. */
		case cop0_index_index:
			if constexpr (raw) IntToStruct(index, value);
			else               IntToStructMasked(index, value, 0x8000'003F);
			break;

		case cop0_index_random:
			if constexpr (raw) random = value;
			else               random = value & 0x20;
			break;

		case cop0_index_entry_lo_0:
			if constexpr (raw) IntToStruct(entry_lo_0, value);
			else               IntToStructMasked(entry_lo_0, value, 0x3FFF'FFFF);
			break;

		case cop0_index_entry_lo_1:
			if constexpr (raw) IntToStruct(entry_lo_1, value);
			else               IntToStructMasked(entry_lo_1, value, 0x3FFF'FFFF);
			break;

		case cop0_index_context:
			if constexpr (raw) IntToStruct(context, value);
			else               IntToStructMasked(context, value, ~0xFull);
			break;

		case cop0_index_page_mask:
			if constexpr (raw) page_mask = value;
			else               page_mask = value & 0x01FF'E000;
			break;

		case cop0_index_wired:
			if constexpr (raw) wired = value;
			else               wired = value & 0x3F;
			OnWriteToWired();
			break;

		case cop0_index_bad_v_addr:
			if constexpr (raw) bad_v_addr = value;
			break;

		case cop0_index_count:
			count = value << 1; /* See the declaration of 'count' */
			OnWriteToCount();
			break;

		case cop0_index_entry_hi:
			if constexpr (raw) IntToStruct(entry_hi, value);
			else               IntToStructMasked(entry_hi, value, 0xC000'00FF'FFFF'E0FF);
			break;

		case cop0_index_compare:
			compare = value << 1; /* See the declaration of 'compare' */
			OnWriteToCompare();
			break;

		case cop0_index_status:
			if constexpr (raw) IntToStruct(status, value);
			else               IntToStructMasked(status, value, 0xFF57'FFFF);
			OnWriteToStatus();
			break;

		case cop0_index_cause:
			if constexpr (raw) IntToStruct(cause, value);
			else               IntToStructMasked(cause, value, 0x300);
			OnWriteToCause();
			break;

		case cop0_index_epc:
			epc = value;
			break;

		case cop0_index_config:
			if constexpr (raw) IntToStruct(config, value);
			else               IntToStructMasked(config, value, 0xF00'800F);
			break;

		case cop0_index_ll_addr:
			ll_addr = value;
			break;

		case cop0_index_watch_lo:
			if constexpr (raw) IntToStruct(watch_lo, value);
			else               IntToStructMasked(watch_lo, value, 0xFFFF'FFFB);
			break;

		case cop0_index_watch_hi:
			IntToStruct(watch_hi, value);
			break;

		case cop0_index_x_context:
			if constexpr (raw) IntToStruct(x_context, value);
			else               IntToStructMasked(x_context, value, ~0xFull);
			break;

		case cop0_index_parity_error:
			if constexpr (raw) IntToStruct(parity_error, value);
			else               IntToStructMasked(parity_error, value, 0xFF);
			break;

		case cop0_index_tag_lo:
			if constexpr (raw) IntToStruct(tag_lo, value);
			else               IntToStructMasked(tag_lo, value, 0x0FFF'FFC0);
			break;

		case cop0_index_error_epc:
			error_epc = value;
			break;

		case  7: cop0_unused_7 =  u32(value); break;
		case 21: cop0_unused_21 = u32(value); break;
		case 22: cop0_unused_22 = u32(value); break;
		case 23: cop0_unused_23 = u32(value); break;
		case 24: cop0_unused_24 = u32(value); break;
		case 25: cop0_unused_25 = u32(value); break;
		case 31: cop0_unused_31 = u32(value); break;
		}
	}


	void OnCountCompareMatchEvent()
	{
		cop0_reg.cause.ip |= 0x80;
		CheckInterrupts();
		ReloadCountCompareEvent();
	}


	template<bool initial_add>
	void ReloadCountCompareEvent()
	{
		u64 cycles_until_count_compare_match = (cop0_reg.compare - cop0_reg.count) & 0x1'FFFF'FFFF;
		if ((cop0_reg.count & 0x1'FFFF'FFFF) >= (cop0_reg.compare & 0x1'FFFF'FFFF)) {
			cycles_until_count_compare_match += 0x2'0000'0000;
		}
		if constexpr (initial_add) {
			Scheduler::AddEvent(Scheduler::EventType::CountCompareMatch, cycles_until_count_compare_match, OnCountCompareMatchEvent);
		}
		else {
			Scheduler::ChangeEventTime(Scheduler::EventType::CountCompareMatch, cycles_until_count_compare_match);
		}
	}


	template<COP0Instruction instr>
	void COP0Move(u32 instr_code)
	{
		AdvancePipeline(1);
		using enum COP0Instruction;

		if (operating_mode != OperatingMode::Kernel) {
			if (!cop0_reg.status.cu0) {
				SignalCoprocessorUnusableException(0);
				return;
			}
			if constexpr (OneOf(instr, DMFC0, DMTC0)) {
				if (addressing_mode == AddressingMode::_32bit) {
					SignalException<Exception::ReservedInstruction>();
					return;
				}
			}
		}

		auto rd = instr_code >> 11 & 0x1F;
		auto rt = instr_code >> 16 & 0x1F;

		if constexpr (log_cpu_instructions) {
			current_instr_log_output = std::format("{} {}, {}", current_instr_name, rt, cop0_reg_str_repr[rd]);
		}

		if constexpr (instr == MTC0) {
			/* Move To System Control Coprocessor;
			   Loads the contents of the word of the general purpose register rt of the CPU
			   to the general purpose register rd of CP0. */
			cop0_reg.Set(rd, s32(gpr[rt]));
		}
		else if constexpr (instr == MFC0) {
			/* Move From System Control Coprocessor;
			   Loads the contents of the word of the general purpose register rd of CP0
			   to the general purpose register rt of the CPU. */
			gpr.Set(rt, s32(cop0_reg.Get(rd)));
		}
		else if constexpr (instr == DMTC0) {
			/* Doubleword Move To System Control Coprocessor;
			   Loads the contents of the doubleword of the general purpose register rt of the CPU
			   to the general purpose register rd of CP0. */
			cop0_reg.Set(rd, gpr[rt]);
		}
		else if constexpr (instr == DMFC0) {
			/* Doubleword Move From System Control Coprocessor;
			   Loads the contents of the doubleword of the general purpose register rd of CP0
			   to the general purpose register rt of the CPU. */
			gpr.Set(rt, cop0_reg.Get(rd));
		}
		else {
			static_assert(AlwaysFalse<instr>);
		}
	}


	void TLBP()
	{
		/* Translation Lookaside Buffer Probe;
		   Searches a TLB entry that matches with the contents of the entry Hi register and
		   sets the number of that TLB entry to the index register. If a TLB entry that
		   matches is not found, sets the most significant bit of the index register. */
		if constexpr (log_cpu_instructions) {
			current_instr_log_output = current_instr_name;
		}

		AdvancePipeline(1);

		if (operating_mode != OperatingMode::Kernel && !cop0_reg.status.cu0) {
			SignalCoprocessorUnusableException(0);
			return;
		}

		auto index = std::ranges::find_if(tlb_entries, [](const TlbEntry& entry) {
				return entry.entry_hi.asid == cop0_reg.entry_hi.asid &&
					entry.entry_hi.vpn2 == cop0_reg.entry_hi.vpn2 &&
					entry.entry_hi.r == cop0_reg.entry_hi.r;
		});
		if (index == tlb_entries.end()) {
			cop0_reg.index.p = 1;
		}
		else {
			cop0_reg.index.p = 0;
			cop0_reg.index.value = std::distance(tlb_entries.begin(), index);
		}
	}


	void TLBR()
	{
		/* Translation Lookaside Buffer Read;
		   The EntryHi and EntryLo registers are loaded with the contents of the TLB entry
		   pointed at by the contents of the Index register. The G bit (which controls ASID matching)
		   read from the TLB is written into both of the EntryLo0 and EntryLo1 registers. */
		if constexpr (log_cpu_instructions) {
			current_instr_log_output = current_instr_name;
		}

		AdvancePipeline(1);

		if (operating_mode != OperatingMode::Kernel && !cop0_reg.status.cu0) {
			SignalCoprocessorUnusableException(0);
			return;
		}

		tlb_entries[cop0_reg.index.value & 0x1F].Read();
	}


	void TLBWI()
	{
		/* Translation Lookaside Buffer Write Index;
		   The TLB entry pointed at by the Index register is loaded with the contents of the
		   EntryHi and EntryLo registers. The G bit of the TLB is written with the logical
		   AND of the G bits in the EntryLo0 and EntryLo1 registers. */
		if constexpr (log_cpu_instructions) {
			current_instr_log_output = current_instr_name;
		}

		AdvancePipeline(1);

		if (operating_mode != OperatingMode::Kernel && !cop0_reg.status.cu0) {
			SignalCoprocessorUnusableException(0);
			return;
		}

		tlb_entries[cop0_reg.index.value & 0x1F].Write();
	}


	void TLBWR()
	{
		/* Translation Lookaside Buffer Write Random;
		   The TLB entry pointed at by the Random register is loaded with the contents of
		   the EntryHi and EntryLo registers. The G bit of the TLB is written with the logical
		   AND of the G bits in the EntryLo0 and EntryLo1 registers.
		   The 'wired' register determines which TLB entries cannot be overwritten. */
		if constexpr (log_cpu_instructions) {
			current_instr_log_output = current_instr_name;
		}

		AdvancePipeline(1);

		if (operating_mode != OperatingMode::Kernel && !cop0_reg.status.cu0) {
			SignalCoprocessorUnusableException(0);
			return;
		}

		auto index = cop0_reg.random & 0x1F;
		auto wired = cop0_reg.wired & 0x1F;
		if (index <= wired) {
			return;
		}
		tlb_entries[index].Write();
	}


	void ERET()
	{
		/* Return From Exception;
		   Returns from an exception, interrupt, or error trap. */
		if constexpr (log_cpu_instructions) {
			current_instr_log_output = current_instr_name;
		}

		AdvancePipeline(1);

		if (operating_mode != OperatingMode::Kernel && !cop0_reg.status.cu0) {
			SignalCoprocessorUnusableException(0);
			return;
		}

		if (cop0_reg.status.erl == 0) {
			pc = cop0_reg.epc;
			cop0_reg.status.exl = 0;
		}
		else {
			pc = cop0_reg.error_epc;
			cop0_reg.status.erl = 0;
		}
		ll_bit = 0;

		/* Check if the pc is misaligned, and if so, signal an exception right away.
		   Then, there is no need to check if the pc is misaligned every time an instruction is fetched
		   (this is one of the few places where the pc can be set to a misaligned value). */
		if (pc & 3) {
			SignalAddressErrorException<Memory::Operation::InstrFetch>(pc);
		}
		SetActiveVirtualToPhysicalFunctions();
	}


	template void COP0Registers::Set<false>(size_t, s32);
	template void COP0Registers::Set<false>(size_t, u32);
	template void COP0Registers::Set<false>(size_t, u64);
	template void COP0Registers::Set<true>(size_t, s32);
	template void COP0Registers::Set<true>(size_t, u32);
	template void COP0Registers::Set<true>(size_t, u64);

	template void COP0Move<COP0Instruction::MTC0>(u32);
	template void COP0Move<COP0Instruction::MFC0>(u32);
	template void COP0Move<COP0Instruction::DMTC0>(u32);
	template void COP0Move<COP0Instruction::DMFC0>(u32);

	template void ReloadCountCompareEvent<false>();
	template void ReloadCountCompareEvent<true>();
}