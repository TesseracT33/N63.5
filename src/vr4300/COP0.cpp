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
		fpu_is_enabled = status.cu1;
		SetActiveVirtualToPhysicalFunctions();
		CheckInterrupts();
	}


	void COP0Registers::OnWriteToWired()
	{
		random_generator.SetRange(wired.value);
	}


	u64 COP0Registers::Get(size_t reg_index) const
	{
		auto StructToInt = [](auto struct_) {
			if constexpr (sizeof(struct_) == 4) {
				u32 ret;
				std::memcpy(&ret, &struct_, 4);
				return ret;
			}
			else if constexpr (sizeof(struct_) == 8) {
				u64 ret;
				std::memcpy(&ret, &struct_, 8);
				return ret;
			}
			else {
				static_assert(AlwaysFalse<sizeof(struct_)>, "Incorrectly sized struct given.");
			}
		};

		switch (reg_index & 31) {
		case cop0_index_index: return StructToInt(index);
		case cop0_index_random: return random_generator.Generate(); /* Generate a random number in the interval [wired, 31] */
		case cop0_index_entry_lo_0: return StructToInt(entry_lo_0);
		case cop0_index_entry_lo_1: return StructToInt(entry_lo_1);
		case cop0_index_context: return StructToInt(context);
		case cop0_index_page_mask: return StructToInt(page_mask);
		case cop0_index_wired: return StructToInt(wired);
		case cop0_index_bad_v_addr: return StructToInt(bad_v_addr);
		case cop0_index_count: return u32(count.value >> 1); /* See the declaration of 'count' */
		case cop0_index_entry_hi: return StructToInt(entry_hi);
		case cop0_index_compare: return u32(compare.value >> 1); /* See the declaration of 'compare' */
		case cop0_index_status: return StructToInt(status);
		case cop0_index_cause: return StructToInt(cause);
		case cop0_index_epc: return StructToInt(epc);
		case cop0_index_pr_id: return StructToInt(pr_id);
		case cop0_index_config: return StructToInt(config);
		case cop0_index_ll_addr: return StructToInt(ll_addr);
		case cop0_index_watch_lo: return StructToInt(watch_lo);
		case cop0_index_watch_hi: return StructToInt(watch_hi);
		case cop0_index_x_context: return StructToInt(x_context);
		case cop0_index_parity_error: return StructToInt(parity_error);
		case cop0_index_tag_lo: return StructToInt(tag_lo);
		case cop0_index_tag_hi: return StructToInt(tag_hi);
		case cop0_index_error_epc: return StructToInt(error_epc);
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
			if constexpr (raw) IntToStruct(random, value);
			else               IntToStructMasked(random, value, 0x0000'0040);
			break;

		case cop0_index_entry_lo_0:
			if constexpr (raw) IntToStruct(entry_lo_0, value);
			else               IntToStructMasked(entry_lo_0, value, 0x03FF'FFFF);
			break;

		case cop0_index_entry_lo_1:
			if constexpr (raw) IntToStruct(entry_lo_1, value);
			else               IntToStructMasked(entry_lo_1, value, 0x03FF'FFFF);
			break;

		case cop0_index_context:
			if constexpr (raw) IntToStruct(context, value);
			else               IntToStructMasked(context, value, 0xFFFF'FFFF'FFFF'FFF0);
			break;

		case cop0_index_page_mask:
			if constexpr (raw) IntToStruct(page_mask, value);
			else               IntToStructMasked(page_mask, value, 0x01FF'E000);
			break;

		case cop0_index_wired:
			if constexpr (raw) IntToStruct(wired, value);
			else               IntToStructMasked(wired, value, 0x3F);
			OnWriteToWired();
			break;

		case cop0_index_bad_v_addr:
			IntToStruct(bad_v_addr, value);
			break;

		case cop0_index_count:
			IntToStruct(count, value << 1); /* See the declaration of 'count' */
			OnWriteToCount();
			break;

		case cop0_index_entry_hi:
			if constexpr (raw) IntToStruct(entry_hi, value);
			else               IntToStructMasked(entry_hi, value, 0xC000'00FF'FFFF'E0FF);
			break;

		case cop0_index_compare:
			IntToStruct(compare, value << 1); /* See the declaration of 'compare' */
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
			IntToStruct(epc, value);
			break;

		case cop0_index_config:
			if constexpr (raw) IntToStruct(config, value);
			else               IntToStructMasked(config, value, 0xF00'800F);
			break;

		case cop0_index_ll_addr:
			IntToStruct(ll_addr, value);
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
			else               IntToStructMasked(x_context, value, 0xFFFF'FFFF'FFFF'FFF0);
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
			IntToStruct(error_epc, value);
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
		u64 cycles_until_count_compare_match = cop0_reg.compare.value - cop0_reg.count.value;
		if (cop0_reg.count.value >= cop0_reg.compare.value) {
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
	void COP0Move(const u32 instr_code)
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

		auto tlb_index = std::ranges::find_if(tlb_entries, [](const TlbEntry& entry) {
				return entry.entry_hi.asid == cop0_reg.entry_hi.asid &&
					entry.entry_hi.vpn2 == cop0_reg.entry_hi.vpn2 &&
					entry.entry_hi.r == cop0_reg.entry_hi.r;
			});
		if (tlb_index == tlb_entries.end()) {
			cop0_reg.index.p = 1;
		}
		else {
			cop0_reg.index.p = 0;
			cop0_reg.index.value = std::distance(tlb_entries.begin(), tlb_index);
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

		auto tlb_index = cop0_reg.index.value & 0x1F; /* bit 5 is not used */
		std::memcpy(&cop0_reg.entry_lo_0, &tlb_entries[tlb_index].entry_lo[0], 4);
		std::memcpy(&cop0_reg.entry_lo_1, &tlb_entries[tlb_index].entry_lo[1], 4);
		std::memcpy(&cop0_reg.entry_hi  , &tlb_entries[tlb_index].entry_hi   , 8);
		std::memcpy(&cop0_reg.page_mask , &tlb_entries[tlb_index].page_mask  , 4);
		cop0_reg.entry_hi.padding_of_zeroes = 0; /* entry_hi, unlike an TLB entry, does not have the G bit, but this is copied in from the memcpy. */
		cop0_reg.entry_lo_0.g = cop0_reg.entry_lo_1.g = tlb_entries[tlb_index].entry_hi.g;
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

		auto tlb_index = cop0_reg.index.value & 0x1F; /* bit 5 is not used */
		std::memcpy(&tlb_entries[tlb_index].entry_lo[0], &cop0_reg.entry_lo_0, 4);
		std::memcpy(&tlb_entries[tlb_index].entry_lo[1], &cop0_reg.entry_lo_1, 4);
		std::memcpy(&tlb_entries[tlb_index].entry_hi   , &cop0_reg.entry_hi  , 8);
		std::memcpy(&tlb_entries[tlb_index].page_mask  , &cop0_reg.page_mask , 4);
		tlb_entries[tlb_index].entry_hi.g = cop0_reg.entry_lo_0.g & cop0_reg.entry_lo_1.g;
		/* Compute things that will make the virtual-to-physical-address process faster. */
		auto addr_offset_bit_length = page_size_to_addr_offset_bit_length[cop0_reg.page_mask.value];
		tlb_entries[tlb_index].address_vpn2_mask = 0xFF'FFFF'FFFF << addr_offset_bit_length & 0xFF'FFFF'FFFF;
		tlb_entries[tlb_index].address_offset_mask = (1 << addr_offset_bit_length) - 1;
		tlb_entries[tlb_index].address_vpn_even_odd_mask = tlb_entries[tlb_index].address_offset_mask + 1;
		tlb_entries[tlb_index].vpn2_shifted = tlb_entries[tlb_index].entry_hi.vpn2 << addr_offset_bit_length;
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

		auto tlb_index = cop0_reg.random.value & 0x1F; /* bit 5 is not used */
		auto tlb_wired_index = cop0_reg.wired.value & 0x1F;
		if (tlb_index <= tlb_wired_index) {
			return;
		}
		std::memcpy(&tlb_entries[tlb_index].entry_lo[0], &cop0_reg.entry_lo_0, 4);
		std::memcpy(&tlb_entries[tlb_index].entry_lo[1], &cop0_reg.entry_lo_1, 4);
		std::memcpy(&tlb_entries[tlb_index].entry_hi   , &cop0_reg.entry_hi  , 8);
		std::memcpy(&tlb_entries[tlb_index].page_mask  , &cop0_reg.page_mask , 4);
		tlb_entries[tlb_index].entry_hi.g = cop0_reg.entry_lo_0.g & cop0_reg.entry_lo_1.g;

		auto addr_offset_bit_length = page_size_to_addr_offset_bit_length[cop0_reg.page_mask.value];
		tlb_entries[tlb_index].address_vpn2_mask = 0xFF'FFFF'FFFF << addr_offset_bit_length & 0xFF'FFFF'FFFF;
		tlb_entries[tlb_index].address_offset_mask = (1 << addr_offset_bit_length) - 1;
		tlb_entries[tlb_index].address_vpn_even_odd_mask = tlb_entries[tlb_index].address_offset_mask + 1;
		tlb_entries[tlb_index].vpn2_shifted = tlb_entries[tlb_index].entry_hi.vpn2 << addr_offset_bit_length;
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
			pc = cop0_reg.epc.value;
			cop0_reg.status.exl = 0;
		}
		else {
			pc = cop0_reg.error_epc.value;
			cop0_reg.status.erl = 0;
		}
		ll_bit = 0;

		/* Check if the pc is misaligned, and if so, signal an exception right away.
		   Then, there is no need to check if the pc is misaligned every time an instruction is fetched
		   (this is one of the few places where the pc can be set to a misaligned value). */
		if (pc & 3) {
			SignalAddressErrorException<Memory::Operation::InstrFetch>(pc);
		}
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