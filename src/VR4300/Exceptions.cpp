module VR4300:Exceptions;

import :COP0;
import :CPU;
import :MMU;
import :Operation;
import :Registers;

import DebugOptions;
import Logging;

namespace VR4300
{
	/* Exception handlers */
	template<MemoryAccess::Operation operation> void AddressErrorException();
	template<MemoryAccess::Operation operation> void BusErrorException();
	template<MemoryAccess::Operation operation> void TLB_InvalidException();
	template<MemoryAccess::Operation operation> void TLB_MissException();
	template<MemoryAccess::Operation operation> void XTLB_MissException();
	void BreakPointException();
	void BusErrorException();
	void ColdResetException();
	void CoprocessorUnusableException();
	void FloatingpointException();
	void IntegerOverflowException();
	void InterruptException();
	void NMI_Exception();
	void ReservedInstructionException();
	void SoftResetException();
	void SyscallException();
	void TLB_ModException();
	void TrapException();
	void WatchException();


	template<Exception exception, MemoryAccess::Operation operation>
	void SignalException()
	{
		constexpr static int new_exception_priority = GetExceptionPriority<exception, operation>();
		if (exception_has_occurred)
		{
			/* Compare exception priorities; return if the new exception has a lower priority than an already occured one. */
			if (new_exception_priority < occurred_exception_priority)
				return;
		}
		exception_has_occurred = true;
		occurred_exception = exception;
		occurred_exception_priority = new_exception_priority;
		/* The below two assignments incur a run-time cost of two stores and one branch if a new exception occurs with a lower priority.
		   However, if we fetched this data once we knew which exception to handle, then the functions 'GetExceptionVector',
		   'GetExceptionCauseCode' and 'GetExceptionHandlerFun' could not take 'exception' as a template argument, and would
		   instead have to take it as a function argument. Then, several run-time branches would have to be taken over this argument. */
		exception_vector = GetExceptionVector<exception>();
		exception_handler_fun = GetExceptionHandlerFun<exception, operation>();
	}


	template<MemoryAccess::Operation operation>
	void SignalAddressErrorException(const u64 bad_virt_addr)
	{
		SignalException<Exception::AddressError>();
		address_failure.bad_virt_addr = bad_virt_addr;
		address_failure.bad_vpn2 = bad_virt_addr >> (page_size_to_addr_offset_bit_length[cop0_reg.page_mask.value] + 1) & 0xFF'FFFF'FFFF;
		address_failure.bad_asid = cop0_reg.entry_hi.asid;
	}


	void HandleException()
	{
		if constexpr (log_cpu_exceptions)
		{
			Logging::LogException(ExceptionToString(occurred_exception));
		}

		exception_has_occurred = false;

		cop0_reg.cause.bd = pc_is_inside_branch_delay_slot;
		if (cop0_reg.status.exl == 0)
		{
			/* Store to the EPC register the address of the instruction causing the exception.
			   If the instruction was executing in a branch delay slot, the CPU loads the EPC register
			   to the address of the branch instruction immediately preceding the branch delay slot. */
			cop0_reg.epc.value = pc - 4 * (1 + pc_is_inside_branch_delay_slot);
			cop0_reg.status.exl = 1;
			SetActiveVirtualToPhysicalFunctions();
		}
		pc = exception_vector;
		pc_is_inside_branch_delay_slot = false;
		jump_is_pending = false;

		std::invoke(exception_handler_fun);
	}


	template<Exception exception, MemoryAccess::Operation operation>
	constexpr int GetExceptionPriority()
	{
		using enum Exception;
		using enum MemoryAccess::Operation;

		if constexpr (exception == AddressError) {
			if constexpr (operation == InstrFetch) return 17;
			else return 6;
		}
		else if constexpr (exception == Breakpoint) {
			return 12;
		}
		else if constexpr (exception == BusError) {
			if constexpr (operation == InstrFetch) return 14;
			else return 1;
		}
		else if constexpr (exception == ColdReset) {
			return 20;
		}
		else if constexpr (exception == CoprocessorUnusable) {
			return 11;
		}
		else if constexpr (exception == FloatingPoint) {
			return 7;
		}
		else if constexpr (exception == IntegerOverflow) {
			return 8;
		}
		else if constexpr (exception == Interrupt) {
			return 0;
		}
		else if constexpr (exception == NMI) {
			return 18;
		}
		else if constexpr (exception == ReservedInstruction) {
			return 10;
		}
		else if constexpr (exception == SoftReset) {
			return 19;
		}
		else if constexpr (exception == Syscall) {
			return 13;
		}
		else if constexpr (exception == TLB_Invalid) {
			if constexpr (operation == InstrFetch) return 15;
			else return 4;
		}
		else if constexpr (exception == TLB_Miss || exception == XTLB_Miss) {
			if constexpr (operation == InstrFetch) return 16;
			else return 5;
		}
		else if constexpr (exception == TLB_Modification) {
			return 3;
		}
		else if constexpr (exception == Trap) {
			return 9;
		}
		else if constexpr (exception == Watch) {
			return 2;
		}
		else {
			static_assert(exception != exception);
		}
	}


