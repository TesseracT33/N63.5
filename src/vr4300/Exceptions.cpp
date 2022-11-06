module VR4300:Exceptions;

import :COP0;
import :CPU;
import :MMU;
import :Operation;

import DebugOptions;
import Logging;

namespace VR4300
{
	template<Exception exception, Memory::Operation operation>
	constexpr int GetExceptionPriority()
	{
		if constexpr (exception == Exception::AddressError) {
			if constexpr (operation == Memory::Operation::InstrFetch) return 17;
			else return 6;
		}
		else if constexpr (exception == Exception::Breakpoint) {
			return 12;
		}
		else if constexpr (exception == Exception::BusError) {
			if constexpr (operation == Memory::Operation::InstrFetch) return 14;
			else return 1;
		}
		else if constexpr (exception == Exception::ColdReset) {
			return 20;
		}
		else if constexpr (exception == Exception::CoprocessorUnusable) {
			return 11;
		}
		else if constexpr (exception == Exception::FloatingPoint) {
			return 7;
		}
		else if constexpr (exception == Exception::IntegerOverflow) {
			return 8;
		}
		else if constexpr (exception == Exception::Interrupt) {
			return 0;
		}
		else if constexpr (exception == Exception::Nmi) {
			return 18;
		}
		else if constexpr (exception == Exception::ReservedInstruction) {
			return 10;
		}
		else if constexpr (exception == Exception::SoftReset) {
			return 19;
		}
		else if constexpr (exception == Exception::Syscall) {
			return 13;
		}
		else if constexpr (exception == Exception::TlbInvalid) {
			if constexpr (operation == Memory::Operation::InstrFetch) return 15;
			else return 4;
		}
		else if constexpr (exception == Exception::TlbMiss || exception == Exception::XtlbMiss) {
			if constexpr (operation == Memory::Operation::InstrFetch) return 16;
			else return 5;
		}
		else if constexpr (exception == Exception::TlbModification) {
			return 3;
		}
		else if constexpr (exception == Exception::Trap) {
			return 9;
		}
		else if constexpr (exception == Exception::Watch) {
			return 2;
		}
		else {
			static_assert(AlwaysFalse<exception>);
		}
	}


	template<Exception exception, Memory::Operation operation>
	constexpr ExceptionHandler GetExceptionHandler()
	{
		     if constexpr (exception == Exception::AddressError)        return AddressErrorException<operation>;
		else if constexpr (exception == Exception::Breakpoint)          return BreakPointException;
		else if constexpr (exception == Exception::BusError)            return BusErrorException<operation>;
		else if constexpr (exception == Exception::ColdReset)           return ColdResetException;
		else if constexpr (exception == Exception::CoprocessorUnusable) return CoprocessorUnusableException;
		else if constexpr (exception == Exception::FloatingPoint)       return FloatingpointException;
		else if constexpr (exception == Exception::IntegerOverflow)     return IntegerOverflowException;
		else if constexpr (exception == Exception::Interrupt)           return InterruptException;
		else if constexpr (exception == Exception::Nmi)                 return NmiException;
		else if constexpr (exception == Exception::ReservedInstruction) return ReservedInstructionException;
		else if constexpr (exception == Exception::SoftReset)           return SoftResetException;
		else if constexpr (exception == Exception::Syscall)             return SyscallException;
		else if constexpr (exception == Exception::TlbInvalid)          return TlbInvalidException<operation>;
		else if constexpr (exception == Exception::TlbMiss)             return TlbMissException<operation>;
		else if constexpr (exception == Exception::TlbModification)     return TlbModException;
		else if constexpr (exception == Exception::Trap)                return TrapException;
		else if constexpr (exception == Exception::Watch)               return WatchException;
		else if constexpr (exception == Exception::XtlbMiss)            return XtlbMissException<operation>;
		else                                                            static_assert(AlwaysFalse<exception>);
	}


	template<Exception exception>
	u64 GetExceptionVector()
	{ /* See p. 181, Table 6-3 */
		if constexpr (exception == Exception::ColdReset || exception == Exception::SoftReset || exception == Exception::Nmi) {
			return 0xFFFF'FFFF'BFC0'0000;
		}
		else {
			if constexpr (exception == Exception::TlbMiss) {
				static constexpr u64 base_addr[2][2] = {
					0xFFFF'FFFF'8000'0000, 0xFFFF'FFFF'8000'0180,
					0xFFFF'FFFF'BFC0'0200, 0xFFFF'FFFF'BFC0'0380
				};
				return base_addr[cop0_reg.status.bev][cop0_reg.status.exl];
			}
			else if constexpr (exception == Exception::XtlbMiss) {
				static constexpr u64 base_addr[2][2] = {
					0xFFFF'FFFF'8000'0080, 0xFFFF'FFFF'8000'0180,
					0xFFFF'FFFF'BFC0'0280, 0xFFFF'FFFF'BFC0'0380
				};
				return base_addr[cop0_reg.status.bev][cop0_reg.status.exl];
			}
			else {
				return cop0_reg.status.bev ? 0xFFFF'FFFF'BFC0'0380 : 0xFFFF'FFFF'8000'0180;
			}
		}
	}


	void HandleException()
	{
		if constexpr (log_cpu_exceptions) {
			LogException(ExceptionToString(occurred_exception));
		}

		exception_has_occurred = false;

		if (cop0_reg.status.exl == 0) {
			cop0_reg.cause.bd = in_branch_delay_slot; /* Peter Lemon exception tests indicate that this should only be set if !exl */
			/* Store to the EPC register the address of the instruction causing the exception.
			   If the instruction was executing in a branch delay slot, the CPU loads the EPC register
			   to the address of the branch instruction immediately preceding the branch delay slot. */
			cop0_reg.epc = pc - (in_branch_delay_slot ? 8 : 4);
			cop0_reg.status.exl = 1;
			SetActiveVirtualToPhysicalFunctions();
		}
		pc = exception_vector;
		in_branch_delay_slot = false;
		jump_is_pending = false;

		exception_handler();
	}


	void SignalCoprocessorUnusableException(int co)
	{
		SignalException<Exception::CoprocessorUnusable>();
		coprocessor_unusable_source = co;
	}


	template<Exception exception, Memory::Operation operation>
	void SignalException()
	{
		constexpr static auto new_exception_priority = GetExceptionPriority<exception, operation>();
		if (exception_has_occurred) {
			/* Compare exception priorities; return if the new exception has a lower priority than an already occured one. */
			if (new_exception_priority < occurred_exception_priority) {
				return;
			}
		}
		exception_has_occurred = true;
		occurred_exception = exception;
		occurred_exception_priority = new_exception_priority;
		/* The below two assignments incur a run-time cost of two stores and one branch if a new exception occurs with a lower priority.
		   However, if we fetched this data once we knew which exception to handle, then the functions 'GetExceptionVector',
		   'GetExceptionCauseCode' and 'GetExceptionHandlerFun' could not take 'exception' as a template argument, and would
		   instead have to take it as a function argument. Then, several run-time branches would have to be taken over this argument. */
		exception_vector = GetExceptionVector<exception>();
		exception_handler = GetExceptionHandler<exception, operation>();
	}