	template<Exception exception, MemoryAccess::Operation operation>
	constexpr ExceptionHandlerFun GetExceptionHandlerFun()
	{
		using enum Exception;
		using enum MemoryAccess::Operation;

		     if constexpr (exception == AddressError)        return AddressErrorException<operation>;
		else if constexpr (exception == Breakpoint)          return BreakPointException;
		else if constexpr (exception == BusError)            return BusErrorException<operation>;
		else if constexpr (exception == ColdReset)           return ColdResetException;
		else if constexpr (exception == CoprocessorUnusable) return CoprocessorUnusableException;
		else if constexpr (exception == FloatingPoint)       return FloatingpointException;
		else if constexpr (exception == IntegerOverflow)     return IntegerOverflowException;
		else if constexpr (exception == Interrupt)           return InterruptException;
		else if constexpr (exception == NMI)                 return NMI_Exception;
		else if constexpr (exception == ReservedInstruction) return ReservedInstructionException;
		else if constexpr (exception == SoftReset)           return SoftResetException;
		else if constexpr (exception == Syscall)             return SyscallException;
		else if constexpr (exception == TLB_Invalid)         return TLB_InvalidException<operation>;
		else if constexpr (exception == TLB_Miss)            return TLB_MissException<operation>;
		else if constexpr (exception == TLB_Modification)    return TLB_ModException;
		else if constexpr (exception == Trap)                return TrapException;
		else if constexpr (exception == Watch)               return WatchException;
		else if constexpr (exception == XTLB_Miss)           return XTLB_MissException<operation>;
		else                                                 static_assert(exception != exception);
	}


	template<Exception exception>
	u64 GetExceptionVector()
	{ /* See p. 181, Table 6-3 */
		using enum Exception;

		static constexpr std::array<u64, 2> vector_base_addr = { /* Indexed by cop0.status.BEV */
			0xFFFF'FFFF'8000'0000, 0xFFFF'FFFF'BFC0'0200 };

		if constexpr (exception == ColdReset || exception == SoftReset || exception == NMI)
			return vector_base_addr[1];
		else if constexpr (exception == TLB_Miss)
			return vector_base_addr[cop0_reg.status.bev] | (cop0_reg.status.exl ? 0x0180 : 0x0000);
		else if constexpr (exception == XTLB_Miss)
			return vector_base_addr[cop0_reg.status.bev] | (cop0_reg.status.exl ? 0x0180 : 0x0080);
		else
			return vector_base_addr[cop0_reg.status.bev] | 0x0180;
	}


	template<MemoryAccess::Operation operation>
	void AddressErrorException()
	{
		cop0_reg.cause.exc_code = [&] {
			if constexpr (operation == MemoryAccess::Operation::Write) return 5;
			else                                                       return 4;
		}();
		cop0_reg.bad_v_addr.value = address_failure.bad_virt_addr;
		cop0_reg.context.bad_vpn2 = address_failure.bad_vpn2;
		cop0_reg.cause.ce = 0;
	}


	void BreakPointException()
	{
		cop0_reg.cause.exc_code = 9;
	}