	template<Memory::Operation operation>
	void SignalAddressErrorException(u64 bad_virt_addr)
	{
		SignalException<Exception::AddressError, operation>();
		address_failure.bad_virt_addr = bad_virt_addr;
		address_failure.bad_vpn2 = bad_virt_addr >> (page_size_to_addr_offset_bit_length[cop0_reg.page_mask.value] + 1) & 0xFF'FFFF'FFFF;
		address_failure.bad_asid = cop0_reg.entry_hi.asid;
		/* TODO: should this not depend on virt addr? */
		address_failure.bad_space_id = [&] {
			if (operating_mode == OperatingMode::Kernel && bad_virt_addr >= 0xFFFF'FFFF'0000'0000) return 3;
			if (addressing_mode == AddressingMode::_32bit) return 0;
			if (operating_mode == OperatingMode::User) return 0;
			if (operating_mode == OperatingMode::Supervisor) return 1;
		}();
	}


	template<Memory::Operation operation>
	void AddressErrorException()
	{
		cop0_reg.cause.exc_code = [&] {
			if constexpr (operation == Memory::Operation::Write) return 5;
			else                                                 return 4;
		}();
		cop0_reg.bad_v_addr = address_failure.bad_virt_addr;
		cop0_reg.context.bad_vpn2 = address_failure.bad_vpn2;
		cop0_reg.x_context.bad_vpn2 = address_failure.bad_vpn2;
		cop0_reg.x_context.r = address_failure.bad_space_id;
		cop0_reg.cause.ce = 0;
	}


	void BreakPointException()
	{
		cop0_reg.cause.exc_code = 9;
	}


	template<Memory::Operation operation>
	void BusErrorException()
	{
		cop0_reg.cause.exc_code = [&] {
			if constexpr (operation == Memory::Operation::InstrFetch) return 6;
			else                                                      return 7;
		}();
		cop0_reg.cause.ce = 0;
	}


	void ColdResetException()
	{
		cop0_reg.status.rp = cop0_reg.status.sr = cop0_reg.status.ts = 0;
		cop0_reg.status.erl = cop0_reg.status.bev = 1;
		cop0_reg.config.ep = 0;
		cop0_reg.config.be = 1;
		cop0_reg.random.value = 31;
		cop0_reg.OnWriteToStatus();
		/* TODO The EC(2:0) bits of the Config register are set to the contents of the DivMode(1:0)* pins */
	}


	void CoprocessorUnusableException()
	{
		cop0_reg.cause.exc_code = 11;
		cop0_reg.cause.ce = coprocessor_unusable_source;
	}


	void FloatingpointException()
	{
		cop0_reg.cause.exc_code = 15;
		cop0_reg.cause.ce = 0;
	}


	void IntegerOverflowException()
	{
		cop0_reg.cause.exc_code = 12;
		cop0_reg.cause.ce = 0;
	}


	void InterruptException()
	{
		cop0_reg.cause.exc_code = 0;
		cop0_reg.cause.ce = 0;
	}


	void NmiException()
	{
		pc = cop0_reg.error_epc;
		cop0_reg.status.ts = 0;
		cop0_reg.status.erl = cop0_reg.status.sr = cop0_reg.status.bev = 1;
		cop0_reg.cause.ce = 0;
	}


	void ReservedInstructionException()
	{
		cop0_reg.cause.exc_code = 10;
		cop0_reg.cause.ce = 0;
	}


	void SoftResetException()
	{
		if (cop0_reg.status.erl == 0) {
			pc = cop0_reg.error_epc;
		}
		cop0_reg.status.rp = cop0_reg.status.ts = 0;
		cop0_reg.status.bev = cop0_reg.status.erl = cop0_reg.status.sr = 1;
	}


	void SyscallException()
	{
		cop0_reg.cause.exc_code = 8;
		cop0_reg.cause.ce = 0;
	}


	template<Memory::Operation operation>
	void TlbInvalidException()
	{
		cop0_reg.cause.exc_code = [&] {
			if constexpr (operation == Memory::Operation::Write) return 3;
			else                                                 return 2;
		}();
		cop0_reg.bad_v_addr = address_failure.bad_virt_addr;
		cop0_reg.context.bad_vpn2 = address_failure.bad_vpn2; /* TODO: write to xcontext in 64 bit mode? */
		cop0_reg.entry_hi.vpn2 = address_failure.bad_vpn2; /* TODO: should this assignment be made? */
		cop0_reg.entry_hi.asid = address_failure.bad_asid;
		cop0_reg.cause.ce = 0;
		/* TODO: what about the R field in entry_hi? */
	}


	template<Memory::Operation operation>
	void TlbMissException()
	{
		cop0_reg.cause.exc_code = [&] {
			if constexpr (operation == Memory::Operation::Write) return 3;
			else                                                 return 2;
		}();
		cop0_reg.bad_v_addr = address_failure.bad_virt_addr;
		cop0_reg.context.bad_vpn2 = address_failure.bad_vpn2;
		cop0_reg.entry_hi.vpn2 = address_failure.bad_vpn2; /* TODO: should this assignment be made? */
		cop0_reg.entry_hi.asid = address_failure.bad_asid;
		cop0_reg.cause.ce = 0;
	}


	void TlbModException()
	{
		cop0_reg.cause.exc_code = 1;
		cop0_reg.bad_v_addr = address_failure.bad_virt_addr;
		cop0_reg.context.bad_vpn2 = address_failure.bad_vpn2;
		cop0_reg.entry_hi.vpn2 = address_failure.bad_vpn2; /* TODO: should this assignment be made? */
		cop0_reg.entry_hi.asid = address_failure.bad_asid;
		cop0_reg.cause.ce = 0;
	}


	void TrapException()
	{
		cop0_reg.cause.exc_code = 13;
		cop0_reg.cause.ce = 0;
	}


	void WatchException()
	{
		cop0_reg.cause.exc_code = 23;
		cop0_reg.cause.ce = 0;
	}


	template<Memory::Operation operation>
	void XtlbMissException()
	{
		cop0_reg.cause.exc_code = [&] {
			if constexpr (operation == Memory::Operation::Write) return 3;
			else                                                 return 2;
		}();
		cop0_reg.bad_v_addr = address_failure.bad_virt_addr;
		cop0_reg.context.bad_vpn2 = address_failure.bad_vpn2;
		cop0_reg.x_context.bad_vpn2 = address_failure.bad_vpn2;
		cop0_reg.entry_hi.vpn2 = address_failure.bad_vpn2;
		cop0_reg.entry_hi.asid = address_failure.bad_asid;
		cop0_reg.cause.ce = 0;
	}


	constexpr std::string_view ExceptionToString(Exception exception)
	{
		switch (exception) {
		case Exception::AddressError:        return "Address Error";
		case Exception::Breakpoint:          return "Breakpoint";
		case Exception::BusError:            return "Bus Error";
		case Exception::ColdReset:           return "Cold Reset";
		case Exception::CoprocessorUnusable: return "Coprocessor Unusable";
		case Exception::FloatingPoint:       return "Floating Point";
		case Exception::IntegerOverflow:     return "Integer Overflow";
		case Exception::Interrupt:           return "Interrupt";
		case Exception::Nmi:                 return "NMI";
		case Exception::ReservedInstruction: return "Reserved instruction";
		case Exception::SoftReset:           return "Soft Reset";
		case Exception::Syscall:             return "Syscall";
		case Exception::TlbInvalid:          return "Invalid TLB";
		case Exception::TlbMiss:             return "TLB Miss";
		case Exception::TlbModification:     return "TLB Modification";
		case Exception::Trap:                return "Trap";
		case Exception::Watch:               return "Watch";
		case Exception::XtlbMiss:            return "XTLB Miss";
		default: assert(false);              return "";
		}
	}


#define ENUMERATE_SIGNAL_EXCEPTION_SPECIALIZATIONS(MEMORY_OPERATION) \
	template void SignalException<Exception::AddressError, MEMORY_OPERATION>(); \
	template void SignalException<Exception::Breakpoint, MEMORY_OPERATION>(); \
	template void SignalException<Exception::BusError, MEMORY_OPERATION>(); \
	template void SignalException<Exception::ColdReset, MEMORY_OPERATION>(); \
	template void SignalException<Exception::CoprocessorUnusable, MEMORY_OPERATION>(); \
	template void SignalException<Exception::FloatingPoint, MEMORY_OPERATION>(); \
	template void SignalException<Exception::IntegerOverflow, MEMORY_OPERATION>(); \
	template void SignalException<Exception::Interrupt, MEMORY_OPERATION>(); \
	template void SignalException<Exception::Nmi, MEMORY_OPERATION>(); \
	template void SignalException<Exception::ReservedInstruction, MEMORY_OPERATION>(); \
	template void SignalException<Exception::SoftReset, MEMORY_OPERATION>(); \
	template void SignalException<Exception::Syscall, MEMORY_OPERATION>(); \
	template void SignalException<Exception::TlbInvalid, MEMORY_OPERATION>(); \
	template void SignalException<Exception::TlbMiss, MEMORY_OPERATION>(); \
	template void SignalException<Exception::TlbModification, MEMORY_OPERATION>(); \
	template void SignalException<Exception::Trap, MEMORY_OPERATION>(); \
	template void SignalException<Exception::Watch, MEMORY_OPERATION>(); \
	template void SignalException<Exception::XtlbMiss, MEMORY_OPERATION>();

	ENUMERATE_SIGNAL_EXCEPTION_SPECIALIZATIONS(Memory::Operation::Read)
	ENUMERATE_SIGNAL_EXCEPTION_SPECIALIZATIONS(Memory::Operation::Write)
	ENUMERATE_SIGNAL_EXCEPTION_SPECIALIZATIONS(Memory::Operation::InstrFetch)

	template void SignalAddressErrorException<Memory::Operation::InstrFetch>(u64);
	template void SignalAddressErrorException<Memory::Operation::Read>(u64);
	template void SignalAddressErrorException<Memory::Operation::Write>(u64);
}