	template<MemoryAccess::Operation operation>
	void BusErrorException()
	{
		cop0_reg.cause.exc_code = [&] {
			if constexpr (operation == MemoryAccess::Operation::InstrFetch) return 6;
			else                                                            return 7;
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

		cop0_reg.status.NotifyCpuAfterWrite();

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


	void NMI_Exception()
	{
		pc = cop0_reg.error_epc.value;
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
		if (cop0_reg.status.erl == 0)
			pc = cop0_reg.error_epc.value;
		cop0_reg.status.rp = cop0_reg.status.ts = 0;
		cop0_reg.status.bev = cop0_reg.status.erl = cop0_reg.status.sr = 1;
	}


	void SyscallException()
	{
		cop0_reg.cause.exc_code = 8;
		cop0_reg.cause.ce = 0;
	}


	template<MemoryAccess::Operation operation>
	void TLB_InvalidException()
	{
		cop0_reg.cause.exc_code = [&] {
			if constexpr (operation == MemoryAccess::Operation::Write) return 3;
			else                                                       return 2;
		}();
		cop0_reg.bad_v_addr.value = address_failure.bad_virt_addr;
		cop0_reg.context.bad_vpn2 = address_failure.bad_vpn2; /* TODO: write to xcontext in 64 bit mode? */
		cop0_reg.entry_hi.vpn2 = address_failure.bad_vpn2; /* TODO: should this assignment be made? */
		cop0_reg.entry_hi.asid = address_failure.bad_asid;
		cop0_reg.cause.ce = 0;
		/* TODO: what about the R field in entry_hi? */
	}


	template<MemoryAccess::Operation operation>
	void TLB_MissException()
	{
		cop0_reg.cause.exc_code = [&] {
			if constexpr (operation == MemoryAccess::Operation::Write) return 3;
			else                                                       return 2;
		}();
		cop0_reg.bad_v_addr.value = address_failure.bad_virt_addr;
		cop0_reg.context.bad_vpn2 = address_failure.bad_vpn2;
		cop0_reg.entry_hi.vpn2 = address_failure.bad_vpn2; /* TODO: should this assignment be made? */
		cop0_reg.entry_hi.asid = address_failure.bad_asid;
		cop0_reg.cause.ce = 0;
	}


	void TLB_ModException()
	{
		cop0_reg.cause.exc_code = 1;
		cop0_reg.bad_v_addr.value = address_failure.bad_virt_addr;
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


	template<MemoryAccess::Operation operation>
	void XTLB_MissException()
	{
		cop0_reg.cause.exc_code = [&] {
			if constexpr (operation == MemoryAccess::Operation::Write) return 3;
			else                                                       return 2;
		}();
		cop0_reg.bad_v_addr.value = address_failure.bad_virt_addr;
		cop0_reg.context.bad_vpn2 = address_failure.bad_vpn2;
		cop0_reg.x_context.bad_vpn2 = address_failure.bad_vpn2;
		cop0_reg.entry_hi.vpn2 = address_failure.bad_vpn2;
		cop0_reg.entry_hi.asid = address_failure.bad_asid;
		cop0_reg.cause.ce = 0;
	}


	constexpr std::string_view ExceptionToString(const Exception exception)
	{
		switch (exception)
		{
		case Exception::AddressError: return "Address Error";
		case Exception::Breakpoint: return "Breakpoint";
		case Exception::BusError: return "Bus Error";
		case Exception::ColdReset: return "Cold Reset";
		case Exception::CoprocessorUnusable: return "Coprocessor Unusable";
		case Exception::FloatingPoint: return "Floating Point";
		case Exception::IntegerOverflow: return "Integer Overflow";
		case Exception::Interrupt: return "Interrupt";
		case Exception::NMI: return "NMI";
		case Exception::ReservedInstruction: return "Reserved instruction";
		case Exception::SoftReset: return "Soft Reset";
		case Exception::Syscall: return "Syscall";
		case Exception::TLB_Invalid: return "Invalid TLB";
		case Exception::TLB_Miss: return "TLB Miss";
		case Exception::TLB_Modification: return "TLB Modification";
		case Exception::Trap: return "Trap";
		case Exception::Watch: return "Watch";
		case Exception::XTLB_Miss: return "XTLB Miss";
		default: assert(false);
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
	template void SignalException<Exception::NMI, MEMORY_OPERATION>(); \
	template void SignalException<Exception::ReservedInstruction, MEMORY_OPERATION>(); \
	template void SignalException<Exception::SoftReset, MEMORY_OPERATION>(); \
	template void SignalException<Exception::Syscall, MEMORY_OPERATION>(); \
	template void SignalException<Exception::TLB_Invalid, MEMORY_OPERATION>(); \
	template void SignalException<Exception::TLB_Miss, MEMORY_OPERATION>(); \
	template void SignalException<Exception::TLB_Modification, MEMORY_OPERATION>(); \
	template void SignalException<Exception::Trap, MEMORY_OPERATION>(); \
	template void SignalException<Exception::Watch, MEMORY_OPERATION>(); \
	template void SignalException<Exception::XTLB_Miss, MEMORY_OPERATION>();

	ENUMERATE_SIGNAL_EXCEPTION_SPECIALIZATIONS(MemoryAccess::Operation::Read)
	ENUMERATE_SIGNAL_EXCEPTION_SPECIALIZATIONS(MemoryAccess::Operation::Write)
	ENUMERATE_SIGNAL_EXCEPTION_SPECIALIZATIONS(MemoryAccess::Operation::InstrFetch)

	template void SignalAddressErrorException<MemoryAccess::Operation::InstrFetch>(u64);
	template void SignalAddressErrorException<MemoryAccess::Operation::Read>(u64);
	template void SignalAddressErrorException<MemoryAccess::Operation::Write>(u64);
}